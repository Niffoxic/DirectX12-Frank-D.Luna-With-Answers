//
// Created by Niffoxic (Aka Harsh Dubey) on 12/20/2025.
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
#include "application/scene/scene_chapter_6.h"

#include "framework/exception/dx_exception.h"
#include "framework/windows_manager/windows_manager.h"
#include "utility/logger.h"

#include <algorithm>
#include <imgui.h>

#include "utility/helpers.h"

SceneChapter6::SceneChapter6(framework::DxRenderManager& renderer)
	: IScene(renderer)
{
	m_waitEvent = CreateEvent(nullptr, false, false, nullptr);

	// Left cube
	DirectX::XMStoreFloat4x4(
		&m_world,
		DirectX::XMMatrixTranslation(-2.0f, 0.f, 0.f)
	);

	// Right cube
	DirectX::XMStoreFloat4x4(
		&m_world_answer_2,
		DirectX::XMMatrixTranslation(0.f, 0.f, 0.f)
	);

	// most Right cube
	DirectX::XMStoreFloat4x4(
		&m_pyramidTransformation,
		DirectX::XMMatrixTranslation(2.f, 0.f, 0.f)
	);

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

	LoadData();
}

SceneChapter6::~SceneChapter6()
{
	SaveData();
	if (m_mapped)
	{
		m_constantBuffer->Unmap(0u, nullptr);
		m_mapped = nullptr;
	}
}

bool SceneChapter6::Initialize()
{
	return true;
}

void SceneChapter6::Shutdown()
{
}

void SceneChapter6::FrameBegin(const float deltaTime)
{
	m_totalTime += deltaTime;
	ResetEvent(m_waitEvent);
	THROW_DX_IF_FAILS(Render.GfxAllocator->Reset());
	THROW_DX_IF_FAILS(Render.GfxCmd->Reset(Render.GfxAllocator.Get(),
		    m_pipelineState.Get()));

	//~ Draw
	const auto frameIndex = Render.GetFrameIndex();
	const auto RTVHandle		 = Render.GetBackBufferHandle(frameIndex);
	auto* backRtv  = Render.GetBackBuffer(frameIndex);
	const auto DSVHandle		 = Render.GetDSVBaseHandle();

	//~ Create Resources
	CreateVertexBuffer  ();
	CreateIndexBuffer   ();
	CreateResourceHeap  ();
	CreateRootSignature ();
	CreateShaders		();
	CreatePipelineState ();

	//~ Create Resources for box-2
	CreateGeometry_answer_2		();
	CreatePipelineState_answer_2();

	ID3D12DescriptorHeap* heaps[]{ m_resourceHeap.Get() };
	Render.GfxCmd->SetDescriptorHeaps(1u, heaps);

	CreateConstantBuffer();
	//~ Updates
	UpdateConstantBuffer(deltaTime);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource	= backRtv;
	barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_RENDER_TARGET;
	Render.GfxCmd->ResourceBarrier(1u, &barrier);

	Render.GfxCmd->ClearDepthStencilView(DSVHandle,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f,
		0, 0, nullptr);

	constexpr float color[]{.7f, .7f, .7f, 1.f};
	Render.GfxCmd->ClearRenderTargetView(RTVHandle, color, 0u, nullptr);

	Render.GfxCmd->RSSetViewports(1u, &m_viewport);
	Render.GfxCmd->RSSetScissorRects(1u, &m_scissorRect);

	Render.GfxCmd->OMSetRenderTargets(1u, &RTVHandle,
		TRUE, &DSVHandle);

	//~ Set signature
	Render.GfxCmd->SetPipelineState(m_pipelineState.Get());
	Render.GfxCmd->SetGraphicsRootSignature(m_rootSignature.Get());
	DrawMainCubes();
}

void SceneChapter6::FrameEnd(float deltaTime)
{
	ID3D12DescriptorHeap* heaps[] = { Render.SrvHeap.Get() };
	Render.GfxCmd->SetDescriptorHeaps(1u, heaps);

	const auto frameIndex = Render.GetFrameIndex();
	auto* backRtv  = Render.GetBackBuffer(frameIndex);
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource	= backRtv;
	barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_PRESENT;
	Render.GfxCmd->ResourceBarrier(1u, &barrier);

	THROW_DX_IF_FAILS(Render.GfxCmd->Close());

	ID3D12CommandList* cmdLists[] = { Render.GfxCmd.Get() };
	Render.GfxQueue->ExecuteCommandLists(1u, cmdLists);

	THROW_DX_IF_FAILS(Render.SwapChain->Present(0u, 0u));

	Render.IncrementFenceValue();
	const auto fenceValue = Render.GetFenceValue();
	THROW_DX_IF_FAILS(Render.GfxQueue->Signal(Render.Fence.Get(), fenceValue));

	if (Render.Fence->GetCompletedValue() < fenceValue)
	{
		THROW_DX_IF_FAILS(Render.Fence->SetEventOnCompletion(fenceValue, m_waitEvent));
		WaitForSingleObject(m_waitEvent, INFINITE);
	}
	Render.IncrementFrameIndex();
}

