//
// Created by Niffoxic (Aka Harsh Dubey) on 12/27/2025.
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

#ifndef DIRECTX12_OBJECT_H
#define DIRECTX12_OBJECT_H

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

#include "decriptor_heap.h"
#include "utility/json_loader.h"
#include "utility/mesh_generator.h"

enum class EPrimitiveMode : std::uint8_t
{
	PointList      = 0,
	LineStrip      = 1,
	LineList       = 2,
	TriangleStrip  = 3,
	TriangleList   = 4,
};

inline static D3D_PRIMITIVE_TOPOLOGY GetTopologyType(const EPrimitiveMode mode)
{
	switch (mode)
	{
		case EPrimitiveMode::PointList:     return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case EPrimitiveMode::LineStrip:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case EPrimitiveMode::LineList:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case EPrimitiveMode::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case EPrimitiveMode::TriangleList:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		default:                           return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

inline static std::string ToString(const EPrimitiveMode mode)
{
	switch (mode)
	{
		case EPrimitiveMode::PointList:     return "PointList";
		case EPrimitiveMode::LineStrip:     return "LineStrip";
		case EPrimitiveMode::LineList:      return "LineList";
		case EPrimitiveMode::TriangleStrip: return "TriangleStrip";
		case EPrimitiveMode::TriangleList:  return "TriangleList";
		default:                           return "Unknown";
	}
}

struct Transformation
{
	DirectX::XMFLOAT3 Position{ 0.f, 0.f, 0.f };
	DirectX::XMFLOAT3 Rotation{ 0.f, 0.f, 0.f }; // Pitch, Yaw, Roll (radians)
	DirectX::XMFLOAT3 Scale   { 1.f, 1.f, 1.f };

private:
	mutable DirectX::XMMATRIX m_cached{ DirectX::XMMatrixIdentity() };
	mutable bool m_dirty{ true };

public:
	void MarkDirty() const noexcept { m_dirty = true; }
	DirectX::XMFLOAT4X4 GetTransform() const;
	void				ImguiView	();
};

struct ConstantData
{
	std::uint32_t Size;
	std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> Views;
	std::vector<BYTE*> Mapped;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GPUHandle;
};

struct MeshGeometry
{
	Microsoft::WRL::ComPtr<ID3D12Resource> GeometryBuffer; // both index and vertex
	Microsoft::WRL::ComPtr<ID3D12Resource> GeometryUploader;
	std::vector<D3D12_VERTEX_BUFFER_VIEW>  VertexViews;
	D3D12_INDEX_BUFFER_VIEW IndexViews;

	MeshData Data{};
	std::uint32_t VertexStride;
	std::uint32_t VertexByteSize;
	BYTE* Mapped{ nullptr };

	UINT IndexCount			{ 0u };
	UINT StartIndexLocation	{ 0u };
	UINT BaseVertexLocation	{ 0u };

	void InitGeometryBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const MeshData& mesh,
		bool keepMapping=false);
};

struct PerObjectConstantsCPU
{
	DirectX::XMFLOAT4X4 World;
};

enum class ELightType : std::uint8_t
{
	Directional,
	Point,
	Spotlight
};

inline std::string ToString(const ELightType type)
{
	switch (type)
	{
		case ELightType::Directional: return "Directional";
		case ELightType::Point:       return "Point";
		case ELightType::Spotlight:   return "Spotlight";
	}
	return "Unknown";
}

struct LightCPU
{
	DirectX::XMFLOAT3 Strength;
	float FalloffStart = 0.0f;  // point/spot
	DirectX::XMFLOAT3 Direction;
	float FalloffEnd   = 0.0f;  // dir/spot + range
	DirectX::XMFLOAT3 Position;
	float SpotPower    = 0.0f;  // point/spot
};

struct PassConstantsCPU;

struct LightManager
{
	static constexpr std::uint32_t MaxLights = 16;

	std::vector<LightCPU> DirectionalLights;
	std::vector<LightCPU> PointLights;
	std::vector<LightCPU> SpotLights;

	// Creation
	LightCPU& AddDirectional(const DirectX::XMFLOAT3& direction,
							 const DirectX::XMFLOAT3& strength);

	LightCPU& AddPoint(const DirectX::XMFLOAT3& position,
					   const DirectX::XMFLOAT3& strength,
					   float falloffStart,
					   float falloffEnd);

	LightCPU& AddSpot(const DirectX::XMFLOAT3& position,
					  const DirectX::XMFLOAT3& direction,
					  const DirectX::XMFLOAT3& strength,
					  float falloffStart,
					  float falloffEnd,
					  float spotPower);

	// Management
	void Clear();
	std::uint32_t TotalLightCount() const;

	// GPU packing
	void FillPassConstants(PassConstantsCPU& out) const;

	void ImguiView();

	JsonLoader GetJsonData() const;
	void LoadJsonData(const JsonLoader& data);
};

struct PassConstantsCPU
{
	DirectX::XMFLOAT4X4 View;
	DirectX::XMFLOAT4X4 InvView;
	DirectX::XMFLOAT4X4 Projection;
	DirectX::XMFLOAT4X4 InvProjection;
	DirectX::XMFLOAT4X4 ViewProjection;
	DirectX::XMFLOAT4X4 InvViewProjection;
	DirectX::XMFLOAT3	EyePositionW;
	float padding;
	DirectX::XMFLOAT2 RenderTargetSize;
	DirectX::XMFLOAT2 InvRenderTargetSize;
	float NearZ;
	float FarZ;
	float TotalTime;
	float DeltaTime;

	// lights
	DirectX::XMFLOAT4 AmbientLight{ 0.10f, 0.10f, 0.10f, 1.0f };

	std::uint32_t NumDirLights   = 1;
	std::uint32_t NumPointLights = 0;
	std::uint32_t NumSpotLights  = 0;
	std::uint32_t cbPassPad2     = 0;

	static constexpr std::uint32_t MaxLights = 16;
	LightCPU Lights[MaxLights]{};
};

struct RenderItem
{
	std::string Name{ "NoName" };
	bool		   Visible{ true };
	EPrimitiveMode PrimitiveMode{ EPrimitiveMode::TriangleList };
	MeshGeometry*  Mesh;
	Transformation Transform;
	std::uint32_t  FrameIndex{ 0 };
	std::uint32_t  FrameCount{ 1u };

	//~ Constants
	Microsoft::WRL::ComPtr<ID3D12Resource>	 ConstantBuffer;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> BaseCBHandle;
	ConstantData PerObject	 {};
	ConstantData PassConstant{};

	void InitConstantBuffer(
		const std::uint32_t frameCount,
		ID3D12Device* device,
		framework::DescriptorHeap& heap);

	void ImguiView();
};

struct Material
{
	std::string   Name		       { "NoName" };
	std::uint32_t SrvHeapIndex     { 0u };
	std::uint32_t FrameIndex       { 0u };
	std::uint32_t FrameCount       { 1u };

	struct MaterialConstants
	{
		DirectX::XMFLOAT4	DiffuseAlbedo{ 1.f, 1.f, 1.f, 1.f };
		DirectX::XMFLOAT3	FresnelR0	 { 0.04f, 0.04f, 0.04f };
		float				Roughness	 { 0.25f };
		DirectX::XMFLOAT4X4 MatTransform;
	};
	MaterialConstants Config{};

	void ImguiView();

	//~ pixel constant buffers
	Microsoft::WRL::ComPtr<ID3D12Resource>	 PixelConstantBuffer;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> BasePCBHandle;
	ConstantData PixelConstantMap{};

	void InitPixelConstantBuffer(
	const std::uint32_t frameCount,
	ID3D12Device* device,
	framework::DescriptorHeap& heap);
};

#endif //DIRECTX12_OBJECT_H
