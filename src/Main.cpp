/*
 *      Copyright (C) 2008-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// Waveform.vis
// A simple visualisation example by MrC

#include <kodi/addon-instance/Visualization.h>
#include <stdio.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

typedef struct {
  int TopLeftX;
  int TopLeftY;
  int Width;
  int Height;
  int MinDepth;
  int MaxDepth;
} D3D11_VIEWPORT;
typedef unsigned long D3DCOLOR;

struct Vertex_t
{
  float x, y, z;
  D3DCOLOR  col;
};

class CVisualizationWaveForm
  : public kodi::addon::CAddonBase,
    public kodi::addon::CInstanceVisualization
{
public:
  CVisualizationWaveForm();
  virtual ~CVisualizationWaveForm();

  virtual ADDON_STATUS Create() override;
  virtual void Render() override;
  virtual void AudioData(const float* audioData, int audioDataLength, float *freqData, int freqDataLength) override;

private:
  void* m_device;
  float m_fWaveform[2][512];
  D3D11_VIEWPORT m_viewport;
};

CVisualizationWaveForm::CVisualizationWaveForm()
{
  m_device = nullptr;
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
CVisualizationWaveForm::~CVisualizationWaveForm()
{
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS CVisualizationWaveForm::Create()
{
  m_device = Device();
  m_viewport.TopLeftX = -1;
  m_viewport.TopLeftY = -1;
  m_viewport.Width = 2;
  m_viewport.Height = 2;
  m_viewport.MinDepth = 0;
  m_viewport.MaxDepth = 1;

  return ADDON_STATUS_OK;
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
void CVisualizationWaveForm::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int ipos=0;
  while (ipos < 512)
  {
    for (int i=0; i < iAudioDataLength; i+=2)
    {
      m_fWaveform[0][ipos] = pAudioData[i  ]; // left channel
      m_fWaveform[1][ipos] = pAudioData[i+1]; // right channel
      ipos++;
      if (ipos >= 512) break;
    }
  }
}


//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
void CVisualizationWaveForm::Render()
{
  Vertex_t  verts[512];

  // Left channel
  GLenum errcode;
  glColor3f(1.0, 1.0, 1.0);
  glDisable(GL_BLEND);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glTranslatef(0,0,-1.0);
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < 256; i++)
  {
    verts[i].col = 0xffffffff;
    verts[i].x = m_viewport.TopLeftX + ((i / 255.0f) * m_viewport.Width);
    verts[i].y = m_viewport.TopLeftY + m_viewport.Height * 0.33f + (m_fWaveform[0][i] * m_viewport.Height * 0.15f);
    verts[i].z = 1.0;
    glVertex2f(verts[i].x, verts[i].y);
  }

  glEnd();
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }

  // Right channel
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < 256; i++)
  {
    verts[i].col = 0xffffffff;
    verts[i].x = m_viewport.TopLeftX + ((i / 255.0f) * m_viewport.Width);
    verts[i].y = m_viewport.TopLeftY + m_viewport.Height * 0.66f + (m_fWaveform[1][i] * m_viewport.Height * 0.15f);
    verts[i].z = 1.0;
    glVertex2f(verts[i].x, verts[i].y);
  }

  glEnd();
  glEnable(GL_BLEND);


  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
}

ADDONCREATOR(CVisualizationWaveForm)
