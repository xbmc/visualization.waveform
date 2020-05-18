/*
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 2008-2019 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

/*
 *  Wed May 24 10:49:37 CDT 2000
 *  Fixes to threading/context creation for the nVidia X4 drivers by
 *  Christian Zander <phoenix@minion.de>
 */

/*
 *  Ported to GLES by gimli
 */

#include <string.h>
#include <math.h>

#include <kodi/addon-instance/Visualization.h>
#include <kodi/gui/gl/GL.h>
#include <kodi/gui/gl/Shader.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define NUM_BANDS 16

#ifndef M_PI
#define M_PI 3.141592654f
#endif

class ATTRIBUTE_HIDDEN CVisualizationWaveForm
  : public kodi::addon::CAddonBase,
    public kodi::addon::CInstanceVisualization,
    public kodi::gui::gl::CShaderProgram
{
public:
  CVisualizationWaveForm() = default;
  ~CVisualizationWaveForm() override = default;

  bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) override;
  void Stop() override;
  void Render() override;
  bool IsDirty() override { return true; }
  void AudioData(const float* audioData, int audioDataLength, float *freqData, int freqDataLength) override;

  void OnCompiledAndLinked() override;
  bool OnEnabled() override;

private:
  void DrawLine(float* waveform, bool topBottom);

  float m_fWaveform[2][1024];

  glm::mat4 m_modelProjMat;

#ifdef HAS_GL
  GLuint m_vertexVBO = 0;
#endif
  std::vector<glm::vec3> m_position; // Position x, y, z

  GLint m_uModelProjMatrix = -1;
  GLint m_uColor = -1;
  GLint m_aPosition = -1;

  int m_usedLinePoints = 500;
  glm::vec4 m_backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
  glm::vec4 m_lineColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
  int m_lineThickness = 3;
  float m_lineThicknessFactor;
  bool m_ignoreResample = false;

  bool m_startOK = false;
};

//-- Start -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
bool CVisualizationWaveForm::Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName)
{
  (void)channels;
  (void)samplesPerSec;
  (void)bitsPerSample;
  (void)songName;

  std::string fraqShader = kodi::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/frag.glsl");
  std::string vertShader = kodi::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/vert.glsl");
  if (!LoadShaderFiles(vertShader, fraqShader) || !CompileAndLink())
  {
    kodi::Log(ADDON_LOG_ERROR, "Failed to create or compile shader");
    return false;
  }

  kodi::CheckSettingInt("points-per-line", m_usedLinePoints);

  // If lines are 0 use the old style from before
  if (m_usedLinePoints == 0)
  {
    m_usedLinePoints = 250;
    m_ignoreResample = true;
  }
  else
  {
    m_ignoreResample = false;
  }

  kodi::CheckSettingInt("line-thickness", m_lineThickness);
  m_lineThicknessFactor = 1.0f / static_cast<float>(Height()) * static_cast<float>(m_lineThickness) / 2.0f;
  if (m_lineThickness == 1)
  {
    glLineWidth(1.0f); // Force set to 1.0
    m_position.resize(1024);
  }
  else
  {
    m_position.resize(1024*6);
  }

  kodi::CheckSettingFloat("line-red", m_lineColor.r);
  kodi::CheckSettingFloat("line-green", m_lineColor.g);
  kodi::CheckSettingFloat("line-blue", m_lineColor.b);

  kodi::CheckSettingFloat("bg-red", m_backgroundColor.r);
  kodi::CheckSettingFloat("bg-green", m_backgroundColor.g);
  kodi::CheckSettingFloat("bg-blue", m_backgroundColor.b);
  if (m_backgroundColor.r != 0.0f || m_backgroundColor.g != 0.0f || m_backgroundColor.b != 0.0f)
    m_backgroundColor.a = 1.0f;
  else
    m_backgroundColor.a = 0.0f;

#ifdef HAS_GL
  glGenBuffers(1, &m_vertexVBO);
#endif

  m_modelProjMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f ,0.0f ,-1.0f));
  m_modelProjMat = glm::rotate(m_modelProjMat, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
  m_modelProjMat = glm::rotate(m_modelProjMat, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
  m_modelProjMat = glm::rotate(m_modelProjMat, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));

  m_startOK = true;
  return true;
}

void CVisualizationWaveForm::Stop()
{
  if (!m_startOK)
    return;

  m_startOK = false;

#ifdef HAS_GL
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &m_vertexVBO);
  m_vertexVBO = 0;
