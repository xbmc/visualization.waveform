/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

struct VS_OUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
};

cbuffer cbViewPort : register(b0)
{
  float g_viewPortWidth;
  float g_viewPortHeigh;
  float align1;
  float align2;
};

VS_OUT main(float4 pos : POSITION, float4 col : COLOR)
{
  VS_OUT r = (VS_OUT)0;
  r.pos.x  =  (pos.x / (g_viewPortWidth / 2.0)) - 1;
  r.pos.y  = -(pos.y / (g_viewPortHeigh / 2.0)) + 1;
  r.pos.z  = pos.z;
  r.pos.w  = 1.0;
  r.col    = col;
  return r;
}