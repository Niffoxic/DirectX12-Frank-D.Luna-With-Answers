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
#include "application/scene/scene_chapter_7.h"

#include <ranges>

#include "framework/exception/dx_exception.h"
#include "framework/windows_manager/windows_manager.h"
#include "utility/helpers.h"
#include "utility/logger.h"
#include "utility/mesh_generator.h"
#include "utility/json_loader.h"

#include <imgui.h>

SceneChapter7::SceneChapter7(framework::DxRenderManager &renderer)
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

	m_mountainConfig.Width = 60.f;
	m_mountainConfig.Depth = 150.f;
	m_mountainConfig.SubdivisionsX = 500;
	m_mountainConfig.SubdivisionsZ = 500;
	m_mountainConfig.Falloff = 4.7f;
}

SceneChapter7::~SceneChapter7()
{
	SaveData();
}

bool SceneChapter7::Initialize()
{
	return true;
}

void SceneChapter7::Shutdown()
{
}

void SceneChapter7::FrameBegin(float deltaTime)
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
	CreateRenderItems	();
	CreateMountain		();

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

	DrawRenderItems();
}

void SceneChapter7::FrameEnd(float deltaTime)
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

void SceneChapter7::ImguiView(const float deltaTime)
{
	(void)deltaTime;

	m_descriptorHeap.ImguiView();

	constexpr EShape kShapes[] =
	{
		EShape::Sphere,
		EShape::Box,
		EShape::Cylinder,
		EShape::Mountain
	};

	for (EShape shape : kShapes)
	{
		auto it = m_renderItems.find(shape);
		if (it == m_renderItems.end() || it->second.empty())
			continue;

		auto& items = it->second;

		const std::string header =
			ToString(shape) + " (" + std::to_string(items.size()) + ")";

		ImGui::PushID(static_cast<int>(shape));

		if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent();

			for (size_t i = 0; i < items.size(); ++i)
			{
				auto& item = items[i];

				ImGui::PushID(static_cast<int>(i));
				item.ImguiView();
				ImGui::PopID();
			}

			ImGui::Unindent();
		}

		ImGui::PopID();
	}
}

void SceneChapter7::LoadData()
{
    JsonLoader loader{};
    loader.Load("save/chapter_7.json");
    if (!loader.IsValid()) return;

    constexpr EShape kShapes[] =
    {
        EShape::Sphere,
        EShape::Box,
        EShape::Cylinder,
        EShape::Mountain
    };

    for (EShape shape : kShapes)
    {
        const std::string shapeKey = ToString(shape);
        if (!loader.Contains(shapeKey)) continue;

        auto it = m_renderItems.find(shape);
        if (it == m_renderItems.end()) continue;

        auto& items = it->second;

        for (size_t i = 0; i < items.size(); ++i)
        {
            const std::string itemKey = "Item_" + std::to_string(i);
            if (!loader[shapeKey].Contains(itemKey)) continue;

            auto& item = items[i];

            if (loader[shapeKey][itemKey].Contains("Visible"))
                item.Visible = loader[shapeKey][itemKey]["Visible"].AsBool();

            if (loader[shapeKey][itemKey].Contains("Transform"))
            {
                auto& tr = item.Transform;

                if (loader[shapeKey][itemKey]["Transform"].Contains("Position"))
                {
                    tr.Position.x = loader[shapeKey][itemKey]["Transform"]["Position"]["X"].AsFloat();
                    tr.Position.y = loader[shapeKey][itemKey]["Transform"]["Position"]["Y"].AsFloat();
                    tr.Position.z = loader[shapeKey][itemKey]["Transform"]["Position"]["Z"].AsFloat();
                }

                if (loader[shapeKey][itemKey]["Transform"].Contains("Rotation"))
                {
                    tr.Rotation.x = loader[shapeKey][itemKey]["Transform"]["Rotation"]["X"].AsFloat();
                    tr.Rotation.y = loader[shapeKey][itemKey]["Transform"]["Rotation"]["Y"].AsFloat();
                    tr.Rotation.z = loader[shapeKey][itemKey]["Transform"]["Rotation"]["Z"].AsFloat();
                }

                if (loader[shapeKey][itemKey]["Transform"].Contains("Scale"))
                {
                    tr.Scale.x = loader[shapeKey][itemKey]["Transform"]["Scale"]["X"].AsFloat();
                    tr.Scale.y = loader[shapeKey][itemKey]["Transform"]["Scale"]["Y"].AsFloat();
                    tr.Scale.z = loader[shapeKey][itemKey]["Transform"]["Scale"]["Z"].AsFloat();
                }

                tr.MarkDirty();
            }
        }
    }
}

