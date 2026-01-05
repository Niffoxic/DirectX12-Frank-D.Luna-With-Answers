//
// Created by Niffoxic (Aka Harsh Dubey) on 1/5/2026.
//
// -----------------------------------------------------------------------------
// Project   : DirectX12
// Purpose   : Academic and self-learning computer graphics project.
// Codebase  : DirectX 12 implementation based on the Luna Graphics Programming
//             textbook. This repository includes practical scene-style
//             implementations as well as answers and solutions to the
//             end-of-chapter questions for learning and reference purposes.
// License   : MIT License
// -----------------------------------------------------------------------------
//
// Copyright (c) 2026 Niffoxic
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------------------------------------------------
#ifndef DIRECTX12_COMMON_SCENE_DATA_H
#define DIRECTX12_COMMON_SCENE_DATA_H

#include <string>
#include <DirectXMath.h>
#include <algorithm>
#include <d3d12.h>
#include <cstdint>

inline DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t)
{
	t = std::clamp(t, 0.0f, 1.0f);
	return {
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t
	};
}

inline DirectX::XMFLOAT3 Mul3(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
	return { a.x * b.x, a.y * b.y, a.z * b.z };
}

inline DirectX::XMFLOAT3 Add3(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline DirectX::XMFLOAT3 Clamp01(const DirectX::XMFLOAT3& c)
{
	return {
		std::clamp(c.x, 0.0f, 1.0f),
		std::clamp(c.y, 0.0f, 1.0f),
		std::clamp(c.z, 0.0f, 1.0f)
	};
}

inline D3D12_STATIC_SAMPLER_DESC MakeStaticSampler(
	UINT shaderRegister,
	D3D12_FILTER filter,
	D3D12_TEXTURE_ADDRESS_MODE addressU,
	D3D12_TEXTURE_ADDRESS_MODE addressV,
	D3D12_TEXTURE_ADDRESS_MODE addressW,
	D3D12_COMPARISON_FUNC comparison = D3D12_COMPARISON_FUNC_ALWAYS,
	D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL,
	UINT registerSpace = 0,
	UINT maxAnisotropy = 1,
	D3D12_STATIC_BORDER_COLOR borderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
	float mipLODBias = 0.0f,
	float minLOD = 0.0f,
	float maxLOD = D3D12_FLOAT32_MAX
)
{
	D3D12_STATIC_SAMPLER_DESC s{};
	s.Filter           = filter;
	s.AddressU         = addressU;
	s.AddressV         = addressV;
	s.AddressW         = addressW;
	s.MipLODBias       = mipLODBias;
	s.MaxAnisotropy    = maxAnisotropy;
	s.ComparisonFunc   = comparison;
	s.BorderColor      = borderColor;
	s.MinLOD           = minLOD;
	s.MaxLOD           = maxLOD;
	s.ShaderRegister   = shaderRegister;
	s.RegisterSpace    = registerSpace;
	s.ShaderVisibility = visibility;
	return s;
}

inline D3D12_STATIC_SAMPLER_DESC StaticSampler_LinearWrap(UINT reg)
{
	return MakeStaticSampler(
		reg,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);
}

inline D3D12_STATIC_SAMPLER_DESC StaticSampler_LinearClamp(UINT reg)
{
	return MakeStaticSampler(
		reg,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);
}

inline D3D12_STATIC_SAMPLER_DESC StaticSampler_PointClamp(UINT reg)
{
	return MakeStaticSampler(
		reg,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);
}

inline D3D12_STATIC_SAMPLER_DESC StaticSampler_AnisoWrap(UINT reg, UINT aniso = 8)
{
	return MakeStaticSampler(
		reg,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_COMPARISON_FUNC_ALWAYS,
		D3D12_SHADER_VISIBILITY_PIXEL,
		0,
		aniso
	);
}

inline D3D12_STATIC_SAMPLER_DESC StaticSampler_EnvMap(UINT reg)
{
	return MakeStaticSampler(
		reg,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);
}

enum class ERenderType: std::uint32_t
{
	River,
	Mountain,
};

inline std::string ToString(const ERenderType &type)
{
	switch (type)
	{
		case ERenderType::River:	return "River";
		case ERenderType::Mountain: return "Mountain";
		default: return "Unknown";
	}
	return "Unknown";
}

struct RiverUpdateParam
{
	float amp1 = 0.06f;
	float amp2 = 0.03f;
	float freq1 = 1.40f;
	float freq2 = 2.80f;
	float waveLen1 = 0.35f;
	float waveLen2 = 0.85f;
	float flowSpeed = 0.75f;

	float halfWidth = 2.0f;
	float minZ = -10.0f;
	float maxZ =  10.0f;

	DirectX::XMFLOAT3 leftColor      { 0.02f, 0.12f, 0.25f };
	DirectX::XMFLOAT3 rightColor     { 0.05f, 0.30f, 0.50f };
	DirectX::XMFLOAT3 downLeftColor  { 0.02f, 0.08f, 0.18f };
	DirectX::XMFLOAT3 downRightColor { 0.03f, 0.10f, 0.22f };

	DirectX::XMFLOAT3 shallowColor { 0.05f, 0.35f, 0.55f };
	DirectX::XMFLOAT3 deepColor    { 0.02f, 0.10f, 0.20f };
	DirectX::XMFLOAT3 foamColor    { 0.70f, 0.88f, 0.96f };

	float foamStrength    = 1.25f;
	float shimmerStrength = 0.10f;

	float edgeNoiseStrength = 0.75f;
	float octaveBaseAmp     = 0.020f;
	float octaveBaseFreq    = 1.50f;
	float octaveBaseWaveLen = 0.60f;
	int   octaves           = 5;

	float heightScale = 1.0f;
	float heightBias  = 0.0f;
	float maxHeight   = 0.25f;
	float foamHeightThreshold = 0.10f;

	void ImguiView();
};

#endif //DIRECTX12_COMMON_SCENE_DATA_H