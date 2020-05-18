/*
 *  Copyright (C) 2008-2020 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

// Waveform.vis
// A simple visualisation example by MrC

#include <kodi/addon-instance/Visualization.h>
#include <stdio.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
  CVisualizationWaveForm() = default;
  ~CVisualizationWaveForm() override;

  ADDON_STATUS Create() override;
  bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) override;
  void Render() override;
  void AudioData(const float* audioData, int audioDataLength, float *freqData, int freqDataLength) override;

private:
  bool init_renderer_objs();

  ID3D11Device* m_device = nullptr;
  ID3D11DeviceContext* m_context = nullptr;
  ID3D11VertexShader* m_vShader = nullptr;
  ID3D11PixelShader* m_pShader = nullptr;
  ID3D11InputLayout* m_inputLayout = nullptr;
  ID3D11Buffer* m_vBuffer = nullptr;
  ID3D11Buffer* m_cViewPort = nullptr;
  float m_fWaveform[2][1024];
  D3D11_VIEWPORT m_viewport;
  Vertex_t m_verts[1024 * 2];

  int m_usedLinePoints = 500;
  glm::vec4 m_backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  glm::vec4 m_lineColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
  bool m_ignoreResample = false;

  bool m_startOK = false;
};

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
  m_viewport.TopLeftX = static_cast<float>(X());
  m_viewport.TopLeftY = static_cast<float>(Y());
  m_viewport.Width = static_cast<float>(Width());
  m_viewport.Height = static_cast<float>(Height());
  m_viewport.MinDepth = 0;
  m_viewport.MaxDepth = 1;
  m_context = (ID3D11DeviceContext*)Device();
  m_context->GetDevice(&m_device);
  if (!init_renderer_objs())
    return ADDON_STATUS_PERMANENT_FAILURE;

  return ADDON_STATUS_OK;
}

//-- Start -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
bool CVisualizationWaveForm::Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName)
{
  (void)channels;
  (void)samplesPerSec;
  (void)bitsPerSample;
  (void)songName;

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

  return true;
}

//-- Audiodata ----------------------------------------------------------------
// Called by Kodi to pass new audio data to the vis
//-----------------------------------------------------------------------------
void CVisualizationWaveForm::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  (void)pFreqData;
  (void)iFreqDataLength;

  int ipos = 0;
  int length = iAudioDataLength;
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
    for (int i=0; i < length; i+=usedStep)
    {
      m_fWaveform[0][ipos] = pAudioData[i  ]; // left channel
      m_fWaveform[1][ipos] = pAudioData[i+1]; // right channel
      ipos++;
      if (ipos >= m_usedLinePoints)
         break;
    }
  }
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
void CVisualizationWaveForm::Render()
{
  D3D11_MAPPED_SUBRESOURCE res;
  unsigned stride = sizeof(Vertex_t), offset = 0;
  m_context->IASetVertexBuffers(0, 1, &m_vBuffer, &stride, &offset);
  m_context->IASetInputLayout(m_inputLayout);
  m_context->VSSetShader(m_vShader, 0, 0);
  m_context->VSSetConstantBuffers(0, 1, &m_cViewPort);
  m_context->PSSetShader(m_pShader, 0, 0);

  if (m_backgroundColor.a != 0.0f)
  {
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Set background color
    for (int i = 0; i < 5; i++)
      m_verts[i].col = XMFLOAT4(glm::value_ptr(m_backgroundColor));
    m_verts[0].x = m_viewport.TopLeftX;
    m_verts[0].y = m_viewport.TopLeftY;
    m_verts[0].z = 1.0;
    m_verts[1].x = m_viewport.TopLeftX;
    m_verts[1].y = m_viewport.TopLeftY + m_viewport.Height;
    m_verts[1].z = 1.0;
    m_verts[2].x = m_viewport.TopLeftX + m_viewport.Width;
    m_verts[2].y = m_viewport.TopLeftY + m_viewport.Height;
    m_verts[2].z = 1.0;
    m_verts[3].x = m_viewport.TopLeftX + m_viewport.Width;
    m_verts[3].y = m_viewport.TopLeftY;
    m_verts[3].z = 1.0;
    m_verts[4].x = m_viewport.TopLeftX;
    m_verts[4].y = m_viewport.TopLeftY;
    m_verts[4].z = 1.0;

    // a little optimization: generate and send all vertecies for both channels
    if (S_OK == m_context->Map(m_vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
    {
      memcpy(res.pData, m_verts, sizeof(Vertex_t) * 5);
      m_context->Unmap(m_vBuffer, 0);
    }
    // draw background
    m_context->Draw(5, 0);
  }

  m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

  int j = 0;
  // Left channel
  for (int i = 0; i < m_usedLinePoints; i++)
  {
    m_verts[j].col = XMFLOAT4(glm::value_ptr(m_lineColor));
    m_verts[j].x = m_viewport.TopLeftX + (-1.0f + ((i / float(m_usedLinePoints)) * 2.0f) * m_viewport.Width);
    m_verts[j].y = m_viewport.TopLeftY + m_viewport.Height * 0.25f + (m_fWaveform[0][i] * m_viewport.Height * 0.3f);
    m_verts[j].z = 1.0;
    j++;
  }

  // Right channel
  for (int i = 0; i < m_usedLinePoints; i++)
  {
    m_verts[j].col = XMFLOAT4(glm::value_ptr(m_lineColor));
    m_verts[j].x = m_viewport.TopLeftX + (-1.0f + ((i / float(m_usedLinePoints)) * 2.0f) * m_viewport.Width);
    m_verts[j].y = m_viewport.TopLeftY + m_viewport.Height * 0.75f + (m_fWaveform[1][i] * m_viewport.Height * 0.3f);
    m_verts[j].z = 1.0;
    j++;
  }

  // a little optimization: generate and send all vertecies for both channels
  if (S_OK == m_context->Map(m_vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    memcpy(res.pData, m_verts, sizeof(Vertex_t) * j);
    m_context->Unmap(m_vBuffer, 0);
  }
  // draw left channel
  m_context->Draw(m_usedLinePoints, 0);
  // draw right channel
  m_context->Draw(m_usedLinePoints, m_usedLinePoints);
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
  CD3D11_BUFFER_DESC desc(sizeof(Vertex_t) * 1024 * 2, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
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