void SceneChapter7::SaveData() const
{
	JsonLoader saver{};

	constexpr EShape kShapes[] =
	{
		EShape::Sphere,
		EShape::Box,
		EShape::Cylinder,
		EShape::Mountain
	};

	for (EShape shape : kShapes)
	{
		const std::string shapeKey = ToString(shape);

		auto it = m_renderItems.find(shape);
		if (it == m_renderItems.end()) continue;

		const auto& items = it->second;

		for (size_t i = 0; i < items.size(); ++i)
		{
			const auto& item = items[i];
			const std::string itemKey = "Item_" + std::to_string(i);

			saver[shapeKey][itemKey]["Visible"] = item.Visible ? 1 : 0;

			saver[shapeKey][itemKey]["Transform"]["Position"]["X"] = item.Transform.Position.x;
			saver[shapeKey][itemKey]["Transform"]["Position"]["Y"] = item.Transform.Position.y;
			saver[shapeKey][itemKey]["Transform"]["Position"]["Z"] = item.Transform.Position.z;

			saver[shapeKey][itemKey]["Transform"]["Rotation"]["X"] = item.Transform.Rotation.x;
			saver[shapeKey][itemKey]["Transform"]["Rotation"]["Y"] = item.Transform.Rotation.y;
			saver[shapeKey][itemKey]["Transform"]["Rotation"]["Z"] = item.Transform.Rotation.z;

			saver[shapeKey][itemKey]["Transform"]["Scale"]["X"] = item.Transform.Scale.x;
			saver[shapeKey][itemKey]["Transform"]["Scale"]["Y"] = item.Transform.Scale.y;
			saver[shapeKey][itemKey]["Transform"]["Scale"]["Z"] = item.Transform.Scale.z;
		}
	}

	saver.Save("save/chapter_7.json");
}

void SceneChapter7::CreateAllocators()
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

void SceneChapter7::CreateShaders()
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

void SceneChapter7::CreateSRVHeap()
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

void SceneChapter7::CreateRootSignature()
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

void SceneChapter7::CreatePipeline()
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

void SceneChapter7::CreateGeometry()
{
	if (m_bGeometryInitialized) return;
	m_bGeometryInitialized = true;

	// Box
	{
		GenerateBoxConfig cfg{};
		cfg.Extents       = { 0.5f, 0.75f, 0.5f };
		cfg.Subdivisions  = 2;
		cfg.Color         = { 0.85f, 0.35f, 0.25f };
		cfg.GenerateTangents = true;
		cfg.FlipWinding   = false;
		cfg.InsideOut     = false;

		m_geometries[EShape::Box] = MeshGeometry{};
		const MeshData data = MeshGenerator::GenerateBox(cfg);
		m_geometries[EShape::Box].InitGeometryBuffer(
			Render.Device.Get(),
			Render.GfxCmd.Get(),
			data, false);
	}

	// Sphere
	{
		GenerateSphereConfig cfg{};
		cfg.Radius        = 0.6f;
		cfg.SliceCount    = 32;
		cfg.StackCount    = 24;
		cfg.Color         = { 0.25f, 0.65f, 0.35f };
		cfg.GenerateTangents = true;
		cfg.FlipWinding   = false;
		cfg.InsideOut     = false;

		m_geometries[EShape::Sphere] = MeshGeometry{};
		const MeshData data = MeshGenerator::GenerateSphere(cfg);
		m_geometries[EShape::Sphere].InitGeometryBuffer(
			Render.Device.Get(),
			Render.GfxCmd.Get(),
			data, false);
	}

	// Cylinder
	{
		GenerateCylinderConfig cfg{};
		cfg.BottomRadius  = 0.5f;
		cfg.TopRadius     = 0.25f;
		cfg.Height        = 1.5f;
		cfg.SliceCount    = 32;
		cfg.StackCount    = 4;
		cfg.CapTop        = true;
		cfg.CapBottom     = true;
		cfg.Color         = { 0.25f, 0.35f, 0.85f };
		cfg.GenerateTangents = true;
		cfg.FlipWinding   = false;
		cfg.InsideOut     = false;

		m_geometries[EShape::Cylinder] = MeshGeometry{};
		const MeshData data = MeshGenerator::GenerateCylinder(cfg);
		m_geometries[EShape::Cylinder].InitGeometryBuffer(
			Render.Device.Get(),
			Render.GfxCmd.Get(),
			data, false);
	}

	CreateMountain();
}

