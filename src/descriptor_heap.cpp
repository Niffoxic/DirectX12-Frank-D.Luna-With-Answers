//
// Created by Niffoxic (Aka Harsh Dubey) on 12/28/2025.
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
#include "framework/exception/dx_exception.h"
#include "framework/render_manager/components/decriptor_heap.h"
#include "utility/logger.h"

#include "imgui.h"

using namespace framework;

void DescriptorHeap::Initialize(const InitDescriptorHeap &desc)
{
    if (IsValid()) return;

    m_bInitialized = false;

    m_allocatedMap.clear();
    m_allocatedMap.resize(desc.AllocationSize, false);

    m_allocationMaxSize = desc.AllocationSize;
    m_allocationCount   = 0u;

    D3D12_DESCRIPTOR_HEAP_DESC heap{};
    heap.Flags          = desc.Flags;
    heap.NodeMask       = 0u;
    heap.Type           = desc.Type;
    heap.NumDescriptors = desc.AllocationSize;

    THROW_DX_IF_FAILS(desc.pDevice->CreateDescriptorHeap(
        &heap,
        IID_PPV_ARGS(&m_heap))
    );

    m_heapIncrement        = desc.pDevice->GetDescriptorHandleIncrementSize(desc.Type);
    m_szDescriptorHeapName = desc.szDebugName;

    const auto w_name = std::wstring(desc.szDebugName.begin(), desc.szDebugName.end());
    if (const HRESULT hr = m_heap->SetName(w_name.c_str()); FAILED(hr))
    {
        logger::warning("Failed to set heap name {}", m_szDescriptorHeapName);
    }

    m_bInitialized = true;
}

std::uint32_t DescriptorHeap::Allocate(const std::uint32_t &allocCounts)
{
    if (!IsValid()) THROW_MSG("DescriptorHeap not initialized");
    if (allocCounts == 0u) THROW_MSG("Allocate called with 0 descriptors");

    if (m_allocationCount + allocCounts > m_allocationMaxSize)
    {
        THROW_MSG("Allocation count exceeded");
    }

    int left  = -1;
    int right = -1;

    const int maxSize = static_cast<int>(m_allocationMaxSize);
    const int need    = static_cast<int>(allocCounts);

    for (int i = 0; i < maxSize; ++i)
    {
        if (!m_allocatedMap[static_cast<size_t>(i)])
        {
            if (left == -1) left = i;
            right = i;

            if ((right - left + 1) == need)
                break; // found a contiguous range
        }
        else
        {
            left  = -1;
            right = -1;
        }
    }

    if (left == -1 || right == -1 || (right - left + 1) < need)
    {
        THROW_MSG("Not enough contiguous space in the heap");
    }

    for (int i = left; i <= right; ++i)
    {
        m_allocatedMap[static_cast<size_t>(i)] = true;
    }

    m_allocationCount += allocCounts;
    return static_cast<std::uint32_t>(left);
}

void DescriptorHeap::Deallocate(const std::uint32_t index, const std::uint32_t count)
{
    if (!IsValid()) return;
    if (count == 0u) return;

    if (index >= m_allocationMaxSize) return;
    if (index + count > m_allocationMaxSize) return;

    std::uint32_t freed = 0u;

    for (std::uint32_t i = index; i < index + count; ++i)
    {
        const size_t idx = static_cast<size_t>(i);
        if (m_allocatedMap[idx])
        {
            m_allocatedMap[idx] = false;
            ++freed;
        }
    }

    if (freed > m_allocationCount) m_allocationCount = 0u;
    else m_allocationCount -= freed;
}

ID3D12DescriptorHeap * DescriptorHeap::GetNative() const
{
    return m_heap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandle(std::uint32_t index) const
{
    auto handle = m_heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * m_heapIncrement;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandle(std::uint32_t index) const
{
    auto handle = m_heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += index * m_heapIncrement;
    return handle;
}

bool DescriptorHeap::IsValid() const
{
    return (m_heap != nullptr) && m_bInitialized;
}

void DescriptorHeap::ImguiView() const
{
    ImGui::PushID(this); // unique per heap instance

    const std::string header =
        "Descriptor Heap: " + m_szDescriptorHeapName + "###DescriptorHeapHeader";

    if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();

        ImGui::TextUnformatted("State");
        ImGui::Separator();
        ImGui::BulletText("Initialized: %s", m_bInitialized ? "true" : "false");
        ImGui::BulletText("Heap Increment: %u", m_heapIncrement);
        ImGui::BulletText("Max Size: %u", m_allocationMaxSize);
        ImGui::BulletText("Allocated Count: %u", m_allocationCount);

        ImGui::Spacing();
        ImGui::TextUnformatted("Descriptor Heap");
        ImGui::Separator();
        ImGui::Text("Name: %s", m_szDescriptorHeapName.c_str());

        ImGui::Spacing();
        ImGui::TextUnformatted("Allocation Map");
        ImGui::Separator();

        const std::uint32_t mapSize = static_cast<std::uint32_t>(m_allocatedMap.size());
        ImGui::Text("Bits: %u", mapSize);

        ImGui::PushID("AllocatedMap");

        if (!m_allocatedMap.empty())
        {
            constexpr int columns = 32;

            if (ImGui::BeginTable("AllocBitsTable", columns, ImGuiTableFlags_SizingFixedFit))
            {
                for (std::uint32_t i = 0; i < mapSize; ++i)
                {
                    ImGui::TableNextColumn();
                    ImGui::PushID(static_cast<int>(i));

                    bool bit = m_allocatedMap[static_cast<size_t>(i)];

                    ImGui::BeginDisabled(true);
                    ImGui::Checkbox("##bit", &bit);
                    ImGui::EndDisabled();

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Index: %u\nAllocated: %s", i, bit ? "true" : "false");
                    }

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextDisabled("(empty)");
        }

        ImGui::PopID();
        ImGui::Unindent();
    }

    ImGui::PopID();
}

std::uint32_t DescriptorHeap::GetAllocatedCounts() const
{
	return m_allocationCount;
}

std::uint32_t DescriptorHeap::GetAllocationSize() const
{
	return m_allocationMaxSize;
}
