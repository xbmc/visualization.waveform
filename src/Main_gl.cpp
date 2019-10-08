/*
 *      XMMS - Cross-platform multimedia player
 *      Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *      Copyright (C) 2008-2019 Team Kodi
 *      http://kodi.tv
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
  float m_fWaveform[2][1024];

  glm::mat4 m_modelProjMat;

#ifdef HAS_GL
  struct PackedVertex
  {
    glm::vec3 position; // Position x, y, z
    glm::vec4 color; // Color r, g, b, a
  } m_vertices[1024];

  GLuint m_vertexVBO = 0;
#else
  glm::vec3 m_position[1024]; // Position x, y, z
  glm::vec4 m_color[1024]; // Color r, g, b, a
#endif

  GLint m_uModelProjMatrix = -1;
  GLint m_aPosition = -1;
  GLint m_aColor = -1;

  int m_usedLinePoints = 500;
  glm::vec4 m_backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
  glm::vec4 m_lineColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
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

  for (int i = 0; i < m_usedLinePoints; i++)
    m_vertices[i].color = m_lineColor;
#else
  for (int i = 0; i < m_usedLinePoints; i++)
    m_color[i] = m_lineColor;
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

  glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, position)));
  glEnableVertexAttribArray(m_aPosition);

  glVertexAttribPointer(m_aColor, 4, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, color)));
  glEnableVertexAttribArray(m_aColor);
#else
  glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, GL_FALSE, 0, m_position);
  glEnableVertexAttribArray(m_aPosition);

  glVertexAttribPointer(m_aColor, 4, GL_FLOAT, GL_FALSE, 0, m_color);
  glEnableVertexAttribArray(m_aColor);
#endif

  if (m_backgroundColor.a != 0.0f)
  {
    glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, m_backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  glDisable(GL_BLEND);

  EnableShader();

  // Left channel
#ifdef HAS_GL
  for (int i = 0; i < m_usedLinePoints; i++)
    m_vertices[i].position = glm::vec3(-1.0f + ((i / float(m_usedLinePoints)) * 2.0f), 0.5f + m_fWaveform[0][i] * 0.9f, 1.0f);

  glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), m_vertices, GL_STATIC_DRAW);
#else
  for (int i = 0; i < m_usedLinePoints; i++)
    m_position[i] = glm::vec3(-1.0f + ((i / float(m_usedLinePoints)) * 2.0f), 0.5f + m_fWaveform[0][i] * 0.9f, 1.0f);
#endif

  glDrawArrays(GL_LINE_STRIP, 0, m_usedLinePoints);

  // Right channel
#ifdef HAS_GL
  for (int i = 0; i < m_usedLinePoints; i++)
    m_vertices[i].position = glm::vec3(-1.0f + ((i / float(m_usedLinePoints)) * 2.0f), -0.5f + m_fWaveform[1][i] * 0.9f, 1.0f);

  glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), m_vertices, GL_STATIC_DRAW);
#else
  for (int i = 0; i < m_usedLinePoints; i++)
    m_position[i] = glm::vec3(-1.0f + ((i / float(m_usedLinePoints)) * 2.0f), -0.5f + m_fWaveform[1][i] * 0.9f, 1.0f);
#endif

  glDrawArrays(GL_LINE_STRIP, 0, m_usedLinePoints);

  DisableShader();

  glDisableVertexAttribArray(m_aPosition);
  glDisableVertexAttribArray(m_aColor);

  glEnable(GL_BLEND);
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

  m_aPosition = glGetAttribLocation(ProgramHandle(), "a_position");
  m_aColor = glGetAttribLocation(ProgramHandle(), "a_color");
}

bool CVisualizationWaveForm::OnEnabled()
{
  glUniformMatrix4fv(m_uModelProjMatrix, 1, GL_FALSE, glm::value_ptr(m_modelProjMat));
  return true;
}

ADDONCREATOR(CVisualizationWaveForm)