void SceneChapter6::ImguiView(const float deltaTime)
{
	(void)deltaTime;

	if (ImGui::Begin("Chapter 6 Settings"))
	{
		ImGui::Checkbox("Draw Answer 2", &m_drawAnswer2);

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Answer 3", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Primitive Mode
			{
				static const char* kPrimItems[] =
				{
					"PointList",
					"LineStrip",
					"LineList",
					"TriangleStrip",
					"TriangleList"
				};

				int prim = static_cast<int>(m_primitiveMode);
				if (ImGui::Combo("Primitive Mode", &prim, kPrimItems, IM_ARRAYSIZE(kPrimItems)))
				{
					m_primitiveMode = static_cast<EPrimitiveMode>(prim);
				}
			}

			// Fill Mode
			{
				static const char* kFillItems[] =
				{
					"WireFrame",
					"Solid"
				};

				int fill = static_cast<int>(m_fillMode);
				if (ImGui::Combo("Fill Mode", &fill, kFillItems, IM_ARRAYSIZE(kFillItems)))
				{
					m_fillMode = static_cast<EFillMode>(fill);

					m_bPipelineInitialized  = false;
					m_bPipeline2Initialized = false;
				}
			}

			// Cull Mode
			{
				static const char* kCullItems[] =
				{
					"None",
					"Front",
					"Back"
				};

				int cull = static_cast<int>(m_cullMode);
				if (ImGui::Combo("Cull Mode", &cull, kCullItems, IM_ARRAYSIZE(kCullItems)))
				{
					m_cullMode = static_cast<ECullMode>(cull);

					m_bPipelineInitialized  = false;
					m_bPipeline2Initialized = false;
				}
			}
		}

		ImGui::Separator();
		ImGui::Checkbox("Draw Answer 4: Pyramid", &m_drawAnswer4);

		ImGui::Separator();
		ImGui::Checkbox("Draw Answer 6: Animate", &m_drawAnswer6);
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Answer 12: Viewport", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Viewport Position");
			ImGui::DragFloat("TopLeft X", &m_viewport.TopLeftX, 1.0f, 0.0f);
			ImGui::DragFloat("TopLeft Y", &m_viewport.TopLeftY, 1.0f, 0.0f);

			ImGui::Spacing();
			ImGui::Text("Viewport Size");

			const auto maxWidth  = static_cast<float>(Render.Windows->GetWindowsWidth());
			const auto maxHeight = static_cast<float>(Render.Windows->GetWindowsHeight());

			ImGui::DragFloat("Width",  &m_viewport.Width,  1.0f, 1.0f, maxWidth);
			ImGui::DragFloat("Height", &m_viewport.Height, 1.0f, 1.0f, maxHeight);

			ImGui::Spacing();
			ImGui::Text("Depth Range");

			ImGui::SliderFloat("Min Depth", &m_viewport.MinDepth, 0.0f, 1.0f);
			ImGui::SliderFloat("Max Depth", &m_viewport.MaxDepth, 0.0f, 1.0f);

			m_viewport.MinDepth = std::min(m_viewport.MinDepth, m_viewport.MaxDepth);
			m_viewport.MaxDepth = std::max(m_viewport.MaxDepth, m_viewport.MinDepth);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
			if (ImGui::Button("Reset Viewport to Default"))
			{
				m_viewport.TopLeftX = 0.0f;
				m_viewport.TopLeftY = 0.0f;

				m_viewport.Width  = static_cast<float>(Render.Windows->GetWindowsWidth());
				m_viewport.Height = static_cast<float>(Render.Windows->GetWindowsHeight());

				m_viewport.MinDepth = 0.0f;
				m_viewport.MaxDepth = 1.0f;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(Window-sized viewport)");
			ImGui::PopStyleColor();
		}

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Answer 13: Scissor Rect", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Scissor Bounds (pixels)");

			int left   = static_cast<int>(m_scissorRect.left);
			int top    = static_cast<int>(m_scissorRect.top);
			int right  = static_cast<int>(m_scissorRect.right);
			int bottom = static_cast<int>(m_scissorRect.bottom);

			const int maxW = static_cast<int>(Render.Windows->GetWindowsWidth());
			const int maxH = static_cast<int>(Render.Windows->GetWindowsHeight());

			ImGui::DragInt("Left",   &left,   1, 0, maxW);
			ImGui::DragInt("Top",    &top,    1, 0, maxH);
			ImGui::DragInt("Right",  &right,  1, 0, maxW);
			ImGui::DragInt("Bottom", &bottom, 1, 0, maxH);

			left   = std::clamp(left,   0, maxW);
			right  = std::clamp(right,  0, maxW);
			top    = std::clamp(top,    0, maxH);
			bottom = std::clamp(bottom, 0, maxH);

			if (right < left)   right = left;
			if (bottom < top)   bottom = top;

			m_scissorRect.left   = static_cast<LONG>(left);
			m_scissorRect.top    = static_cast<LONG>(top);
			m_scissorRect.right  = static_cast<LONG>(right);
			m_scissorRect.bottom = static_cast<LONG>(bottom);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
			if (ImGui::Button("Reset Scissor to Default"))
			{
				m_scissorRect.left   = 0;
				m_scissorRect.top    = 0;
				m_scissorRect.right  = Render.Windows->GetWindowsWidth();
				m_scissorRect.bottom = Render.Windows->GetWindowsHeight();
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(Window-sized scissor)");
			ImGui::PopStyleColor();
		}

		ImGui::Separator();
		ImGui::Checkbox("Draw Answer 14: Color Animate", &m_drawAnswer14_PixelColor);
		ImGui::Separator();

		ImGui::Separator();
		ImGui::Checkbox("Draw Answer 15: Apply Clipping", &m_drawAnswer15_ApplyClipping);
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Answer 16: Pulse Color", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Apply Pulsing", &m_drawAnswer16_ApplyPulsing);

			ImGui::Spacing();
			ImGui::Text("Pulse Color");

			ImGui::ColorEdit4("##PulseColor", &m_pulseColor.x,
				ImGuiColorEditFlags_Float |
				ImGuiColorEditFlags_DisplayRGB);

			// Optional: show current values
			ImGui::TextDisabled("RGBA: %.2f, %.2f, %.2f, %.2f",
				m_pulseColor.x, m_pulseColor.y, m_pulseColor.z, m_pulseColor.w);

			ImGui::Spacing();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
			if (ImGui::Button("Reset Pulse Color"))
			{
				m_pulseColor = DirectX::XMFLOAT4(0.24f, 0.24f, 1.0f, 1.0f);
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(Default pulse tint)");
			ImGui::PopStyleColor();
		}

	}
	ImGui::End();
}

