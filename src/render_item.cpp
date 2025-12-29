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
#include "framework/render_manager/components/render_item.h"
#include "framework/exception/dx_exception.h"
#include "imgui.h"

DirectX::XMFLOAT4X4 Transformation::GetTransform() const
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

void Transformation::ImguiView()
{
	bool changed = false;

	changed |= ImGui::DragFloat3("Position", &Position.x, 0.01f);
	changed |= ImGui::DragFloat3("Rotation (rad)", &Rotation.x, 0.01f);
	changed |= ImGui::DragFloat3("Scale", &Scale.x, 0.01f);

	if (changed)
		MarkDirty();
}

void MeshGeometry::InitGeometryBuffer(
	ID3D12Device *device,
	ID3D12GraphicsCommandList *cmdList,
	const MeshData &mesh,
	const bool keepMapping)
{
	Data = mesh;
	VertexStride = sizeof(MeshVertex);
	const std::uint32_t vbSize	= sizeof(MeshVertex) * mesh.vertices.size();
	const auto vbAlignment = (vbSize + 3u) & ~3u;
	VertexByteSize				= vbAlignment;

	const std::uint32_t ibSize = sizeof(uint32_t) * mesh.indices.size();
	const auto totalSize = vbAlignment + ibSize;

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

	D3D12_HEAP_PROPERTIES property{};
	property.Type				  = D3D12_HEAP_TYPE_DEFAULT;
	property.CPUPageProperty	  = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	property.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	property.CreationNodeMask	  = 1u;
	property.VisibleNodeMask	  = 1u;

	THROW_DX_IF_FAILS(device->CreateCommittedResource(
		&property,
		D3D12_HEAP_FLAG_NONE,
		&resource,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&GeometryBuffer)));

	D3D12_HEAP_PROPERTIES upload = property;
	upload.Type = D3D12_HEAP_TYPE_UPLOAD;

	THROW_DX_IF_FAILS(device->CreateCommittedResource(
		&upload,
		D3D12_HEAP_FLAG_NONE,
		&resource,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&GeometryUploader)));

	//~ copy data on uploader
	THROW_DX_IF_FAILS(GeometryUploader->Map(0u, nullptr,
	reinterpret_cast<void**>(&Mapped)));

	std::memcpy(Mapped, mesh.vertices.data(), vbSize);
	if (vbAlignment > vbSize)
	{
		std::memset(Mapped + vbSize, 0, vbAlignment - vbSize);
	}
	std::memcpy(Mapped + vbAlignment, mesh.indices.data(), ibSize);

	if (!keepMapping) GeometryUploader->Unmap(0u, nullptr);

	//~ transition
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.pResource	= GeometryBuffer.Get();
	barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(1, &barrier);

	cmdList->CopyBufferRegion(
		GeometryBuffer.Get(),
		0,
		GeometryUploader.Get(),
		0,
		totalSize);

	//~ transition back
	D3D12_RESOURCE_BARRIER final = barrier;
	final.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	final.Transition.StateAfter	 = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
								   D3D12_RESOURCE_STATE_INDEX_BUFFER;
	cmdList->ResourceBarrier(1, &final);

	//~ set vertex views
	D3D12_VERTEX_BUFFER_VIEW vert{};
	vert.BufferLocation = GeometryBuffer->GetGPUVirtualAddress();
	vert.StrideInBytes	= sizeof(MeshVertex);
	vert.SizeInBytes	= vbAlignment;
	VertexViews.push_back(vert);

	//~ set index view
	IndexViews.BufferLocation = GeometryBuffer->GetGPUVirtualAddress() + vbAlignment;
	IndexViews.Format		  = DXGI_FORMAT_R32_UINT;
	IndexViews.SizeInBytes	  = ibSize;

	IndexCount = mesh.indices.size();
}

