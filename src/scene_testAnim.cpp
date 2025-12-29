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
#include "application/scene/scene_testAnim.h"

#include <ranges>

#include "framework/exception/dx_exception.h"
#include "framework/windows_manager/windows_manager.h"
#include "utility/helpers.h"
#include "utility/logger.h"
#include "utility/mesh_generator.h"
#include "utility/json_loader.h"

#include <imgui.h>

TestAnim::TestAnim(framework::DxRenderManager &renderer)
	: IScene(renderer)
{
	m_waitEvent = CreateEvent(nullptr,
		FALSE, FALSE,
		nullptr);

	m_rtProtectedFenceValue.resize(framework::DxRenderManager::BackBufferCount);

	// View
	{
		const DirectX::XMVECTOR eye    = DirectX::XMVectorSet(0.f, 0.f, -10.f, 1.f);
		const DirectX::XMVECTOR target = DirectX::XMVectorZero();
		const DirectX::XMVECTOR up     = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);

		DirectX::XMStoreFloat4x4(
			&m_view,
			DirectX::XMMatrixLookAtLH(eye, target, up)
		);
	}

	// Projection
	{
		constexpr float fovY   = DirectX::XMConvertToRadians(60.0f);
		const float aspect = static_cast<float>(Render.Windows->GetWindowsWidth()) /
							 static_cast<float>(Render.Windows->GetWindowsHeight());

		constexpr float zNear  = 0.1f;
		constexpr float zFar   = 1000.0f;

		DirectX::XMStoreFloat4x4(
			&m_proj,
			DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, zNear, zFar)
		);
	}
	//~ set global constant pass
	{
		using namespace DirectX;

		const XMMATRIX V  = XMLoadFloat4x4(&m_view);
		const XMMATRIX P  = XMLoadFloat4x4(&m_proj);
		const XMMATRIX VP = XMMatrixMultiply(V, P);

		const XMMATRIX invV  = XMMatrixInverse(nullptr, V);
		const XMMATRIX invP  = XMMatrixInverse(nullptr, P);
		const XMMATRIX invVP = XMMatrixInverse(nullptr, VP);

		// Store
		XMStoreFloat4x4(&m_globalPassConstant.View,              XMMatrixTranspose(V));
		XMStoreFloat4x4(&m_globalPassConstant.InvView,           XMMatrixTranspose(invV));
		XMStoreFloat4x4(&m_globalPassConstant.Projection,        XMMatrixTranspose(P));
		XMStoreFloat4x4(&m_globalPassConstant.InvProjection,     XMMatrixTranspose(invP));
		XMStoreFloat4x4(&m_globalPassConstant.ViewProjection,    XMMatrixTranspose(VP));
		XMStoreFloat4x4(&m_globalPassConstant.InvViewProjection, XMMatrixTranspose(invVP));

		// Camera position (match your eye above)
		m_globalPassConstant.EyePositionW = XMFLOAT3(0.f, 0.f, -10.f);
		m_globalPassConstant.padding = 0.0f;

		const float w = static_cast<float>(Render.Windows->GetWindowsWidth());
		const float h = static_cast<float>(Render.Windows->GetWindowsHeight());

		m_globalPassConstant.RenderTargetSize     = XMFLOAT2(w, h);
		m_globalPassConstant.InvRenderTargetSize  = XMFLOAT2(1.0f / w, 1.0f / h);

		m_globalPassConstant.NearZ = 0.1f;
		m_globalPassConstant.FarZ  = 1000.0f;

		m_globalPassConstant.TotalTime = 0.0f;
		m_globalPassConstant.DeltaTime = 0.0f;
	}

}

TestAnim::~TestAnim()
{}

bool TestAnim::Initialize()
{
	m_anim.Test();
	return true;
}

void TestAnim::Shutdown()
{
}

