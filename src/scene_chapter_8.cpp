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
#include "application/scene/scene_chapter_8.h"

#include <algorithm>

#include "framework/exception/dx_exception.h"
#include "framework/windows_manager/windows_manager.h"
#include "utility/helpers.h"
#include "utility/logger.h"

#include <ranges>
#include <thread>

#include "imgui.h"
#include "utility/json_loader.h"

SceneChapter8::SceneChapter8(framework::DxRenderManager &renderer)
	: IScene(renderer)
{
    m_waitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
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

SceneChapter8::~SceneChapter8()
{
	SaveData();
}

bool SceneChapter8::Initialize()
{
	return true;
}

void SceneChapter8::Shutdown()
{
}

void SceneChapter8::FrameBegin(float deltaTime)
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
	CreateMaterials		();

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

	constexpr float color[4]{0.2f, 0.1f, 0.3f, 1.f};
	Render.GfxCmd->ClearRenderTargetView(rtvHandle, color, 0u, nullptr);

	Render.GfxCmd->OMSetRenderTargets(1u,
		&rtvHandle, TRUE,
		&dsvHandle);

	UpdateRiver(deltaTime);
	DrawRenderItems();
}

void SceneChapter8::FrameEnd(float deltaTime)
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

void SceneChapter8::ImguiView(const float deltaTime)
{
	(void)deltaTime;

	m_lightManager  .ImguiView();
	m_descriptorHeap.ImguiView();

	constexpr ERenderType kShapes[] =
	{
		ERenderType::River,
		ERenderType::Mountain
	};

	m_riverParam.ImguiView();

	for (ERenderType shape : kShapes)
	{
		//~ update pixel config
		if (m_materials.contains(shape))
		{
			m_materials[shape].ImguiView();
		}else THROW_MSG("LOGIC ERROR MATERIAL ISN'T THERE!");

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

void SceneChapter8::LoadData()
{
	JsonLoader loader{};
	loader.Load("save/chapter_8.json");
	if (!loader.IsValid()) return;

	if (loader.Contains("RiverParam"))
	{
		auto& rp = loader["RiverParam"];
		auto& p  = m_riverParam;

		auto ReadFloat = [&](const char* k, float& v)
		{
			if (rp.Contains(k)) v = rp[k].AsFloat();
		};

		auto ReadInt = [&](const char* k, int& v)
		{
			if (rp.Contains(k)) v = rp[k].AsInt();
		};

		auto ReadColor3 = [&](const char* k, DirectX::XMFLOAT3& c)
		{
			if (!rp.Contains(k)) return;
			if (rp[k].Contains("X")) c.x = rp[k]["X"].AsFloat();
			if (rp[k].Contains("Y")) c.y = rp[k]["Y"].AsFloat();
			if (rp[k].Contains("Z")) c.z = rp[k]["Z"].AsFloat();
		};

		ReadFloat("Amp1", p.amp1);
		ReadFloat("Amp2", p.amp2);
		ReadFloat("Freq1", p.freq1);
		ReadFloat("Freq2", p.freq2);
		ReadFloat("WaveLen1", p.waveLen1);
		ReadFloat("WaveLen2", p.waveLen2);
		ReadFloat("FlowSpeed", p.flowSpeed);

		ReadFloat("HalfWidth", p.halfWidth);
		ReadFloat("MinZ", p.minZ);
		ReadFloat("MaxZ", p.maxZ);

		ReadColor3("LeftColor", p.leftColor);
		ReadColor3("RightColor", p.rightColor);
		ReadColor3("DownLeftColor", p.downLeftColor);
		ReadColor3("DownRightColor", p.downRightColor);

		ReadColor3("ShallowColor", p.shallowColor);
		ReadColor3("DeepColor", p.deepColor);
		ReadColor3("FoamColor", p.foamColor);

		ReadFloat("FoamStrength", p.foamStrength);
		ReadFloat("ShimmerStrength", p.shimmerStrength);

		ReadFloat("EdgeNoiseStrength", p.edgeNoiseStrength);
		ReadFloat("OctaveBaseAmp", p.octaveBaseAmp);
		ReadFloat("OctaveBaseFreq", p.octaveBaseFreq);
		ReadFloat("OctaveBaseWaveLen", p.octaveBaseWaveLen);
		ReadInt("Octaves", p.octaves);

		ReadFloat("HeightScale", p.heightScale);
		ReadFloat("HeightBias", p.heightBias);
		ReadFloat("MaxHeight", p.maxHeight);
		ReadFloat("FoamHeightThreshold", p.foamHeightThreshold);

		p.octaves = std::clamp(p.octaves, 1, 8);
	}

	constexpr ERenderType kShapes[] =
	{
		ERenderType::River,
	};

	for (ERenderType shape : kShapes)
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

void SceneChapter8::SaveData() const
{
	JsonLoader saver{};

	{
		auto& rp = saver["RiverParam"];
		const auto& p = m_riverParam;

		auto WriteColor3 = [&](const char* k, const DirectX::XMFLOAT3& c)
		{
			rp[k]["X"] = c.x;
			rp[k]["Y"] = c.y;
			rp[k]["Z"] = c.z;
		};

		rp["Amp1"] = p.amp1;
		rp["Amp2"] = p.amp2;
		rp["Freq1"] = p.freq1;
		rp["Freq2"] = p.freq2;
		rp["WaveLen1"] = p.waveLen1;
		rp["WaveLen2"] = p.waveLen2;
		rp["FlowSpeed"] = p.flowSpeed;

		rp["HalfWidth"] = p.halfWidth;
		rp["MinZ"] = p.minZ;
		rp["MaxZ"] = p.maxZ;

		WriteColor3("LeftColor", p.leftColor);
		WriteColor3("RightColor", p.rightColor);
		WriteColor3("DownLeftColor", p.downLeftColor);
		WriteColor3("DownRightColor", p.downRightColor);

		WriteColor3("ShallowColor", p.shallowColor);
		WriteColor3("DeepColor", p.deepColor);
		WriteColor3("FoamColor", p.foamColor);

		rp["FoamStrength"] = p.foamStrength;
		rp["ShimmerStrength"] = p.shimmerStrength;

		rp["EdgeNoiseStrength"] = p.edgeNoiseStrength;
		rp["OctaveBaseAmp"] = p.octaveBaseAmp;
		rp["OctaveBaseFreq"] = p.octaveBaseFreq;
		rp["OctaveBaseWaveLen"] = p.octaveBaseWaveLen;
		rp["Octaves"] = p.octaves;

		rp["HeightScale"] = p.heightScale;
		rp["HeightBias"] = p.heightBias;
		rp["MaxHeight"] = p.maxHeight;
		rp["FoamHeightThreshold"] = p.foamHeightThreshold;
	}

	constexpr ERenderType kShapes[] =
	{
		ERenderType::River,
	};

	for (ERenderType shape : kShapes)
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

	saver.Save("save/chapter_8.json");
}

void SceneChapter8::CreateAllocators()
{
	if (m_bAllocatorsInitialized) return;
	m_bAllocatorsInitialized = true;

	for (int i = 0; i < framework::DxRenderManager::BackBufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> alloc{ nullptr };
		THROW_DX_IF_FAILS(Render.Device->CreateCommandAllocator(
						D3D12_COMMAND_LIST_TYPE_DIRECT,
						IID_PPV_ARGS(&alloc)));

		m_commandAllocators.emplace_back(std::move(alloc));
	}
}

void SceneChapter8::CreateShaders()
{
	if (m_bShadersInitialized) return;
	m_bShadersInitialized = true;

	const std::string vertexPath = "shaders/chapter_7/vertex_shader.hlsl";
	const auto wVP = std::wstring(vertexPath.begin(), vertexPath.end());
	const std::string pixelPath  = "shaders/chapter_8/pixel_shader.hlsl";
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

void SceneChapter8::CreateSRVHeap()
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

void SceneChapter8::CreateRootSignature()
{
	if (m_bRootSignatureInitialized) return;
	m_bRootSignatureInitialized = true;

	D3D12_DESCRIPTOR_RANGE range{};
	range.NumDescriptors					= 2u;
	range.BaseShaderRegister				= 0u;
	range.RegisterSpace						= 0u;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	range.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

	D3D12_DESCRIPTOR_RANGE psRange{};
	psRange.NumDescriptors					    = 1u;
	psRange.BaseShaderRegister				    = 2u;
	psRange.RegisterSpace						= 0u;
	psRange.OffsetInDescriptorsFromTableStart   = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	psRange.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

	D3D12_ROOT_PARAMETER param[2]{};
	param[0].ParameterType						 = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[0].ShaderVisibility					 = D3D12_SHADER_VISIBILITY_ALL;
	param[0].DescriptorTable.NumDescriptorRanges = 1u;
	param[0].DescriptorTable.pDescriptorRanges	 = &range;

	param[1].ParameterType						 = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[1].ShaderVisibility					 = D3D12_SHADER_VISIBILITY_PIXEL;
	param[1].DescriptorTable.NumDescriptorRanges = 1u;
	param[1].DescriptorTable.pDescriptorRanges	 = &psRange;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters		= 2u;
	rootSignatureDesc.pParameters	    = param;
	rootSignatureDesc.NumStaticSamplers = 0u;
	rootSignatureDesc.pStaticSamplers	= nullptr;

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

void SceneChapter8::CreatePipeline()
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

void SceneChapter8::CreateGeometry()
{
	if (m_bGeometryInitialized) return;
	m_bGeometryInitialized = true;

	{ // river
		GenerateGridConfig cfg{};
		cfg.Width           = 120.0f;
		cfg.Depth           = 60.0f;
		cfg.SubdivisionsX   = 480;
		cfg.SubdivisionsZ   = 240;
		cfg.Centered        = true;
		cfg.GenerateTangents = true;
		cfg.Color           = DirectX::XMFLOAT3(0.05f, 0.25f, 0.35f);

		m_riverBase  = MeshGenerator::GenerateGrid(cfg);
		m_riverFrame = m_riverBase;

		m_geometries[ERenderType::River] = MeshGeometry{};
		m_geometries[ERenderType::River].InitGeometryBuffer(
			Render.Device.Get(),
			Render.GfxCmd.Get(),
			m_riverBase,
			true
		);
	}
	// mountain
	{
		GenerateMountainConfig cfg{};
		cfg.Width = 60.f;
		cfg.Depth = 150.f;
		cfg.SubdivisionsX = 500;
		cfg.SubdivisionsZ = 500;
		cfg.Falloff = 4.7f;

		m_geometries[ERenderType::Mountain] = MeshGeometry{};
		const MeshData data = MeshGenerator::GenerateMountain(cfg);
		m_geometries[ERenderType::Mountain].InitGeometryBuffer(
			Render.Device.Get(),
			Render.GfxCmd.Get(),
			data, false);

		if (!m_renderItems[ERenderType::Mountain].empty())
			m_renderItems[ERenderType::Mountain][0].Mesh = &m_geometries[ERenderType::Mountain];
	}
}

void SceneChapter8::CreateRenderItems()
{
	if (m_bRenderItemInitialized) return;
	m_bRenderItemInitialized = true;

	{
		RenderItem item{};
		item.Mesh = &m_geometries[ERenderType::River];
		item.InitConstantBuffer(
			framework::DxRenderManager::BackBufferCount,
			Render.Device.Get(),
			m_descriptorHeap);
		m_renderItems[ERenderType::River].emplace_back(std::move(item));
	}

	{
		RenderItem item{};
		item.Mesh = &m_geometries[ERenderType::Mountain];
		item.InitConstantBuffer(
			framework::DxRenderManager::BackBufferCount,
			Render.Device.Get(),
			m_descriptorHeap);
		m_renderItems[ERenderType::Mountain].emplace_back(std::move(item));
	}

	LoadData();
}

void SceneChapter8::CreateMaterials()
{
	if (m_bMaterialsInitialized) return;
	m_bMaterialsInitialized = true;

	//~ grass
	auto& grass = m_materials[ERenderType::Mountain];
	grass.Name			 = "grass";
	grass.FrameCount = framework::DxRenderManager::BackBufferCount;

	grass.InitPixelConstantBuffer(framework::DxRenderManager::BackBufferCount,
		Render.Device.Get(),
		m_descriptorHeap);

	auto& water = m_materials[ERenderType::River];
	water.Name = "water";
	water.Config.DiffuseAlbedo = { 0.0f, 0.2f, 0.6f, 1.0f };
	water.Config.Roughness = 0.f;
	water.FrameCount = framework::DxRenderManager::BackBufferCount;

	water.InitPixelConstantBuffer(framework::DxRenderManager::BackBufferCount,
		Render.Device.Get(),
		m_descriptorHeap);
}

void SceneChapter8::UpdateConstantBuffer(const float deltaTime)
{
	m_globalPassConstant.DeltaTime = deltaTime;
	m_globalPassConstant.TotalTime = m_totalTime;

	m_lightManager.FillPassConstants(m_globalPassConstant);

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

	//~ update material constant buffer
	for (auto& [name, mat] : m_materials)
	{
		const auto index = mat.FrameIndex;
		BYTE* dst				= mat.PixelConstantMap.Mapped[index];
		std::memcpy(dst, &mat.Config, sizeof(mat.Config));
	}
}

void SceneChapter8::DrawRenderItems()
{
	ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.GetNative() };
	Render.GfxCmd->SetDescriptorHeaps(_countof(heaps), heaps);

	Render.GfxCmd->SetGraphicsRootSignature(m_rootSignature.Get());
	Render.GfxCmd->SetPipelineState(m_pipeline.GetNative());

	for (auto &[type, items]: m_renderItems)
	{
		for (auto& item : items)
		{
			const std::uint32_t index = item.FrameIndex;

			if (item.Visible)
			{
				//~ constant buffer vertex
				Render.GfxCmd->SetGraphicsRootDescriptorTable(
						0u, item.BaseCBHandle[index]);

				//~ constant buffer pixel
				//~ set material based on type
				const auto matGpuHandle = m_materials[type].PixelConstantMap.GPUHandle[index];
				Render.GfxCmd->SetGraphicsRootDescriptorTable(1u, matGpuHandle);

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
				m_materials[type].FrameIndex = (index + 1u) % framework::DxRenderManager::BackBufferCount;
				item.FrameIndex				 = (index + 1u) % framework::DxRenderManager::BackBufferCount;
			}
		}
	}
}

void SceneChapter8::UpdateRiver(const float deltaTime)
{
	constexpr float kHz		   = 24.0f;
	constexpr float kStep	   = 1.0f / kHz;
	constexpr int   kMaxCatchUp = 2;

	m_riverUpdateAccum += deltaTime;

	if (m_riverUpdateAccum < kStep)
		return;

	int steps = 0;
	while (m_riverUpdateAccum >= kStep && steps < kMaxCatchUp)
	{
		m_riverUpdateAccum -= kStep;
		++steps;
	}

	const float t = m_totalTime;

	auto& rivers = m_renderItems[ERenderType::River];
	if (rivers.empty() || m_riverBase.vertices.empty())
		return;

	const auto& p = m_riverParam;

	for (auto& river : rivers)
	{
		if (!river.Visible || !river.Mesh)
			continue;

		auto* geo = river.Mesh;
		if (!geo->Mapped)
			continue;

		geo->Data = m_riverBase;

		const size_t count = geo->Data.vertices.size();
		if (count == 0)
			continue;

		uint32_t tc = std::thread::hardware_concurrency();
		if (tc == 0) tc = 4;
		tc = std::min<uint32_t>(tc, 8);
		if (count < 4096) tc = 1;

		const size_t chunk = (count + tc - 1) / tc;

		auto worker = [&](size_t begin, size_t end)
		{
			for (size_t i = begin; i < end; ++i)
			{
				auto& v = geo->Data.vertices[i];
				const auto& b = m_riverBase.vertices[i];

				const float x = b.Position.x;
				const float z = b.Position.z;

				float bank = 1.0f;
				if (p.halfWidth > 0.0001f)
				{
					const float ax = std::abs(x);
					bank = 1.0f - (ax / p.halfWidth);
					bank = (bank < 0.0f) ? 0.0f : bank;
					bank *= bank;
				}

				float mask = bank;
				if (p.edgeNoiseStrength > 0.0f)
				{
					const float edge = 1.0f - bank;
					mask = bank + edge * p.edgeNoiseStrength;
				}

				float height = 0.0f;

				{
					const float w1 = std::sinf((z * p.waveLen1) + (t * p.freq1) + (x * 0.15f));
					const float w2 = std::sinf((z * p.waveLen2) - (t * p.freq2) + (x * 0.40f));
					const float flow = std::sinf((z * 0.55f) + (t * p.flowSpeed));
					height += (p.amp1 * w1 + p.amp2 * w2) * bank + (0.015f * flow * bank);
				}

				{
					float a  = p.octaveBaseAmp;
					float f  = p.octaveBaseFreq;
					float wl = p.octaveBaseWaveLen;

					for (int o = 0; o < p.octaves; ++o)
					{
						const float phase = float(o) * 13.37f;
						const float r1 = std::sinf((z * wl * f) + (t * f) + (x * 0.31f) + phase);
						const float r2 = std::sinf((x * wl * 0.75f * f) - (t * 1.35f * f) + (z * 0.17f) + phase * 0.7f);
						height += (r1 * 0.65f + r2 * 0.35f) * a * mask;

						a *= 0.55f;
						f *= 1.85f;
						wl *= 1.15f;
					}
				}

				height = (height * p.heightScale) + p.heightBias;
				height = std::clamp(height, -p.maxHeight, p.maxHeight);

				v.Position.y = b.Position.y + height;
				v.Position.x = b.Position.x + (0.01f * bank * std::sinf((z * 0.6f) + t * 1.2f));

				const float x01 = (p.halfWidth > 0.0001f)
					? std::clamp((x / (p.halfWidth * 2.0f)) + 0.5f, 0.0f, 1.0f)
					: 0.5f;

				const float zDen = (p.maxZ - p.minZ);
				const float z01 = (std::abs(zDen) > 0.0001f)
					? std::clamp((z - p.minZ) / zDen, 0.0f, 1.0f)
					: 0.5f;

				const auto top  = Lerp(p.leftColor,     p.rightColor,     x01);
				const auto bot  = Lerp(p.downLeftColor, p.downRightColor, x01);
				const auto quad = Lerp(bot, top, z01);

				const float denom = (p.maxHeight > 0.0001f) ? p.maxHeight : 0.0001f;
				float h01 = height / denom;
				h01 = std::clamp(h01 * 0.5f + 0.5f, 0.0f, 1.0f);

				const auto depthTint = Lerp(p.deepColor, p.shallowColor, h01);

				float crest = (height - p.foamHeightThreshold) / (p.maxHeight - p.foamHeightThreshold + 0.0001f);
				crest = std::clamp(crest, 0.0f, 1.0f);
				float foam = std::clamp((crest * crest) * p.foamStrength, 0.0f, 1.0f);

				const auto foamed = Lerp(depthTint, p.foamColor, foam);

				const float shimmer = p.shimmerStrength * std::sinf((z * 0.8f) + t * p.flowSpeed) * mask;
				const DirectX::XMFLOAT3 shimmer3{ shimmer, shimmer, shimmer };

				v.Color = Clamp01(Add3(Mul3(quad, foamed), shimmer3));
			}
		};

		if (tc == 1)
		{
			worker(0, count);
		}
		else
		{
			std::vector<std::jthread> threads;
			threads.reserve(tc);

			for (uint32_t ti = 0; ti < tc; ++ti)
			{
				const size_t begin = size_t(ti) * chunk;
				const size_t end   = std::min(begin + chunk, count);
				if (begin >= end) break;

				threads.emplace_back([&, begin, end]()
				{
					worker(begin, end);
				});
			}
		}

		MeshGenerator::ComputeNormals(geo->Data);
		for (auto& v : geo->Data.vertices)
		{
			v.Normal.x = -v.Normal.x;
			v.Normal.y = -v.Normal.y;
			v.Normal.z = -v.Normal.z;
		}

		const auto vbSize = static_cast<uint32_t>(sizeof(MeshVertex) * geo->Data.vertices.size());
		std::memcpy(geo->Mapped, geo->Data.vertices.data(), vbSize);

		auto* cmdList = Render.GfxCmd.Get();

		D3D12_RESOURCE_BARRIER br{};
		br.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		br.Transition.pResource   = geo->GeometryBuffer.Get();
		br.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		br.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
		br.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
		cmdList->ResourceBarrier(1, &br);

		cmdList->CopyBufferRegion(
			geo->GeometryBuffer.Get(),
			0,
			geo->GeometryUploader.Get(),
			0,
			vbSize);

		std::swap(br.Transition.StateBefore, br.Transition.StateAfter);
		br.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		cmdList->ResourceBarrier(1, &br);
	}
}