void SceneChapter6::CreateVertexBuffer()
{
	if (m_bVertexBufferInitialized) return;
	const std::vector<Vertex> boxVertices =
	{
		// Front face
		{ { -0.5f,  0.5f, -0.5f }, { 1.f, 0.f, 0.f, 1.f } }, // 0
		{ {  0.5f,  0.5f, -0.5f }, { 0.f, 1.f, 0.f, 1.f } }, // 1
		{ {  0.5f, -0.5f, -0.5f }, { 0.f, 0.f, 1.f, 1.f } }, // 2
		{ { -0.5f, -0.5f, -0.5f }, { 1.f, 1.f, 0.f, 1.f } }, // 3

		// Back face
		{ { -0.5f,  0.5f,  0.5f }, { 1.f, 0.f, 1.f, 1.f } }, // 4
		{ {  0.5f,  0.5f,  0.5f }, { 0.f, 1.f, 1.f, 1.f } }, // 5
		{ {  0.5f, -0.5f,  0.5f }, { 1.f, 1.f, 1.f, 1.f } }, // 6
		{ { -0.5f, -0.5f,  0.5f }, { 0.f, 0.f, 0.f, 1.f } }, // 7
	};

	const std::vector<Vertex> pyramidVertices =
	{
		// Apex
		{ {  0.0f,  0.5f,  0.0f }, { 0.61f, 0.65f, 0.91f, 1.f } }, // 0 top

		// FRONT (+Z)
		{ {  0.5f, -0.5f,  0.5f }, { 0.54f, 0.21f, 0.76f, 1.f } }, // 1 front right
		{ { -0.5f, -0.5f,  0.5f }, { 0.f, 0.11f, 0.f, 1.f } }, // 2 front left

		// BACK (-Z)
		{ {  0.5f, -0.5f, -0.5f }, { 0.71f, 0.21f, 0.98f, 1.f } }, // 3 back right
		{ { -0.5f, -0.5f, -0.5f }, { 0.81f, 0.41f, 0.14f, 1.f } }, // 4 back left
	};

	//~ Create Default buffer
	const UINT64 boxSize	 = static_cast<UINT64>(boxVertices.size() * sizeof(Vertex));
	const UINT64 pyramidSize = static_cast<UINT64>(pyramidVertices.size() * sizeof(Vertex));
	const UINT64 totalSize	 = boxSize + pyramidSize;

	D3D12_RESOURCE_DESC resource{};
	resource.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resource.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	resource.Alignment			= 0u;
	resource.DepthOrArraySize	= 1u;
	resource.Format				= DXGI_FORMAT_UNKNOWN;
	resource.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource.Width				= totalSize;
	resource.Height				= 1u;
	resource.MipLevels			= 1u;
	resource.SampleDesc.Count	= 1u;
	resource.SampleDesc.Quality	= 0u;

	D3D12_HEAP_PROPERTIES properties{};
	properties.Type					= D3D12_HEAP_TYPE_DEFAULT;
	properties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.CreationNodeMask		= 1u;
	properties.VisibleNodeMask		= 1u;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	THROW_DX_IF_FAILS(Render.Device->CreateCommittedResource(
		&properties, D3D12_HEAP_FLAG_NONE,
		&resource, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)
	));

	//~ Create Upload Buffer
	D3D12_HEAP_PROPERTIES uploadHeap = properties;
	uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
	THROW_DX_IF_FAILS(Render.Device->CreateCommittedResource(
		&uploadHeap, D3D12_HEAP_FLAG_NONE,
		&resource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_uploadBuffer)
		));

	//~ Copy Resources to the dst
	BYTE* mapped = nullptr;
	THROW_DX_IF_FAILS(m_uploadBuffer->Map(0u,
		nullptr, reinterpret_cast<void**>(&mapped)));

	std::memcpy(mapped + 0u, boxVertices.data(), boxSize);
	std::memcpy(mapped + boxSize, pyramidVertices.data(), pyramidSize);

	m_uploadBuffer->Unmap(0u, nullptr);

	//~ copy data from upload to vb
	Render.GfxCmd->CopyBufferRegion(
		m_vertexBuffer.Get(),
		0u,
		m_uploadBuffer.Get(),
		0u,
		totalSize);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource	= m_vertexBuffer.Get();
	barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	Render.GfxCmd->ResourceBarrier(1u, &barrier);

	m_vertexView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexView.SizeInBytes	= boxSize;
	m_vertexView.StrideInBytes	= sizeof(Vertex);

	m_pyramidVertexView.BufferLocation	= m_vertexBuffer->GetGPUVirtualAddress() + boxSize;
	m_pyramidVertexView.SizeInBytes		= pyramidSize;
	m_pyramidVertexView.StrideInBytes	= sizeof(Vertex);

	m_bVertexBufferInitialized = true;
	logger::success("Created Vertex Buffer!");
}