void TestAnim::FrameBegin(float deltaTime)
{
	m_totalTime += deltaTime;

	//~ Create resources
	CreateAllocators();
	CreateSRVHeap	();

	//~ Frame Clear
	ResetEvent(m_waitEvent);

	const UINT fi = Render.FrameIndex;
	const UINT64 fenceToWaitFor = m_rtProtectedFenceValue[fi];

	if (Render.Fence->GetCompletedValue() < fenceToWaitFor)
	{
		THROW_DX_IF_FAILS(Render.Fence->SetEventOnCompletion(fenceToWaitFor, m_waitEvent));
		++m_cpuIdleCount;
		WaitForSingleObject(m_waitEvent, INFINITE);
	}else ++m_gpuIdleCount;

	if (m_lastPrinted <= 0.0f)
	{
		m_lastPrinted = 5.0f;
		logger::info("GPU Idle count: {}, CPU Idle Count: {}",
			m_gpuIdleCount, m_cpuIdleCount);
		m_gpuIdleCount = 0;
		m_cpuIdleCount = 0;
	}else m_lastPrinted -= deltaTime;

	if (m_pipeline.IsInitialized() && m_pipeline.IsDirty())
	{
		m_pipeline.Initialize(&Render);
	}

	auto* alloc = m_commandAllocators[fi].Get();
	THROW_DX_IF_FAILS(alloc->Reset());
	THROW_DX_IF_FAILS(Render.GfxCmd->Reset(alloc,
			m_pipeline.GetNative()));

	//~ Create Render Resources
	CreateShaders		();
	CreateRootSignature ();
	CreatePipeline		();
	CreateGeometry		();

	UpdateConstantBuffer(deltaTime);

	//~ render target infos
	const auto rtvHandle		= Render.GetBackBufferHandle(Render.FrameIndex);
	const auto dsvHandle		= Render.GetDSVBaseHandle();
	auto* mainRT	= Render.GetBackBuffer(Render.FrameIndex);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource   = mainRT;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	Render.GfxCmd->ResourceBarrier(1, &barrier);

	Render.GfxCmd->RSSetViewports(1u, &Render.Viewport);
	Render.GfxCmd->RSSetScissorRects(1u, &Render.ScissorRect);

	Render.GfxCmd->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH |
		D3D12_CLEAR_FLAG_STENCIL, 1.f, 0u, 0u, nullptr);

	constexpr float color[4]{0.f, 0.f, 0.f, 1.f};
	Render.GfxCmd->ClearRenderTargetView(rtvHandle, color, 0u, nullptr);

	Render.GfxCmd->OMSetRenderTargets(1u,
		&rtvHandle, TRUE,
		&dsvHandle);

	//~ draw
}

void TestAnim::FrameEnd(float deltaTime)
{
	const auto frameIndex = Render.FrameIndex;
	auto* mainRT	 = Render.GetBackBuffer(frameIndex);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource   = mainRT;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
	Render.GfxCmd->ResourceBarrier(1u, &barrier);

	THROW_DX_IF_FAILS(Render.GfxCmd->Close());
	ID3D12CommandList* cmdLists[] = { Render.GfxCmd.Get() };
	Render.GfxQueue->ExecuteCommandLists(1u, cmdLists);

	THROW_DX_IF_FAILS(Render.SwapChain->Present(0u, 0u));

	//~ Protect by fences
	THROW_DX_IF_FAILS(Render.GfxQueue->Signal(Render.Fence.Get(), Render.FenceValue));

	m_rtProtectedFenceValue[frameIndex] = Render.FenceValue; //~ Cache last attached fence

	Render.IncrementFenceValue();
	Render.IncrementFrameIndex();
}

void TestAnim::ImguiView(float deltaTime)
{
}

void TestAnim::LoadData()
{
}

void TestAnim::SaveData() const
{
}

