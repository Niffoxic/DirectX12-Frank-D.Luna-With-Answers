//
// Created by Niffoxic (Aka Harsh Dubey) on 12/29/2025.
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
#ifndef DIRECTX12_SCENE_TESTANIM_H
#define DIRECTX12_SCENE_TESTANIM_H

#include <DirectXMath.h>

#include "interface_scene.h"
#include "framework/animation/anim.h"
#include "utility/mesh_generator.h"
#include "framework/render_manager/components/render_item.h"
#include "framework/render_manager/components/decriptor_heap.h"
#include "framework/render_manager/components/pipeline.h"


class TestAnim final: public IScene
{
public:
	explicit TestAnim(framework::DxRenderManager& renderer);
	~TestAnim() override;

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

	void UpdateConstantBuffer(float deltaTime);

private:
	//~ anim tests
	foxanim::FxAnim m_anim{};

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
	framework::DescriptorHeap m_descriptorHeap{};

	//~ Geometry Resources
	bool m_bGeometryInitialized{ false };

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

#endif //DIRECTX12_SCENE_TESTANIM_H
