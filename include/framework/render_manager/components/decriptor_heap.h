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
#ifndef DIRECTX12_DESCRIPTOR_HEAP_H
#define DIRECTX12_DESCRIPTOR_HEAP_H

#include <cstdint>
#include <d3d12.h>
#include <vector>
#include <wrl/client.h>
#include <string>

namespace framework
{
	struct InitDescriptorHeap
	{
		std::uint32_t				AllocationSize;
		D3D12_DESCRIPTOR_HEAP_TYPE	Type;
		D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
		ID3D12Device*				pDevice;

		std::string szDebugName{ "Default" };
	};

	class DescriptorHeap
	{
	public:
		 DescriptorHeap() = default;
		~DescriptorHeap() = default;

		void Initialize(const InitDescriptorHeap& desc);

		std::uint32_t Allocate(const std::uint32_t& allocCounts = 1u);
		void Deallocate(const std::uint32_t index, const std::uint32_t count = 1u);

		ID3D12DescriptorHeap* GetNative() const;

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(std::uint32_t index) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(std::uint32_t index) const;

		bool IsValid  () const;
		void ImguiView() const;

		std::uint32_t GetAllocatedCounts() const;
		std::uint32_t GetAllocationSize () const;

	private:
		bool m_bInitialized{ false };
		std::vector<bool> m_allocatedMap{};
		std::uint32_t m_heapIncrement	{};

		std::uint32_t m_allocationMaxSize{ 1u };
		std::uint32_t m_allocationCount	 { 0u };

		std::string m_szDescriptorHeapName{ "Default" };

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap{ nullptr};
	};
}

#endif //DIRECTX12_DESCRIPTOR_HEAP_H