void SceneChapter6::CreateIndexBuffer()
{
	if (m_bIndexBufferInitialized) return;

	const std::vector<UINT16> boxIndices =
	{
		// Front
		0, 1, 2,  0, 2, 3,

		// Back
		5, 4, 7,  5, 7, 6,

		// Left
		4, 0, 3,  4, 3, 7,

		// Right
		1, 5, 6,  1, 6, 2,

		// Top
		4, 5, 1,  4, 1, 0,

		// Bottom
		3, 2, 6,  3, 6, 7
	};


	const std::vector<UINT16> pyramidIndices =
	{
		0, 2, 1, // front
		0, 1, 3, // right
		0, 3, 4, // back
		0, 4, 2, // left
		1, 2, 4, // base
		1, 4, 3
	};

	const UINT64 boxSize	 = static_cast<UINT64>(boxIndices.size() * sizeof(UINT16));
	const UINT64 pyramidSize = static_cast<UINT64>(pyramidIndices.size() * sizeof(UINT16));
	const UINT64 totalSize	 = boxSize + pyramidSize;

	//~ Create index buffer
	D3D12_RESOURCE_DESC resource{};
	resource.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resource.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	resource.Alignment			= 0u;
	resource.DepthOrArraySize	= 1u;
	resource.Format				= DXGI_FORMAT_UNKNOWN;
	resource.Width				= totalSize;
	resource.Height				= 1u;
	resource.MipLevels			= 1u;
	resource.SampleDesc.Count	= 1u;
	resource.SampleDesc.Quality = 0u;
	resource.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES properties{};
	properties.Type					= D3D12_HEAP_TYPE_DEFAULT;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.CreationNodeMask		= 1u;
	properties.VisibleNodeMask		= 1u;

	THROW_DX_IF_FAILS(Render.Device->CreateCommittedResource(
		&properties, D3D12_HEAP_FLAG_NONE,
		&resource, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&m_indexBuffer)
	));

	//~ Create Upload buffer
	D3D12_HEAP_PROPERTIES uploadProperties{};
	uploadProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	THROW_DX_IF_FAILS(Render.Device->CreateCommittedResource(
	&uploadProperties, D3D12_HEAP_FLAG_NONE,
	&resource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
	IID_PPV_ARGS(&m_uploadIndex)));

	//~ Map
	BYTE* mapped = nullptr;
	THROW_DX_IF_FAILS(m_uploadIndex->Map(0u,
		nullptr, reinterpret_cast<void**>(&mapped)));

	//~ Attach Resources
	std::memcpy(mapped, boxIndices.data(), boxSize);
	std::memcpy(mapped + boxSize, pyramidIndices.data(), pyramidSize);
	m_uploadIndex->Unmap(0u, nullptr);

	//~ Copy resources from upload to resources
	Render.GfxCmd->CopyBufferRegion(
		m_indexBuffer.Get(),
		0u,
		m_uploadIndex.Get(),
		0u,
		totalSize);

	//~ Set barrier
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource	= m_indexBuffer.Get();
	barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_INDEX_BUFFER;
	barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	Render.GfxCmd->ResourceBarrier(1u, &barrier);

	//~ set index view
	m_indexView.BufferLocation	= m_indexBuffer->GetGPUVirtualAddress();
	m_indexView.SizeInBytes		= boxSize;
	m_indexView.Format			= DXGI_FORMAT_R16_UINT;

	m_pyramidIndexView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress() + boxSize;
	m_pyramidIndexView.SizeInBytes	  = pyramidSize;
	m_pyramidIndexView.Format		  = DXGI_FORMAT_R16_UINT;

	m_bIndexBufferInitialized = true;
	logger::success("Created Index Buffer!");
}

void SceneChapter6::CreateResourceHeap()
{
	if (m_bResourceHeapInitialized) return;
	m_bResourceHeapInitialized = true;

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors	= 3u;
	desc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask		= 0u;

	THROW_DX_IF_FAILS(Render.Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_resourceHeap)));
	logger::success("Created CSV Descriptor Heap!");
}