void SceneChapter7::CreateRenderItems()
{
	if (m_bRenderItemInitialized) return;
	m_bRenderItemInitialized = true;

	for (int i = 0; i < m_nBoxCounts; i++)
	{
		RenderItem item{};
		item.Mesh = &m_geometries[EShape::Box];
		item.InitConstantBuffer(
			Render.BackBufferCount,
			Render.Device.Get(),
			m_descriptorHeap);
		m_renderItems[EShape::Box].emplace_back(std::move(item));
	}
	m_nBoxCounts = 0;

	for (int i = 0; i < m_nCylinderCount; i++)
	{
		RenderItem item{};
		item.Mesh = &m_geometries[EShape::Cylinder];
		item.InitConstantBuffer(
			Render.BackBufferCount,
			Render.Device.Get(),
			m_descriptorHeap);
		m_renderItems[EShape::Cylinder].emplace_back(std::move(item));
	}
	m_nCylinderCount = 0;

	for (int i = 0; i < m_nSphereCount; i++)
	{
		RenderItem item{};
		item.Mesh = &m_geometries[EShape::Sphere];
		item.InitConstantBuffer(
			Render.BackBufferCount,
			Render.Device.Get(),
			m_descriptorHeap);
		m_renderItems[EShape::Sphere].emplace_back(std::move(item));
	}
	m_nSphereCount = 0;

	RenderItem item{};
	item.Mesh = &m_geometries[EShape::Mountain];
	item.InitConstantBuffer(
		Render.BackBufferCount,
		Render.Device.Get(),
		m_descriptorHeap);
	m_renderItems[EShape::Mountain].emplace_back(std::move(item));

	LoadData();
}

void SceneChapter7::CreateMountain()
{
	if (!m_bMountainDirty) return;
	m_bMountainDirty = false;

	m_geometries[EShape::Mountain] = MeshGeometry{};
	const MeshData data = MeshGenerator::GenerateMountain(m_mountainConfig);
	m_geometries[EShape::Mountain].InitGeometryBuffer(
		Render.Device.Get(),
		Render.GfxCmd.Get(),
		data, false);

	if (!m_renderItems[EShape::Mountain].empty())
	m_renderItems[EShape::Mountain][0].Mesh = &m_geometries[EShape::Mountain];
}

void SceneChapter7::CreateRiver()
{
}

