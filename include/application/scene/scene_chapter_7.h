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

#ifndef DIRECTX12_SCENE_CHAPTER_7_H
#define DIRECTX12_SCENE_CHAPTER_7_H
#include <DirectXMath.h>
#include <imgui.h>

#include "interface_scene.h"
#include "utility/mesh_generator.h"

// TODO: Add Skeleton and Water.

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 World;
};

struct PassConstants
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
};

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

	DirectX::XMFLOAT4X4 GetTransform() const
	{
		using namespace DirectX;

		if (m_dirty)
		{
			const XMVECTOR pos   = XMLoadFloat3(&Position);
			const XMVECTOR rot   = XMLoadFloat3(&Rotation);
			const XMVECTOR scale = XMLoadFloat3(&Scale);

			const XMMATRIX S = XMMatrixScalingFromVector(scale);
			const XMMATRIX R = XMMatrixRotationRollPitchYawFromVector(rot);
			const XMMATRIX T = XMMatrixTranslationFromVector(pos);

			m_cached = S * R * T;
			m_dirty  = false;
		}

		XMFLOAT4X4 out{};
		XMStoreFloat4x4(&out, m_cached);
		return out;
	}

	void ImguiView()
	{
		bool changed = false;

		changed |= ImGui::DragFloat3("Position", &Position.x, 0.01f);
		changed |= ImGui::DragFloat3("Rotation (rad)", &Rotation.x, 0.01f);
		changed |= ImGui::DragFloat3("Scale", &Scale.x, 0.01f);

		if (changed)
			MarkDirty();
	}
};

struct MeshGeometry
{
	Microsoft::WRL::ComPtr<ID3D12Resource> GeometryBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> GeometryUploader;

	//~ Geometry Info
	UINT IndexCount			{ 0u };
	UINT StartIndexLocation	{ 0u };
	UINT BaseVertexLocation	{ 0u };
	D3D12_VERTEX_BUFFER_VIEW viewVertex{};
	D3D12_INDEX_BUFFER_VIEW  viewIndex {};
};

struct ConstantData
{
	std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> View;
	std::vector<BYTE*> Mapped{};
};

struct RenderItem
{
	bool Visible{ true };
	MeshGeometry*	Mesh;
	Transformation	Transform;
	UINT			FrameIndex{ 0 };

	//~ Constants
	Microsoft::WRL::ComPtr<ID3D12Resource>	 ConstantBuffer;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> BaseCBHandle;
	ConstantData PerObject	 {};
	ConstantData PassConstant{};
};

enum class EShape
{
	Sphere,
	Box,
	Cylinder,
	Mountain,
	River,
};

inline std::string ToString(const EShape &shape)
{
	switch (shape)
	{
		case EShape::Sphere: return "Sphere";
		case EShape::Box: return "Box";
		case EShape::Cylinder: return "Cylinder";
		case EShape::Mountain: return "Mountain";
		case EShape::River: return "River";
		default: return "Invalid";
	}
}

class SceneChapter7 final: public IScene
{
public:
	explicit SceneChapter7(framework::DxRenderManager& renderer);
	~SceneChapter7() override;

	bool Initialize() override;
	void Shutdown  () override;

	void FrameBegin	  (float deltaTime) override;
	void FrameEnd	  (float deltaTime) override;
	void ImguiView(float deltaTime) override;

private:
	void LoadData();
	void SaveData() const;

	//~ config creations
	void CreateAllocators();

	//~ Create resources
	void CreateShaders		 ();
	void CreateSRVHeap		 ();
	void CreateRootSignature ();
	void CreatePipeline		 ();
	void CreateGeometry		 ();
	void CreateRenderItems	 ();
	void CreateMountain		 ();
	void CreateRiver		 ();

	//~ helpers
	void ImguiMountainConfig();

	void CreateSingleGeometry(
			MeshGeometry& mesh,
			const MeshData& data
		) const;

	void CreateConstantBuffer(RenderItem& item);
	void UpdateConstantBuffer(float deltaTime);
	void DrawRenderItems();

private:
	HANDLE m_waitEvent;

	//~ Protect Swap chain buffer for wait free cycle
	std::vector<std::uint64_t> m_rtProtectedFenceValue;
	bool m_bAllocatorsInitialized{ false };
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_commandAllocators;
	std::uint64_t m_gpuIdleCount{ 0 };
	std::uint64_t m_cpuIdleCount{ 0 };

	//~ shader resources
	bool m_bShadersInitialized{ false };
	Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShaderBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShaderBlob;

	//~ Descriptor Heap for constant buffers
	bool m_bSRVHeapInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeapDescriptor;
	std::uint32_t m_allocated{ 0 };

	//~ Geometry Resources
	bool m_bGeometryInitialized{ false };
	std::unordered_map<EShape, MeshGeometry> m_geometries{};

	//~ render items
	bool m_bRenderItemInitialized{ false };
	std::uint32_t m_nCylinderCount{ 5u };
	std::uint32_t m_nSphereCount  { 5u };
	std::uint32_t m_nBoxCounts	  { 2u };
	std::unordered_map<EShape, std::vector<RenderItem>> m_renderItems;
	GenerateMountainConfig m_mountainConfig{};
	bool m_bMountainDirty{ true };

	//~ pipeline
	bool m_bRootSignatureInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature	{};
	bool m_bPipelineInitialized		{ false };
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline		{};

	//~ configs
	PassConstants m_globalPassConstant{};
	DirectX::XMFLOAT4X4 m_view{};
	DirectX::XMFLOAT4X4 m_proj{};
	float m_lastPrinted{ 5.f };
	float m_totalTime  { 0.0f };
};

#endif //DIRECTX12_SCENE_CHAPTER_7_H
