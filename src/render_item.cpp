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

static inline void Normalize3(DirectX::XMFLOAT3& v)
{
	const float x = v.x, y = v.y, z = v.z;
	const float lenSq = x*x + y*y + z*z;
	if (lenSq > 1e-8f)
	{
		const float invLen = 1.0f / std::sqrt(lenSq);
		v.x *= invLen; v.y *= invLen; v.z *= invLen;
	}
}

static inline void ImGuiEditVec3(const char* label, DirectX::XMFLOAT3& v, float speed, float minV, float maxV)
{
	ImGui::DragFloat3(label, &v.x, speed, minV, maxV);
}

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

LightCPU & LightManager::AddDirectional(const DirectX::XMFLOAT3 &direction,
	const DirectX::XMFLOAT3 &strength)
{
	LightCPU light{};
	light.Direction = direction;
	light.Strength  = strength;

	DirectionalLights.push_back(light);
	return DirectionalLights.back();
}

LightCPU & LightManager::AddPoint(const DirectX::XMFLOAT3 &position,
	const DirectX::XMFLOAT3 &strength,
	float falloffStart, float falloffEnd)
{
	LightCPU light{};
	light.Position     = position;
	light.Strength     = strength;
	light.FalloffStart = falloffStart;
	light.FalloffEnd   = falloffEnd;

	PointLights.push_back(light);
	return PointLights.back();
}

LightCPU & LightManager::AddSpot(const DirectX::XMFLOAT3 &position,
	const DirectX::XMFLOAT3 &direction,
	const DirectX::XMFLOAT3 &strength, float falloffStart, float falloffEnd,
	float spotPower)
{
	LightCPU light{};
	light.Position     = position;
	light.Direction    = direction;
	light.Strength     = strength;
	light.FalloffStart = falloffStart;
	light.FalloffEnd   = falloffEnd;
	light.SpotPower    = spotPower;

	SpotLights.push_back(light);
	return SpotLights.back();
}

void LightManager::Clear()
{
	DirectionalLights.clear();
	PointLights.clear();
	SpotLights.clear();
}

std::uint32_t LightManager::TotalLightCount() const
{
	return static_cast<std::uint32_t>(
	DirectionalLights.size() +
	PointLights.size() +
	SpotLights.size());
}

void LightManager::FillPassConstants(PassConstantsCPU &out) const
{
	out.NumDirLights   = 0;
	out.NumPointLights = 0;
	out.NumSpotLights  = 0;

	std::uint32_t index = 0;

	// Directional lights first
	for (const auto& l : DirectionalLights)
	{
		if (index >= MaxLights) break;
		out.Lights[index++] = l;
		out.NumDirLights++;
	}

	// Point lights next
	for (const auto& l : PointLights)
	{
		if (index >= MaxLights) break;
		out.Lights[index++] = l;
		out.NumPointLights++;
	}

	// Spot lights last
	for (const auto& l : SpotLights)
	{
		if (index >= MaxLights) break;
		out.Lights[index++] = l;
		out.NumSpotLights++;
	}

	// Clear unused slots (important!)
	for (; index < MaxLights; ++index)
	{
		out.Lights[index] = {};
	}
}

