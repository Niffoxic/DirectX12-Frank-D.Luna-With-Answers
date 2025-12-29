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

#ifndef DIRECTX12_SCENE_CHAPTER_8_H
#define DIRECTX12_SCENE_CHAPTER_8_H

#include "interface_scene.h"
#include "framework/render_manager/components/decriptor_heap.h"
#include "framework/render_manager/components/pipeline.h"
#include "framework/render_manager/components/render_item.h"

#include <cstdint>

enum class ERenderType: std::uint32_t
{
	River,
};

inline std::string ToString(const ERenderType &type)
{
	switch (type)
	{
		case ERenderType::River: return "River";
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

class SceneChapter8 final: public IScene
{
public:
	SceneChapter8(framework::DxRenderManager& renderer);
	~SceneChapter8() override;

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

	void UpdateConstantBuffer(float deltaTime);
	void DrawRenderItems();

	//~ update river
	void UpdateRiver(float deltaTime);

private:
    //~ common
    HANDLE m_waitEvent;
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
	framework::DescriptorHeap m_descriptorHeap{};

	//~ Geometry Resources
	bool m_bGeometryInitialized{ false };
	std::unordered_map<ERenderType, MeshGeometry> m_geometries{};

	//~ river
	MeshData m_riverBase;
	MeshData m_riverFrame;
	RiverUpdateParam m_riverParam{};
	float m_riverUpdateAccum = 0.0f;

    //~ render items
    bool m_bRenderItemInitialized{ false };
    std::unordered_map<ERenderType, std::vector<RenderItem>> m_renderItems;

    //~ pipeline
	bool m_bRootSignatureInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature	{};
	bool m_bPipelineInitialized		{ false };
	framework::Pipeline m_pipeline	{};

    //~ configs
	PassConstantsCPU m_globalPassConstant{};
	DirectX::XMFLOAT4X4 m_view{};
	DirectX::XMFLOAT4X4 m_proj{};
	float m_lastPrinted{ 5.f };
	float m_totalTime  { 0.0f };

};

#endif //DIRECTX12_SCENE_CHAPTER_8_H