void SceneChapter7::ImguiMountainConfig()
{
	bool changed = false;

	changed |= ImGui::DragFloat("Width",  &m_mountainConfig.Width,  0.1f,  0.1f, 10000.0f);
	changed |= ImGui::DragFloat("Depth",  &m_mountainConfig.Depth,  0.1f,  0.1f, 10000.0f);

	changed |= ImGui::DragInt("SubdivisionsX", reinterpret_cast<int*>(&m_mountainConfig.SubdivisionsX), 1.0f, 1, 2000);
	changed |= ImGui::DragInt("SubdivisionsZ", reinterpret_cast<int*>(&m_mountainConfig.SubdivisionsZ), 1.0f, 1, 2000);

	changed |= ImGui::DragFloat("HeightScale", &m_mountainConfig.HeightScale, 0.01f, 0.0f, 1000.0f);
	changed |= ImGui::DragFloat("Harshness",   &m_mountainConfig.Harshness,   0.01f, 0.1f,  20.0f);
	changed |= ImGui::DragFloat("Falloff",     &m_mountainConfig.Falloff,     0.01f, 0.0f,  50.0f);

	changed |= ImGui::DragFloat("Freq1", &m_mountainConfig.Freq1, 0.001f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Freq2", &m_mountainConfig.Freq2, 0.001f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Amp1",  &m_mountainConfig.Amp1,  0.01f,  0.0f, 1000.0f);
	changed |= ImGui::DragFloat("Amp2",  &m_mountainConfig.Amp2,  0.01f,  0.0f, 1000.0f);

	changed |= ImGui::Checkbox("GenerateTangents", &m_mountainConfig.GenerateTangents);
	changed |= ImGui::Checkbox("FlipWinding",      &m_mountainConfig.FlipWinding);
	changed |= ImGui::Checkbox("Centered",         &m_mountainConfig.Centered);

	changed |= ImGui::ColorEdit3("GroundGreen", &m_mountainConfig.GroundGreen.x);
	changed |= ImGui::ColorEdit3("GroundBrown", &m_mountainConfig.GroundBrown.x);
	changed |= ImGui::ColorEdit3("SnowColor",    &m_mountainConfig.SnowColor.x);

	changed |= ImGui::DragFloat("SnowStart", &m_mountainConfig.SnowStart, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("SnowBlend", &m_mountainConfig.SnowBlend, 0.01f, 0.001f, 1.0f);

	if (changed)
		m_bMountainDirty = true;
}

void SceneChapter7::UpdateConstantBuffer(const float deltaTime)
{
	m_globalPassConstant.DeltaTime = deltaTime;
	m_globalPassConstant.TotalTime = m_totalTime;

	for (auto &vec: m_renderItems | std::views::values)
	{
		for (auto& renderItem : vec)
		{
			const auto index = renderItem.FrameIndex;
			PerObjectConstantsCPU per{};
			const auto world = renderItem.Transform.GetTransform();

			DirectX::XMStoreFloat4x4(
				&per.World,
				DirectX::XMMatrixTranspose(
					DirectX::XMLoadFloat4x4(&world)
				)
			);

			BYTE* dst = renderItem.PerObject.Mapped[index];
			std::memcpy(dst, &per, sizeof(per));

			dst = renderItem.PassConstant.Mapped[index];
			std::memcpy(dst, &m_globalPassConstant, sizeof(m_globalPassConstant));
		}
	}
}

void SceneChapter7::DrawRenderItems()
{
	ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.GetNative() };
	Render.GfxCmd->SetDescriptorHeaps(_countof(heaps), heaps);

	Render.GfxCmd->SetGraphicsRootSignature(m_rootSignature.Get());
	Render.GfxCmd->SetPipelineState(m_pipeline.GetNative());

	for (auto &items: m_renderItems | std::views::values)
	{
		for (auto& item : items)
		{
			const std::uint32_t index = item.FrameIndex;

			if (item.Visible)
			{
				Render.GfxCmd->SetGraphicsRootDescriptorTable(
						0u, item.BaseCBHandle[index]);
				const auto prim = GetTopologyType(item.PrimitiveMode);
				Render.GfxCmd->IASetPrimitiveTopology(prim);
				Render.GfxCmd->IASetIndexBuffer(&item.Mesh->IndexViews);
				Render.GfxCmd->IASetVertexBuffers(0u, item.Mesh->VertexViews.size(),
					item.Mesh->VertexViews.data());

				Render.GfxCmd->DrawIndexedInstanced(
						item.Mesh->IndexCount,
						1u, item.Mesh->StartIndexLocation,
						item.Mesh->BaseVertexLocation,
						0u
					);
			}

			item.FrameIndex = (index + 1u) % Render.BackBufferCount;
		}
	}
}