void SceneChapter6::CreateConstantBuffer()
{
	if (m_bConstantBufferInitialized) return;

	constexpr UINT64 cbSize = (sizeof(Constants) + 255u) & ~255u;
	const UINT64 totalSize = cbSize * m_constantsNumbers;

	//~ Create buffer
	D3D12_RESOURCE_DESC resource{};
	resource.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resource.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	resource.Alignment			= 0u;
	resource.DepthOrArraySize	= 1u;
	resource.Format				= DXGI_FORMAT_UNKNOWN;
	resource.Width				= totalSize;
	resource.Height				= 1u;
	resource.MipLevels			= 1u;
	resource.SampleDesc.Count	= 1u;
	resource.SampleDesc.Quality = 0u;
	resource.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES properties{};
	properties.Type					= D3D12_HEAP_TYPE_UPLOAD;
	properties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CreationNodeMask		= 1u;
	properties.VisibleNodeMask		= 1u;

	THROW_DX_IF_FAILS(Render.Device->CreateCommittedResource(
		&properties, D3D12_HEAP_FLAG_NONE,
		&resource, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&m_constantBuffer)
		));

	THROW_DX_IF_FAILS(m_constantBuffer->Map(0u, nullptr, &m_mapped));

	m_constantBufferView.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
	m_constantBufferView.SizeInBytes	= cbSize;
	Render.Device->CreateConstantBufferView(&m_constantBufferView,
		m_resourceHeap->GetCPUDescriptorHandleForHeapStart());
	m_bConstantBufferInitialized = true;

	//~ Create 2nd Constant buffer
	m_answer2_ConstantHandle = m_resourceHeap->GetCPUDescriptorHandleForHeapStart();
	m_answer2_ConstantHandle.ptr += Render.HeapSizes.SRV;

	m_answer2_CBV.BufferLocation = m_constantBuffer->GetGPUVirtualAddress() + cbSize;
	m_answer2_CBV.SizeInBytes	 = cbSize;
	Render.Device->CreateConstantBufferView(&m_answer2_CBV, m_answer2_ConstantHandle);

	//~ Create 3rd Constant Buffer for pyramids
	m_answer2_ConstantHandle.ptr += Render.HeapSizes.SRV; // point to the 3rd heap handle
	m_pyramidGPUHandle = m_resourceHeap->GetGPUDescriptorHandleForHeapStart();
	m_pyramidGPUHandle.ptr += (Render.HeapSizes.SRV * 2u); // 3rd constant buffer

	m_pyramidCBV.BufferLocation = m_constantBuffer->GetGPUVirtualAddress() + (cbSize * 2);
	m_pyramidCBV.SizeInBytes	= cbSize;
	Render.Device->CreateConstantBufferView(&m_pyramidCBV, m_answer2_ConstantHandle);

	logger::success("Created ConstantBuffer and constant view allocated at 1st resource heap index!");
}

void SceneChapter6::CreateRootSignature()
{
	if (m_bRootSignatureInitialized) return;

	D3D12_DESCRIPTOR_RANGE table{};
	table.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	table.NumDescriptors					= 1u;
	table.RegisterSpace						= 0u;
	table.BaseShaderRegister				= 0u; // b0
	table.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER param[1];
	param[0].ParameterType	  = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	param[0].DescriptorTable.NumDescriptorRanges = 1u;
	param[0].DescriptorTable.pDescriptorRanges   = &table;

	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.Flags				= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	desc.NumParameters		= 1u;
	desc.pParameters		= param;
	desc.NumStaticSamplers	= 0u;
	desc.pStaticSamplers	= nullptr;

	//~ Create root signature with a single slot
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignature{ nullptr };
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob{ nullptr };

	const HRESULT hr = D3D12SerializeRootSignature(&desc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSignature.GetAddressOf(),
		errorBlob.GetAddressOf());

	if (FAILED(hr))
	{
		logger::warning("Failed to serialize root signature!");
	}

	THROW_DX_IF_FAILS(Render.Device->CreateRootSignature(
		0u, serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)
	));
	logger::success("Created root signature!");
	m_bRootSignatureInitialized = true;
}

void SceneChapter6::CreateShaders()
{
	if (m_bShadersInitialized) return;

	const std::string vsPath	= "shaders/chapter_6/vertex_shader.hlsl";
	const std::wstring vsPathW	= L"shaders/chapter_6/vertex_shader.hlsl";
	const std::string psPath	= "shaders/chapter_6/pixel_shader.hlsl";
	const std::wstring psPathW	= L"shaders/chapter_6/pixel_shader.hlsl";

	if (!helpers::IsFile(vsPath) || !helpers::IsFile(psPath))
	{
		const std::string message = std::format("Either {} or {} is not a valid file path! ", vsPath, psPath);
		THROW_MSG(message.c_str());
	}

	m_vertexShader = framework::DxRenderManager::CompileShader(
		vsPathW, nullptr, "main", "vs_5_0");

	m_pixelShader = framework::DxRenderManager::CompileShader(
	psPathW, nullptr, "main", "ps_5_0");

	m_bShadersInitialized = true;

	//~ init viewport
	m_viewport.Height	= Render.Windows->GetWindowsHeight();
	m_viewport.Width	= Render.Windows->GetWindowsWidth();
	m_viewport.TopLeftX = 0.f;
	m_viewport.TopLeftY = 0.f;
	m_viewport.MinDepth = 0.f;
	m_viewport.MaxDepth = 1.f;

	//~ init scissor rect
	m_scissorRect.left = 0ll;
	m_scissorRect.top = 0ll;
	m_scissorRect.right = Render.Windows->GetWindowsWidth();
	m_scissorRect.bottom = Render.Windows->GetWindowsHeight();
}