void RenderItem::InitConstantBuffer(
	const std::uint32_t frameCount,
	ID3D12Device *device,
	framework::DescriptorHeap &heap)
{
	FrameCount = frameCount;
	constexpr std::uint32_t perObjSize	= (sizeof(PerObjectConstantsCPU) + 255u) & ~255u;
	constexpr std::uint32_t passSize	= (sizeof(PassConstantsCPU) + 255u) & ~255u;
	const std::uint32_t totalSize		= (passSize + perObjSize) * frameCount;

	D3D12_RESOURCE_DESC resource{};
	resource.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resource.Format				= DXGI_FORMAT_UNKNOWN;
	resource.Alignment			= 0u;
	resource.DepthOrArraySize	= 1u;
	resource.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	resource.Height				= 1u;
	resource.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource.MipLevels			= 1u;
	resource.SampleDesc.Count	= 1u;
	resource.SampleDesc.Quality = 0u;
	resource.Width = totalSize;

	D3D12_HEAP_PROPERTIES property{};
	property.Type				  = D3D12_HEAP_TYPE_UPLOAD;
	property.CPUPageProperty	  = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	property.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	property.CreationNodeMask	  = 1u;
	property.VisibleNodeMask	  = 1u;

	THROW_DX_IF_FAILS(device->CreateCommittedResource(
		&property, D3D12_HEAP_FLAG_NONE,
		&resource, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&ConstantBuffer)));

	BYTE* mapped{ nullptr };
	THROW_DX_IF_FAILS(ConstantBuffer->Map(
		0u, nullptr,
		reinterpret_cast<void**>(&mapped)));

	//~ init views
	PerObject.Size	  = perObjSize;
	PassConstant.Size = passSize;

	std::uint32_t offset = 0;
	for (int i = 0; i < FrameCount; i++)
	{
		const std::uint32_t base = heap.Allocate(2u);
		auto cpuHandle = heap.GetCPUHandle(base);
		auto gpuHandle = heap.GetGPUHandle(base);

		BaseCBHandle.push_back(gpuHandle);

		//~ init per object cb
		D3D12_CONSTANT_BUFFER_VIEW_DESC perView{};
		perView.BufferLocation = ConstantBuffer->GetGPUVirtualAddress() + offset;
		perView.SizeInBytes = perObjSize;
		device->CreateConstantBufferView(&perView, cpuHandle);
		PerObject.Views.push_back(perView);
		PerObject.Mapped.emplace_back(mapped + offset);

		//~ step
		offset += perObjSize;
		cpuHandle = heap.GetCPUHandle(base + 1u);

		//~ init per object cb
		D3D12_CONSTANT_BUFFER_VIEW_DESC passView{};
		passView.BufferLocation = ConstantBuffer->GetGPUVirtualAddress() + offset;
		passView.SizeInBytes = passSize;
		device->CreateConstantBufferView(&passView, cpuHandle);
		PassConstant.Views.push_back(passView);
		PassConstant.Mapped.emplace_back(mapped + offset);

		//~ step
		offset += passSize;
	}
}

void RenderItem::ImguiView()
{
	ImGui::PushID(this);

	if (const char* headerLabel = Name.empty() ? "RenderItem" : Name.c_str(); ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		// Identity
		{
			char nameBuffer[128]{};
			if (!Name.empty())
				std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", Name.c_str());

			if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
				Name = nameBuffer;

			ImGui::Checkbox("Visible", &Visible);
		}

		// Mesh
		{
			ImGui::Text("Mesh: %s", Mesh ? "Bound" : "NULL");
			if (Mesh) ImGui::Text("Mesh Ptr: %p", static_cast<void*>(Mesh));
		}

		// Primitive Mode
		{
			constexpr EPrimitiveMode modes[] =
			{
				EPrimitiveMode::PointList,
				EPrimitiveMode::LineStrip,
				EPrimitiveMode::LineList,
				EPrimitiveMode::TriangleStrip,
				EPrimitiveMode::TriangleList
			};

			const std::string preview = ToString(PrimitiveMode);

			if (ImGui::BeginCombo("Primitive Mode", preview.c_str()))
			{
				for (EPrimitiveMode mode : modes)
				{
					const bool selected = (mode == PrimitiveMode);
					if (ImGui::Selectable(ToString(mode).c_str(), selected))
						PrimitiveMode = mode;

					if (selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}

		// Frames
		{
			ImGui::Text("Frame Index: %u", FrameIndex);
			ImGui::Text("Frame Count: %u", FrameCount);
		}

		// Transform
		{
			ImGui::PushID("Transform");
			Transform.ImguiView();
			ImGui::PopID();
		}

		ImGui::Unindent();
	}

	ImGui::PopID();
}