#endif
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
void CVisualizationWaveForm::Render()
{
  if (!m_startOK)
    return;

#ifdef HAS_GL
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);

  glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), BUFFER_OFFSET(offsetof(glm::vec3, x)));
  glEnableVertexAttribArray(m_aPosition);

  glEnable(GL_LINE_SMOOTH);
#else
  glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, GL_FALSE, 0, m_position.data());
  glEnableVertexAttribArray(m_aPosition);
#endif

  if (m_backgroundColor.a != 0.0f)
  {
    glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, m_backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  glDisable(GL_BLEND);

  EnableShader();

  // Left channel
  DrawLine(m_fWaveform[0], false);

  // Right channel
  DrawLine(m_fWaveform[1], true);

  DisableShader();

  glDisableVertexAttribArray(m_aPosition);

  glEnable(GL_BLEND);

#ifdef HAS_GL
  glDisable(GL_LINE_SMOOTH);
#endif
}

void CVisualizationWaveForm::DrawLine(float* waveform, bool topBottom)
{
  int mode;
  int ptr = 0;
  float posYOffset = topBottom ? -0.5f : 0.5f;

  if (m_lineThickness > 1)
  {
    for (int i = 0; i < m_usedLinePoints-1; i++)
    {
      glm::vec2 A = glm::vec2(-1.0f + ((i     / float(m_usedLinePoints-1)) * 2.0f), posYOffset + waveform[i]   * 0.9f);
      glm::vec2 B = glm::vec2(-1.0f + (((i+1) / float(m_usedLinePoints-1)) * 2.0f), posYOffset + waveform[i+1] * 0.9f);

      glm::vec2 p(B.x - A.x, B.y - A.y);
      p = glm::normalize(p);
      glm::vec2 p1(-p.y, p.x), p2(p.y, -p.x);

      m_position[ptr++] = glm::vec3(A, 1.0f);
      m_position[ptr++] = glm::vec3(B, 1.0f);
      m_position[ptr++] = glm::vec3(A + p1 * m_lineThicknessFactor, 1.0f);
      m_position[ptr++] = glm::vec3(A + p2 * m_lineThicknessFactor, 1.0f);
      m_position[ptr++] = glm::vec3(B + p1 * m_lineThicknessFactor, 1.0f);
      m_position[ptr++] = glm::vec3(B + p2 * m_lineThicknessFactor, 1.0f);
    }

    mode = GL_TRIANGLE_STRIP;
  }
  else
  {
    for (int i = 0; i < m_usedLinePoints; i++)
    {
      m_position[ptr++] = glm::vec3(-1.0f + ((i / float(m_usedLinePoints)) * 2.0f), posYOffset + waveform[i] * 0.9f, 1.0f);
    }

    mode = GL_LINE_STRIP;
  }

#ifdef HAS_GL
  glBufferData(GL_ARRAY_BUFFER, m_position.size()*sizeof(glm::vec3), m_position.data(), GL_STATIC_DRAW);
#endif

  glDrawArrays(mode, 0, ptr);
}

void CVisualizationWaveForm::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  (void)pFreqData;
  (void)iFreqDataLength;

  int ipos = 0;
  int usedStep;
  if (m_ignoreResample)
  {
    usedStep = 2;
  }
  else
  {
    usedStep = (iAudioDataLength / m_usedLinePoints) & ~1;
    usedStep = usedStep < 2 ? 2 : usedStep;
  }
  while (ipos < m_usedLinePoints)
  {
    for (int i=0; i < iAudioDataLength; i+=usedStep)
    {
      m_fWaveform[0][ipos] = pAudioData[i  ]; // left channel
      m_fWaveform[1][ipos] = pAudioData[i+1]; // right channel
      ipos++;
      if (ipos >= m_usedLinePoints)
         break;
    }
  }
}

void CVisualizationWaveForm::OnCompiledAndLinked()
{
  m_uModelProjMatrix = glGetUniformLocation(ProgramHandle(), "u_modelViewProjectionMatrix");
  m_uColor = glGetUniformLocation(ProgramHandle(), "u_color");

  m_aPosition = glGetAttribLocation(ProgramHandle(), "a_position");
}

bool CVisualizationWaveForm::OnEnabled()
{
  glUniformMatrix4fv(m_uModelProjMatrix, 1, GL_FALSE, glm::value_ptr(m_modelProjMat));
  glUniform4fv(m_uColor, 1, glm::value_ptr(m_lineColor));

  return true;
}

ADDONCREATOR(CVisualizationWaveForm)