void SceneChapter6::CreatePipelineState()
{
	if (m_bPipelineInitialized) return;

	//~ Rasterizer
	D3D12_RASTERIZER_DESC rasterizer{};
	rasterizer.FillMode				 = GetFillMode(m_fillMode);
	rasterizer.CullMode				 = GetCullMode(m_cullMode);
	rasterizer.FrontCounterClockwise = FALSE;
	rasterizer.DepthBias			 = 0;
	rasterizer.DepthBiasClamp		 = 0.0f;
	rasterizer.SlopeScaledDepthBias	 = 0.0f;
	rasterizer.DepthClipEnable		 = TRUE;
	rasterizer.MultisampleEnable	 = FALSE;
	rasterizer.AntialiasedLineEnable = FALSE;
	rasterizer.ForcedSampleCount	 = 0;
	rasterizer.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	static auto layout = Vertex::GetElementLayout();

	//~ Default blend state
	D3D12_BLEND_DESC blend{};
	blend.AlphaToCoverageEnable = FALSE;
	blend.IndependentBlendEnable = FALSE;

	auto& rt = blend.RenderTarget[0];
	rt.BlendEnable           = FALSE;
	rt.LogicOpEnable         = FALSE;
	rt.SrcBlend              = D3D12_BLEND_ONE;
	rt.DestBlend             = D3D12_BLEND_ZERO;
	rt.BlendOp               = D3D12_BLEND_OP_ADD;
	rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
	rt.DestBlendAlpha        = D3D12_BLEND_ZERO;
	rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
	rt.LogicOp               = D3D12_LOGIC_OP_NOOP;
	rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//~ Default depth-stencil state
	D3D12_DEPTH_STENCIL_DESC depth{};
	depth.DepthEnable      = TRUE;
	depth.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
	depth.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
	depth.StencilEnable    = FALSE;
	depth.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;
	depth.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	//~ PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.Flags    = D3D12_PIPELINE_STATE_FLAG_NONE;
	desc.NodeMask = 0u;

	desc.InputLayout.NumElements        = static_cast<UINT>(layout.size());
	desc.InputLayout.pInputElementDescs = layout.data();

	desc.pRootSignature = m_rootSignature.Get();

	desc.VS = { m_vertexShader->GetBufferPointer(), m_vertexShader->GetBufferSize() };
	desc.PS = { m_pixelShader->GetBufferPointer(),  m_pixelShader->GetBufferSize()  };

	desc.BlendState        = blend;
	desc.DepthStencilState = depth;
	desc.RasterizerState   = rasterizer;

	switch (m_primitiveMode)
	{
	case EPrimitiveMode::PointList:     desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;    break;
	case EPrimitiveMode::LineStrip:
	case EPrimitiveMode::LineList:      desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;     break;
	case EPrimitiveMode::TriangleStrip:
	case EPrimitiveMode::TriangleList:  desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
	default:                            desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
	}

	desc.NumRenderTargets = 1;
	desc.RTVFormats[0]    = Render.BackBufferFormat;
	desc.DSVFormat        = Render.DsvFormat;

	desc.SampleMask         = UINT_MAX;
	desc.SampleDesc.Count   = 1;
	desc.SampleDesc.Quality = 0;

	THROW_DX_IF_FAILS(Render.Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));

	m_bPipelineInitialized = true;
}

void SceneChapter6::CreatePipelineState_answer_2()
{
	if (m_bPipeline2Initialized) return;

	//~ Rasterizer
	D3D12_RASTERIZER_DESC rasterizer{};
	rasterizer.FillMode				 = GetFillMode(m_fillMode);
	rasterizer.CullMode				 = GetCullMode(m_cullMode);
	rasterizer.FrontCounterClockwise = FALSE;
	rasterizer.DepthBias			 = 0;
	rasterizer.DepthBiasClamp		 = 0.0f;
	rasterizer.SlopeScaledDepthBias	 = 0.0f;
	rasterizer.DepthClipEnable		 = TRUE;
	rasterizer.MultisampleEnable	 = FALSE;
	rasterizer.AntialiasedLineEnable = FALSE;
	rasterizer.ForcedSampleCount	 = 0;
	rasterizer.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	std::vector<D3D12_INPUT_ELEMENT_DESC> layout;
	layout.emplace_back(VPosData::GetElementLayout());
	layout.emplace_back(VColorData::GetElementLayout());

	//~ Default blend state
	D3D12_BLEND_DESC blend{};
	blend.AlphaToCoverageEnable  = FALSE;
	blend.IndependentBlendEnable = FALSE;

	auto& rt = blend.RenderTarget[0];
	rt.BlendEnable           = FALSE;
	rt.LogicOpEnable         = FALSE;
	rt.SrcBlend              = D3D12_BLEND_ONE;
	rt.DestBlend             = D3D12_BLEND_ZERO;
	rt.BlendOp               = D3D12_BLEND_OP_ADD;
	rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
	rt.DestBlendAlpha        = D3D12_BLEND_ZERO;
	rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
	rt.LogicOp               = D3D12_LOGIC_OP_NOOP;
	rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//~ Default depth-stencil state
	D3D12_DEPTH_STENCIL_DESC depth{};
	depth.DepthEnable      = TRUE;
	depth.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
	depth.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
	depth.StencilEnable    = FALSE;
	depth.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;
	depth.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	//~ PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.Flags    = D3D12_PIPELINE_STATE_FLAG_NONE;
	desc.NodeMask = 0u;

	desc.InputLayout.NumElements        = static_cast<UINT>(layout.size());
	desc.InputLayout.pInputElementDescs = layout.data();

	desc.pRootSignature = m_rootSignature.Get();

	desc.VS = { m_vertexShader->GetBufferPointer(), m_vertexShader->GetBufferSize() };
	desc.PS = { m_pixelShader->GetBufferPointer(),  m_pixelShader->GetBufferSize()  };

	desc.BlendState        = blend;
	desc.DepthStencilState = depth;
	desc.RasterizerState   = rasterizer;

	switch (m_primitiveMode)
	{
	case EPrimitiveMode::PointList:     desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;    break;
	case EPrimitiveMode::LineStrip:
	case EPrimitiveMode::LineList:      desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;     break;
	case EPrimitiveMode::TriangleStrip:
	case EPrimitiveMode::TriangleList:  desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
	default:                            desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
	}

	desc.NumRenderTargets = 1;
	desc.RTVFormats[0]    = Render.BackBufferFormat;
	desc.DSVFormat        = Render.DsvFormat;

	desc.SampleMask         = UINT_MAX;
	desc.SampleDesc.Count   = 1;
	desc.SampleDesc.Quality = 0;

	THROW_DX_IF_FAILS(Render.Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipeline2State)));

	m_bPipeline2Initialized = true;
}

