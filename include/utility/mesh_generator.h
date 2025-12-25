//
// Created by Niffoxic (Aka Harsh Dubey) on 12/22/2025.
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
// Copyright (c) 2025 Niffoxic
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

#ifndef DIRECTX12_MESH_GENERATOR_H
#define DIRECTX12_MESH_GENERATOR_H

#include <DirectXMath.h>
#include <cstdint>
#include <d3d12.h>
#include <vector>

struct MeshVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 Tangent;
	DirectX::XMFLOAT2 UV;
	DirectX::XMFLOAT3 Color;

	static const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputLayout()
	{
		static const std::vector<D3D12_INPUT_ELEMENT_DESC> layout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		return layout;
	}
};


struct MeshData
{
	std::vector<MeshVertex> vertices;
	std::vector<uint32_t> indices;
};

struct GenerateBoxConfig
{
	DirectX::XMFLOAT3 Extents{ 0.5f, 0.5f, 0.5f };
	DirectX::XMFLOAT3 Color{ 1.f, 1.f, 1.f };

	uint32_t Subdivisions{ 0 };
	bool GenerateTangents{ true };
	bool FlipWinding{ false };
	bool InsideOut{ false };
};

struct GenerateMountainConfig
{
	float Width{ 1.0f };
	float Depth{ 1.0f };

	uint32_t SubdivisionsX{ 1 };
	uint32_t SubdivisionsZ{ 1 };

	DirectX::XMFLOAT3 Normal{ 0.f, 1.f, 0.f };

	bool GenerateTangents{ true };
	bool FlipWinding{ false };
	bool Centered{ true };

	float HeightScale{ 6.0f };
	float Harshness{ 2.5f };
	float Falloff{ 6.0f };

	float Freq1{ 0.15f };
	float Freq2{ 0.45f };
	float Amp1{ 1.8f };
	float Amp2{ 0.6f };

	DirectX::XMFLOAT3 GroundGreen{ 0.20f, 0.55f, 0.20f };
	DirectX::XMFLOAT3 GroundBrown{ 0.45f, 0.30f, 0.15f };
	DirectX::XMFLOAT3 SnowColor{ 0.95f, 0.95f, 0.98f };

	float SnowStart{ 0.65f };
	float SnowBlend{ 0.15f };
};

struct GenerateSphereConfig
{
	float Radius{ 0.5f };
	uint32_t SliceCount{ 20 }; // longitude
	uint32_t StackCount{ 20 }; // latitude

	DirectX::XMFLOAT3 Color{ 1.f, 1.f, 1.f };

	bool GenerateTangents{ true };
	bool FlipWinding{ false };
	bool InsideOut{ false };
};

struct GenerateCylinderConfig
{
	float BottomRadius{ 0.5f };
	float TopRadius{ 0.5f };
	float Height{ 1.0f };

	uint32_t SliceCount{ 20 };
	uint32_t StackCount{ 1 };

	DirectX::XMFLOAT3 Color{ 1.f, 1.f, 1.f };

	bool CapTop{ true };
	bool CapBottom{ true };
	bool GenerateTangents{ true };
	bool FlipWinding{ false };
	bool InsideOut{ false };
};

// MeshGenerator
class MeshGenerator
{
public:
	//~ Primitives
	static MeshData GenerateBox(const GenerateBoxConfig& config);
	static MeshData GenerateMountain(const GenerateMountainConfig& config);
	static MeshData GenerateSphere(const GenerateSphereConfig& config);
	static MeshData GenerateCylinder(const GenerateCylinderConfig& config);

	//~ Utilities
	static void ComputeNormals(MeshData& mesh, bool flip = false);
	static void ComputeTangents(MeshData& mesh, bool flip = false);
	static void Transform(MeshData& mesh, DirectX::CXMMATRIX M);
	static void Append(MeshData& dst, const MeshData& src);
};

#endif // DIRECTX12_MESH_GENERATOR_H