void LightManager::ImguiView()
{
	ImGui::PushID(this);

	if (!ImGui::CollapsingHeader("Light Manager", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PopID();
		return;
	}

	ImGui::Indent();

	ImGui::Text("Total: %u (Packed Max: %u)", TotalLightCount(), MaxLights);

	// Add buttons
	if (ImGui::Button("+ Directional"))
	{
		AddDirectional({ 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	}
	ImGui::SameLine();
	if (ImGui::Button("+ Point"))
	{
		AddPoint({ 0.0f, 2.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f, 10.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("+ Spot"))
	{
		AddSpot({ 0.0f, 2.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f, 15.0f, 64.0f);
	}

	ImGui::Separator();

	auto DrawList = [&](const char* title, std::vector<LightCPU>& list, ELightType type)
	{
		std::string header = std::string(title) + " (" + std::to_string(list.size()) + ")";
		if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			return;

		ImGui::Indent();

		for (int i = 0; i < (int)list.size(); ++i)
		{
			ImGui::PushID(i);

			char label[64]{};
			std::snprintf(label, sizeof(label), "%s %d", title, i);

			if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto& l = list[static_cast<size_t>(i)];

				ImGuiEditVec3("Strength", l.Strength, 0.01f, 0.0f, 10000.0f);

				if (type == ELightType::Directional || type == ELightType::Spotlight)
				{
					ImGuiEditVec3("Direction", l.Direction, 0.01f, -1.0f, 1.0f);
					ImGui::SameLine();
					if (ImGui::Button("Normalize##Dir"))
						Normalize3(l.Direction);
				}

				if (type == ELightType::Point || type == ELightType::Spotlight)
				{
					ImGuiEditVec3("Position", l.Position, 0.05f, -100000.0f, 100000.0f);

					ImGui::DragFloat("FalloffStart", &l.FalloffStart, 0.05f, 0.0f, 100000.0f);
					ImGui::DragFloat("FalloffEnd",   &l.FalloffEnd,   0.05f, 0.0f, 100000.0f);

					if (l.FalloffEnd < l.FalloffStart)
						l.FalloffEnd = l.FalloffStart;
				}

				if (type == ELightType::Spotlight)
				{
					ImGui::DragFloat("SpotPower", &l.SpotPower, 1.0f, 1.0f, 512.0f);
				}

				ImGui::Separator();

				if (ImGui::Button("Remove"))
				{
					list.erase(list.begin() + i);
					ImGui::TreePop();
					ImGui::PopID();
					break;
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		ImGui::Unindent();
	};

	DrawList("Directional", DirectionalLights, ELightType::Directional);
	DrawList("Point",       PointLights,       ELightType::Point);
	DrawList("Spotlight",   SpotLights,        ELightType::Spotlight);

	ImGui::Unindent();
	ImGui::PopID();
}

JsonLoader LightManager::GetJsonData() const
{
	JsonLoader saver{};

	auto WriteVec3 = [&](JsonLoader& node, const char* k, const DirectX::XMFLOAT3& v)
	{
		node[k]["X"] = v.x;
		node[k]["Y"] = v.y;
		node[k]["Z"] = v.z;
	};

	auto WriteLight = [&](JsonLoader& node, const LightCPU& l)
	{
		WriteVec3(node, "Strength",  l.Strength);
		node["FalloffStart"] = l.FalloffStart;

		WriteVec3(node, "Direction", l.Direction);
		node["FalloffEnd"] = l.FalloffEnd;

		WriteVec3(node, "Position",  l.Position);
		node["SpotPower"] = l.SpotPower;
	};

	auto WriteList = [&](const char* listKey, const std::vector<LightCPU>& list)
	{
		auto& root = saver["Lights"][listKey];
		root["Count"] = static_cast<int>(list.size());

		for (size_t i = 0; i < list.size(); ++i)
		{
			const std::string itemKey = "Item_" + std::to_string(i);
			auto& n = root[itemKey];
			WriteLight(n, list[i]);
		}
	};

	WriteList("Directional", DirectionalLights);
	WriteList("Point",       PointLights);
	WriteList("Spot",        SpotLights);

	return saver;
}

void LightManager::LoadJsonData(const JsonLoader& data)
{
	auto ReadVec3 = [](const JsonLoader& node, const char* key, const DirectX::XMFLOAT3& def) -> DirectX::XMFLOAT3
	{
		if (!node.Has(key)) return def;

		const auto& v = node[key];
		if (!v.Has("X") || !v.Has("Y") || !v.Has("Z")) return def;

		return DirectX::XMFLOAT3(
			v["X"].AsFloat(def.x),
			v["Y"].AsFloat(def.y),
			v["Z"].AsFloat(def.z)
		);
	};

	auto ReadLight = [&](const JsonLoader& node) -> LightCPU
	{
		LightCPU l{};

		l.Strength     = ReadVec3(node, "Strength",  DirectX::XMFLOAT3(1.f, 1.f, 1.f));
		l.FalloffStart = node.Has("FalloffStart") ? node["FalloffStart"].AsFloat(0.0f) : 0.0f;

		l.Direction    = ReadVec3(node, "Direction", DirectX::XMFLOAT3(0.f, -1.f, 0.f));
		l.FalloffEnd   = node.Has("FalloffEnd") ? node["FalloffEnd"].AsFloat(0.0f) : 0.0f;

		l.Position     = ReadVec3(node, "Position",  DirectX::XMFLOAT3(0.f, 0.f, 0.f));
		l.SpotPower    = node.Has("SpotPower") ? node["SpotPower"].AsFloat(64.0f) : 64.0f;

		return l;
	};

	auto ReadList = [&](const char* listKey, std::vector<LightCPU>& outList)
	{
		outList.clear();

		if (!data.Has("Lights")) return;
		const auto& lightsRoot = data["Lights"];

		if (!lightsRoot.Has(listKey)) return;
		const auto& root = lightsRoot[listKey];

		const int count = root.Has("Count") ? root["Count"].AsInt(0) : 0;
		if (count <= 0) return;

		outList.reserve(static_cast<size_t>(count));

		for (int i = 0; i < count; ++i)
		{
			const std::string itemKey = "Item_" + std::to_string(i);
			if (!root.Has(itemKey)) continue;

			outList.push_back(ReadLight(root[itemKey]));
		}
	};

	ReadList("Directional", DirectionalLights);
	ReadList("Point",       PointLights);
	ReadList("Spot",        SpotLights);
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

void Material::ImguiView()
{
	ImGui::PushID(this);

	if (const char* headerLabel = Name.empty() ? "Material" : Name.c_str();
		ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		// Identity / indices
		{
			char nameBuffer[128]{};
			if (!Name.empty())
				std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", Name.c_str());

			if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
				Name = nameBuffer;

			// Indices / state
			ImGui::InputScalar("SRV Heap Index", ImGuiDataType_U32, &SrvHeapIndex);
			ImGui::InputScalar("Num Frames", ImGuiDataType_U32, &FrameCount);
		}

		ImGui::Separator();

		// Constant data
		{
			ImGui::TextUnformatted("Constant Data");

			ImGui::ColorEdit4("Diffuse Albedo", &Config.DiffuseAlbedo.x);

			ImGui::ColorEdit3("Fresnel R0", &Config.FresnelR0.x);
			ImGui::SliderFloat("Roughness", &Config.Roughness, 0.0f, 1.0f);

			if (ImGui::TreeNodeEx("Mat Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				float* m = &Config.MatTransform._11;

				// Display/edit rows (row-major in memory for XMFLOAT4X4)
				ImGui::DragFloat4("Row 0", m + 0, 0.01f);
				ImGui::DragFloat4("Row 1", m + 4, 0.01f);
				ImGui::DragFloat4("Row 2", m + 8, 0.01f);
				ImGui::DragFloat4("Row 3", m + 12, 0.01f);

				// Quick buttons
				if (ImGui::Button("Identity"))
				{
					Config.MatTransform = DirectX::XMFLOAT4X4(
						1.f, 0.f, 0.f, 0.f,
						0.f, 1.f, 0.f, 0.f,
						0.f, 0.f, 1.f, 0.f,
						0.f, 0.f, 0.f, 1.f
					);
				}
				ImGui::SameLine();
				if (ImGui::Button("Zero"))
				{
					Config.MatTransform = DirectX::XMFLOAT4X4(
						0.f, 0.f, 0.f, 0.f,
						0.f, 0.f, 0.f, 0.f,
						0.f, 0.f, 0.f, 0.f,
						0.f, 0.f, 0.f, 0.f
					);
				}

				ImGui::TreePop();
			}
		}

		ImGui::Unindent();
	}

	ImGui::PopID();
}

void Material::InitPixelConstantBuffer(
	const std::uint32_t frameCount,
	ID3D12Device *device,
	framework::DescriptorHeap &heap)
{
	constexpr std::uint32_t resourceSize = (sizeof(Material::MaterialConstants) + 255u) & ~255u;
	const std::uint32_t totalSize		 = resourceSize * frameCount;
	if (!totalSize) return;

	D3D12_RESOURCE_DESC resource{};
	resource.Alignment			= 0u;
	resource.DepthOrArraySize	= 1u;
	resource.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	resource.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resource.Format				= DXGI_FORMAT_UNKNOWN;
	resource.Height				= 1u;
	resource.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource.MipLevels			= 1u;
	resource.SampleDesc.Count	= 1u;
	resource.SampleDesc.Quality = 0u;
	resource.Width				= totalSize;

	D3D12_HEAP_PROPERTIES properties{};
	properties.Type					= D3D12_HEAP_TYPE_UPLOAD;
	properties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CreationNodeMask		= 1u;
	properties.VisibleNodeMask		= 1u;

	THROW_DX_IF_FAILS(device->CreateCommittedResource(
			&properties, D3D12_HEAP_FLAG_NONE, &resource,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&PixelConstantBuffer)
		));

	BYTE* mapped{ nullptr };
	THROW_DX_IF_FAILS(PixelConstantBuffer->Map(0u, nullptr,
		reinterpret_cast<void**>(&mapped)));

	std::uint32_t offset = 0;
	std::uint32_t startIndex = heap.Allocate(frameCount);
	for (int i = 0; i < frameCount; i++)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = heap.GetGPUHandle(startIndex);
		BasePCBHandle.push_back(gpuHandle); //~ one of each iter

		const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap.GetCPUHandle(startIndex);
		D3D12_CONSTANT_BUFFER_VIEW_DESC view{};
		view.BufferLocation = PixelConstantBuffer->GetGPUVirtualAddress() + offset;
		view.SizeInBytes	= resourceSize;

		device->CreateConstantBufferView(&view, cpuHandle);
		PixelConstantMap.Mapped.push_back(mapped + offset);
		PixelConstantMap.Views .push_back(view);
		PixelConstantMap.GPUHandle.push_back(gpuHandle);

		//~ step
		offset += resourceSize;
		++startIndex;
	}
}
