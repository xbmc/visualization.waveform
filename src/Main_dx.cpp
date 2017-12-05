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
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <stdio.h>

using namespace DirectX;
using namespace DirectX::PackedVector;

// Include the precompiled shader code.
namespace
{
  #include "DefaultPixelShader.inc"
  #include "DefaultVertexShader.inc"
}

struct cbViewPort
{
  float g_viewPortWidth;
  float g_viewPortHeigh;
  float align1, align2;
};

struct Vertex_t
{
  float x, y, z;
  XMFLOAT4 col;
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
  bool init_renderer_objs();

  ID3D11Device* m_device;
  ID3D11DeviceContext* m_context;
  ID3D11VertexShader* m_vShader;
  ID3D11PixelShader* m_pShader;
  ID3D11InputLayout* m_inputLayout;
  ID3D11Buffer* m_vBuffer;
  ID3D11Buffer* m_cViewPort;
  float m_fWaveform[2][512];
  D3D11_VIEWPORT m_viewport;
};

CVisualizationWaveForm::CVisualizationWaveForm()
{
  m_device = nullptr;
  m_context = nullptr;
  m_vShader = nullptr;
  m_pShader = nullptr;
  m_inputLayout = nullptr;
  m_vBuffer = nullptr;
  m_cViewPort = nullptr;
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
CVisualizationWaveForm::~CVisualizationWaveForm()
{
  if (m_cViewPort)
    m_cViewPort->Release();
  if (m_vBuffer)
    m_vBuffer->Release();
  if (m_inputLayout)
    m_inputLayout->Release();
  if (m_vShader)
    m_vShader->Release();
  if (m_pShader)
    m_pShader->Release();
  if (m_device)
    m_device->Release();
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS CVisualizationWaveForm::Create()
{
  m_viewport.TopLeftX = X();
  m_viewport.TopLeftY = Y();
  m_viewport.Width = Width();
  m_viewport.Height = Height();
  m_viewport.MinDepth = 0;
  m_viewport.MaxDepth = 1;
  m_context = (ID3D11DeviceContext*)Device();
  m_context->GetDevice(&m_device);
  if (!init_renderer_objs())
    return ADDON_STATUS_PERMANENT_FAILURE;

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

  unsigned stride = sizeof(Vertex_t), offset = 0;
  m_context->IASetVertexBuffers(0, 1, &m_vBuffer, &stride, &offset);
  m_context->IASetInputLayout(m_inputLayout);
  m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
  m_context->VSSetShader(m_vShader, 0, 0);
  m_context->VSSetConstantBuffers(0, 1, &m_cViewPort);
  m_context->PSSetShader(m_pShader, 0, 0);
  float xcolor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

  // Left channel
  for (int i = 0; i < 256; i++)
  {
    verts[i].col = XMFLOAT4(xcolor);;
    verts[i].x = m_viewport.TopLeftX + ((i / 255.0f) * m_viewport.Width);
    verts[i].y = m_viewport.TopLeftY + m_viewport.Height * 0.33f + (m_fWaveform[0][i] * m_viewport.Height * 0.15f);
    verts[i].z = 1.0;
  }

  // Right channel
  for (int i = 256; i < 512; i++)
  {
    verts[i].col = XMFLOAT4(xcolor);
    verts[i].x = m_viewport.TopLeftX + (((i - 256) / 255.0f) * m_viewport.Width);
    verts[i].y = m_viewport.TopLeftY + m_viewport.Height * 0.66f + (m_fWaveform[1][i] * m_viewport.Height * 0.15f);
    verts[i].z = 1.0;
  }

  // a little optimization: generate and send all vertecies for both channels
  D3D11_MAPPED_SUBRESOURCE res;
  if (S_OK == m_context->Map(m_vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    memcpy(res.pData, verts, sizeof(Vertex_t) * 512);
    m_context->Unmap(m_vBuffer, 0);
  }
  // draw left channel
  m_context->Draw(256, 0);
  // draw right channel
  m_context->Draw(256, 256);
}

bool CVisualizationWaveForm::init_renderer_objs()
{
  // Create vertex shader
  if (S_OK != m_device->CreateVertexShader(DefaultVertexShaderCode, sizeof(DefaultVertexShaderCode), nullptr, &m_vShader))
    return false;

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  if (S_OK != m_device->CreateInputLayout(layout, ARRAYSIZE(layout), DefaultVertexShaderCode, sizeof(DefaultVertexShaderCode), &m_inputLayout))
    return false;

  // Create pixel shader
  if (S_OK != m_device->CreatePixelShader(DefaultPixelShaderCode, sizeof(DefaultPixelShaderCode), nullptr, &m_pShader))
    return false;

  // create buffers
  CD3D11_BUFFER_DESC desc(sizeof(Vertex_t) * 512, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  if (S_OK != m_device->CreateBuffer(&desc, NULL, &m_vBuffer))
    return false;

  desc.ByteWidth = sizeof(cbViewPort);
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.CPUAccessFlags = 0;

  cbViewPort viewPort = { (float)m_viewport.Width, (float)m_viewport.Height, 0.0f, 0.0f };
  D3D11_SUBRESOURCE_DATA initData;
  initData.pSysMem = &viewPort;

  if (S_OK != m_device->CreateBuffer(&desc, &initData, &m_cViewPort))
    return false;

  // we are ready
  return true;
}

ADDONCREATOR(CVisualizationWaveForm)