void TestAnim::CreateAllocators()
{
	if (m_bAllocatorsInitialized) return;
	m_bAllocatorsInitialized = true;

	for (int i = 0; i < Render.BackBufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> alloc;
		THROW_DX_IF_FAILS(Render.Device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&alloc)));
		m_commandAllocators.emplace_back(std::move(alloc));
	}

	logger::success("Created Command Allocations Count: {}", Render.BackBufferCount);
}

void TestAnim::CreateShaders()
{
	if (m_bShadersInitialized) return;
	m_bShadersInitialized = true;

	const std::string vertexPath = "shaders/chapter_7/vertex_shader.hlsl";
	const auto wVP = std::wstring(vertexPath.begin(), vertexPath.end());
	const std::string pixelPath  = "shaders/chapter_7/pixel_shader.hlsl";
	const auto wPP = std::wstring(pixelPath.begin(), pixelPath.end());

	if (!helpers::IsFile(vertexPath) || !helpers::IsFile(pixelPath))
	{
		THROW_MSG("Either Vertex or Pixel Path is not valid!");
	}

	m_vertexShaderBlob = framework::DxRenderManager::CompileShader(
			wVP, nullptr, "main", "vs_5_0");

	m_pixelShaderBlob = framework::DxRenderManager::CompileShader(
		wPP, nullptr, "main", "ps_5_0");

	logger::success("Compiled Shader Resources!");
}

void TestAnim::CreateSRVHeap()
{
	if (m_bSRVHeapInitialized) return;
	m_bSRVHeapInitialized = true;

	framework::InitDescriptorHeap desc{};
	desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.AllocationSize = 64u;
	desc.pDevice		= Render.Device.Get();
	m_descriptorHeap.Initialize(desc);

	logger::success("Created Descriptor Heap Descriptor!");
}

void TestAnim::CreateRootSignature()
{
	if (m_bRootSignatureInitialized) return;
	m_bRootSignatureInitialized = true;

	D3D12_DESCRIPTOR_RANGE range{};
	range.NumDescriptors					= 2u;
	range.BaseShaderRegister				= 0u;
	range.RegisterSpace						= 0u;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	range.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

	D3D12_ROOT_PARAMETER param[1]{};
	param[0].ParameterType						 = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[0].ShaderVisibility					 = D3D12_SHADER_VISIBILITY_VERTEX;
	param[0].DescriptorTable.NumDescriptorRanges = 1u;
	param[0].DescriptorTable.pDescriptorRanges	 = &range;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = 1u;
	rootSignatureDesc.pParameters = param;
	rootSignatureDesc.NumStaticSamplers = 0u;
	rootSignatureDesc.pStaticSamplers = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	THROW_DX_IF_FAILS(D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		sigBlob.GetAddressOf(), &errorBlob
	));

	THROW_DX_IF_FAILS(Render.Device->CreateRootSignature(
			0u, sigBlob->GetBufferPointer(),
			sigBlob->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature)
		));
}

void TestAnim::CreatePipeline()
{
	if (m_bPipelineInitialized) return;
	m_bPipelineInitialized = true;

	m_pipeline.SetRootSignature(m_rootSignature.Get());

	m_pipeline.SetVertexShader(D3D12_SHADER_BYTECODE{
		m_vertexShaderBlob->GetBufferPointer(),
		m_vertexShaderBlob->GetBufferSize()
	});

	m_pipeline.SetPixelShader(D3D12_SHADER_BYTECODE{
		m_pixelShaderBlob->GetBufferPointer(),
		m_pixelShaderBlob->GetBufferSize()
	});

	m_pipeline.SetInputLayout(MeshVertex::GetInputLayout());

	m_pipeline.SetFillMode(EFillMode::Solid);
	m_pipeline.SetCullMode(ECullMode::None);

	m_pipeline.Initialize(&Render);
}

void TestAnim::CreateGeometry()
{
}

void TestAnim::UpdateConstantBuffer(float deltaTime)
{
	m_globalPassConstant.DeltaTime = deltaTime;
	m_globalPassConstant.TotalTime = m_totalTime;

}