void SceneChapter6::CreateGeometry_answer_2()
{
	if (m_bAnswer2_Initialized) return;
	m_bAnswer2_Initialized = true;

	const std::vector<VPosData> geometry =
	{
		// Front face
		{ { -0.5f,  0.5f, -0.5f } }, // 0
		{ {  0.5f,  0.5f, -0.5f } }, // 1
		{ {  0.5f, -0.5f, -0.5f } }, // 2
		{ { -0.5f, -0.5f, -0.5f } }, // 3

		// Back face
		{ { -0.5f,  0.5f,  0.5f } }, // 4
		{ {  0.5f,  0.5f,  0.5f } }, // 5
		{ {  0.5f, -0.5f,  0.5f } }, // 6
		{ { -0.5f, -0.5f,  0.5f } }, // 7
	};

	const std::vector<VColorData> color =
	{
		// Front face
		{ { 0.95f, 0.35f, 0.15f, 1.f } }, // 0  orange
		{ { 0.15f, 0.75f, 0.25f, 1.f } }, // 1  green
		{ { 0.20f, 0.45f, 0.95f, 1.f } }, // 2  blue
		{ { 0.85f, 0.20f, 0.75f, 1.f } }, // 3  magenta

		// Back face
		{ { 0.10f, 0.85f, 0.90f, 1.f } }, // 4  cyan
		{ { 0.95f, 0.90f, 0.25f, 1.f } }, // 5  yellow
		{ { 0.95f, 0.95f, 0.95f, 1.f } }, // 6  white
		{ { 0.12f, 0.12f, 0.18f, 1.f } }, // 7  dark
	};

	const auto geometrySize = static_cast<UINT64>(geometry.size() * sizeof(VPosData));
	const auto colorSize    = static_cast<UINT64>(color.size() * sizeof(VColorData));
	const auto totalSize = geometrySize + colorSize;

	D3D12_RESOURCE_DESC resource{};
	resource.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resource.Alignment			= 0u;
	resource.DepthOrArraySize	= 1u;
	resource.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	resource.Format				= DXGI_FORMAT_UNKNOWN;
	resource.Height				= 1u;
	resource.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource.MipLevels			= 1u;
	resource.SampleDesc.Count	= 1u;
	resource.SampleDesc.Quality = 0u;
	resource.Width				= totalSize;

	D3D12_HEAP_PROPERTIES properties{};
	properties.Type					= D3D12_HEAP_TYPE_DEFAULT;
	properties.CreationNodeMask		= 1u;
	properties.VisibleNodeMask		= 1u;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

	THROW_DX_IF_FAILS(Render.Device->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&resource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_answer2Buffer)
	));

	//~ Create Uploader
	D3D12_HEAP_PROPERTIES uploader = properties;
	uploader.Type = D3D12_HEAP_TYPE_UPLOAD;

	THROW_DX_IF_FAILS(Render.Device->CreateCommittedResource(
		&uploader, D3D12_HEAP_FLAG_NONE,
		&resource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_answer2Uploader)
		));

	//~ attach data
	BYTE* mapped = nullptr;
	THROW_DX_IF_FAILS(m_answer2Uploader->Map(0u, nullptr,
		reinterpret_cast<void**>(&mapped)));
	std::memcpy(mapped + 0, geometry.data(), geometrySize);
	std::memcpy(mapped + geometrySize, color.data(), colorSize);

	m_answer2Uploader->Unmap(0u, nullptr);

	//~ upload data
	Render.GfxCmd->CopyBufferRegion(
		m_answer2Buffer.Get(),
		0U,
		m_answer2Uploader.Get(),
		0U,
		totalSize);

	//~ transition back
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource	= m_answer2Buffer.Get();
	barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	Render.GfxCmd->ResourceBarrier(1u, &barrier);

	//~ prepare view desc
	m_geometryView.BufferLocation = m_answer2Buffer->GetGPUVirtualAddress();
	m_geometryView.SizeInBytes	  = geometrySize;
	m_geometryView.StrideInBytes  = sizeof(VPosData);

	m_colorView.BufferLocation = m_answer2Buffer->GetGPUVirtualAddress() + geometrySize;
	m_colorView.SizeInBytes	   = colorSize;
	m_colorView.StrideInBytes  = sizeof(VColorData);

	logger::success("Created Answer 2 Geometry Data!");
}

void SceneChapter6::LoadData()
{
}

void SceneChapter6::SaveData() const
{
}

