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
  void AudioData(const float* audioData, int audioDataLength, float *freqData, int freqDataLength) override;

  void OnCompiledAndLinked() override;
  bool OnEnabled() override;

private:
  float m_fWaveform[2][512];

  glm::mat4 m_modelProjMat;

#ifdef HAS_GL
  GLuint m_vertexVBO = 0;
#endif
  GLint m_uModelProjMatrix = -1;
  GLint m_aPosition = -1;
  GLint m_aColor = -1;

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

#ifdef HAS_GL
  glGenBuffers(1, &m_vertexVBO);
#endif

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
  struct PackedVertex {
    float position[3]; // Position x, y, z
    float color[4]; // Color r, g, b, a
  } vertices[256];

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);

  glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, position)));
  glEnableVertexAttribArray(m_aPosition);

  glVertexAttribPointer(m_aColor, 4, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, color)));
  glEnableVertexAttribArray(m_aColor);
#else
  float position[256][3]; // Position x, y, z
  float color[256][4]; // Color r, g, b, a

  glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, GL_FALSE, 0, position);
  glEnableVertexAttribArray(m_aPosition);

  glVertexAttribPointer(m_aColor, 4, GL_FLOAT, GL_FALSE, 0, color);
  glEnableVertexAttribArray(m_aColor);
#endif

  glDisable(GL_BLEND);

  m_modelProjMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f ,0.0f ,-1.0f));
  m_modelProjMat = glm::rotate(m_modelProjMat, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
  m_modelProjMat = glm::rotate(m_modelProjMat, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
  m_modelProjMat = glm::rotate(m_modelProjMat, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));

  EnableShader();

  // Left channel
#ifdef HAS_GL
  for (int i = 0; i < 256; i++)
  {
    vertices[i].color[0] = 0.5f;
    vertices[i].color[1] = 0.5f;
    vertices[i].color[2] = 0.5f;
    vertices[i].color[3] = 1.0f;
    vertices[i].position[0] = -1.0f + ((i / 255.0f) * 2.0f);
    vertices[i].position[1] = 0.5f + m_fWaveform[0][i];
    vertices[i].position[2] = 1.0f;
  }

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
#else
  for (int i = 0; i < 256; i++)
  {
    color[i][0] = 0.5f;
    color[i][1] = 0.5f;
    color[i][2] = 0.5f;
    color[i][3] = 1.0f;
    position[i][0] = -1.0f + ((i / 255.0f) * 2.0f);
    position[i][1] = 0.5f + m_fWaveform[0][i];
    position[i][2] = 1.0f;
  }
#endif

  glDrawArrays(GL_LINE_STRIP, 0, 256);

  // Right channel
#ifdef HAS_GL
  for (int i = 0; i < 256; i++)
  {
    vertices[i].color[0] = 0.5f;
    vertices[i].color[1] = 0.5f;
    vertices[i].color[2] = 0.5f;
    vertices[i].color[3] = 1.0f;
    vertices[i].position[0] = -1.0f + ((i / 255.0f) * 2.0f);
    vertices[i].position[1] = -0.5f + m_fWaveform[1][i];
    vertices[i].position[2] = 1.0f;
  }

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
#else
  for (int i = 0; i < 256; i++)
  {
    color[i][0] = 0.5f;
    color[i][1] = 0.5f;
    color[i][2] = 0.5f;
    color[i][3] = 1.0f;
    position[i][0] = -1.0f + ((i / 255.0f) * 2.0f);
    position[i][1] = -0.5f + m_fWaveform[1][i];
    position[i][2] = 1.0f;
  }
#endif

  glDrawArrays(GL_LINE_STRIP, 0, 256);

  DisableShader();

  glDisableVertexAttribArray(m_aPosition);
  glDisableVertexAttribArray(m_aColor);

  glEnable(GL_BLEND);
}

void CVisualizationWaveForm::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  (void)pFreqData;
  (void)iFreqDataLength;

  int ipos=0;
  while (ipos < 512)
  {
    for (int i=0; i < iAudioDataLength; i+=2)
    {
      m_fWaveform[0][ipos] = pAudioData[i  ]; // left channel
      m_fWaveform[1][ipos] = pAudioData[i+1]; // right channel
      ipos++;
      if (ipos >= 512)
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