void SceneChapter6::UpdateConstantBuffer(float deltaTime)
{
	const framework::DxMouseInputs& mouse = Render.Windows->Mouse;

	int idx, idy;
	mouse.GetMouseDelta(idx, idy);

	if (mouse.IsMouseButtonPressed(0u)) //~ left down
	{
		const float dx = DirectX::XMConvertToRadians(0.25f * idx);
		const float dy = DirectX::XMConvertToRadians(0.25f * idy);

		m_theta -= dx;
		m_phi   -= dy;

		m_phi = std::clamp(m_phi, 0.1f, 3.13f);
	}
	else if (mouse.IsMouseButtonPressed(1u)) //~ right down
	{
		const float dx = DirectX::XMConvertToRadians(0.05f * idx);
		const float dy = DirectX::XMConvertToRadians(0.05f * idy);

		m_radius += dx - dy;
		m_radius = std::clamp(m_radius, 3.0f, 15.0f);
	}

	//~ update in vals
	const float x = m_radius * std::sinf(m_phi) * std::cosf(m_theta);
	const float z = m_radius * std::sinf(m_phi) * std::sinf(m_theta);
	const float y = m_radius * std::cosf(m_phi);

	//~ update view matrix
	const DirectX::XMVECTOR pos	   = DirectX::XMVectorSet(x, y, z, 1.0f);
	const DirectX::XMVECTOR target = DirectX::XMVectorZero();
	const DirectX::XMVECTOR up	   = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	const DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, view);

	constexpr UINT64 cbSize = (sizeof(Constants) + 255u) & ~255u;
	auto* base = static_cast<BYTE*>(m_mapped);
	auto* cb0  = reinterpret_cast<Constants*>(base + 0u);
	auto* cb1 =  reinterpret_cast<Constants*>(base + cbSize);
	auto* cb2 =  reinterpret_cast<Constants*>(base + (2u * cbSize));

	DirectX::XMMATRIX W =
		m_drawAnswer2
		? DirectX::XMLoadFloat4x4(&m_world)
		: DirectX::XMMatrixIdentity();

	const DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&m_view);
	const DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&m_proj);

	const DirectX::XMMATRIX WVP = W * V * P;
	DirectX::XMStoreFloat4x4(&cb0->WorldViewProjection,
							 DirectX::XMMatrixTranspose(WVP));

	cb0->Time			= m_totalTime;
	cb0->Animate		= m_drawAnswer6 ? 1.f: 0.f;
	cb0->ColorAnimation = m_drawAnswer14_PixelColor ? 1.f: 0.f;
	cb0->ApplyClipping	= m_drawAnswer15_ApplyClipping ? 1.f: 0.f;
	cb0->ApplyPulse		= m_drawAnswer16_ApplyPulsing ? 1.f: 0.f;
	cb0->PulseColor		= m_pulseColor;

	W = DirectX::XMLoadFloat4x4(&m_world_answer_2);
	const DirectX::XMMATRIX WVP_2 = W * V * P;
	DirectX::XMStoreFloat4x4(&cb1->WorldViewProjection,
						 DirectX::XMMatrixTranspose(WVP_2));

	cb1->Time			= m_totalTime;
	cb1->Animate		= m_drawAnswer6 ? 1.f: 0.f;
	cb1->ColorAnimation = m_drawAnswer14_PixelColor ? 1.f: 0.f;
	cb1->ApplyClipping	= m_drawAnswer15_ApplyClipping ? 1.f: 0.f;
	cb1->ApplyPulse		= m_drawAnswer16_ApplyPulsing ? 1.f: 0.f;
	cb1->PulseColor		= m_pulseColor;

	//~ Update pyramid constant buffer
	const DirectX::XMMATRIX PW	  = DirectX::XMLoadFloat4x4(&m_pyramidTransformation);
	const DirectX::XMMATRIX WVP_3 = PW * V * P;

	DirectX::XMStoreFloat4x4(&cb2->WorldViewProjection,
					 DirectX::XMMatrixTranspose(WVP_3));

	cb2->Time			= m_totalTime;
	cb2->Animate		= m_drawAnswer6 ? 1.f: 0.f;
	cb2->ColorAnimation = m_drawAnswer14_PixelColor ? 1.f: 0.f;
	cb2->ApplyClipping	= m_drawAnswer15_ApplyClipping ? 1.f: 0.f;
	cb2->ApplyPulse		= m_drawAnswer16_ApplyPulsing ? 1.f: 0.f;
	cb2->PulseColor		= m_pulseColor;
}

void SceneChapter6::DrawMainCubes() const
{
	//~ set constant buffer for cube 1
	auto csvHandle = m_resourceHeap->GetGPUDescriptorHandleForHeapStart();
	Render.GfxCmd->SetGraphicsRootDescriptorTable(0u, csvHandle);

	//~ set geometry
	const D3D12_VERTEX_BUFFER_VIEW views[]{ m_vertexView };
	Render.GfxCmd->IASetVertexBuffers(0u, 1u, views);
	Render.GfxCmd->IASetIndexBuffer(&m_indexView);
	Render.GfxCmd->IASetPrimitiveTopology(GetTopologyType(m_primitiveMode));
	Render.GfxCmd->DrawIndexedInstanced(36u, 1u,
							0u, 0u,
							0u);

	if (m_drawAnswer4) //~ draw pyramids
	{
		//~ Set Constant buffer
		Render.GfxCmd->SetGraphicsRootDescriptorTable(0u, m_pyramidGPUHandle);

		const D3D12_VERTEX_BUFFER_VIEW pViews[]{ m_pyramidVertexView };
		Render.GfxCmd->IASetVertexBuffers(0u, 1u, pViews);
		Render.GfxCmd->IASetIndexBuffer(&m_pyramidIndexView);
		Render.GfxCmd->DrawIndexedInstanced(18u, 1u,
								0u, 0u,
								0u);
	}

	//~ draw answer 2 box
	if (m_drawAnswer2)
	{
		Render.GfxCmd->SetPipelineState(m_pipeline2State.Get());

		csvHandle.ptr += Render.HeapSizes.SRV;
		Render.GfxCmd->SetGraphicsRootDescriptorTable(0u, csvHandle);

		const D3D12_VERTEX_BUFFER_VIEW answers2[]{ m_geometryView, m_colorView };
		Render.GfxCmd->IASetVertexBuffers(0u, 2u, answers2);
		Render.GfxCmd->IASetIndexBuffer(&m_indexView);
		Render.GfxCmd->DrawIndexedInstanced(36u, 1u, 0u, 0u, 0u);

	}
}
