//
// Created by Niffoxic (Aka Harsh Dubey) on 12/18/2025.
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

#ifndef DIRECTX12_RENDER_MANAGER_H
#define DIRECTX12_RENDER_MANAGER_H

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>
#include <functional>
#include <unordered_map>

namespace framework
{
	class DxWindowsManager;

	class DxRenderManager
	{
	public:
		DxRenderManager() = default;
		 explicit DxRenderManager(DxWindowsManager* windows);
		~DxRenderManager();

		DxRenderManager(const DxRenderManager&) = delete;
		DxRenderManager(DxRenderManager&&)		= delete;

		DxRenderManager& operator=(const DxRenderManager&) = delete;
		DxRenderManager& operator=(DxRenderManager&&)	   = delete;

		//~ operation
		bool Initialize();
		bool Release   ();

		void AttachWindows(DxWindowsManager* windows);

		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVBaseHandle() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferHandle(const std::uint32_t index) const;
		ID3D12Resource* GetBackBuffer(const std::uint32_t index) const;

		void IncrementFenceValue();
		void IncrementFrameIndex();

		std::uint64_t GetFenceValue() const;
		std::uint32_t GetFrameIndex() const;

		//~ Helpers
		static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
			const std::wstring& filename,
			const D3D_SHADER_MACRO* defines,
			const std::string& entrypoint,
			const std::string& target
		);

		static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

	private:
		bool CreateDirect3D();
		bool CreateSwapChain();

		//~ d3d creations
		bool CreateFactory			();
		bool CreateAdapter			();
		bool CreateMSaa				();
		bool CreateDevice			();
		bool CreateCommandQueue	    ();
		bool CreateCommandAllocator	();
		bool CreateCommandList		();
		bool CreateFence			();
		bool CreateDepthHeap		();
		bool CreateRTVHeap			();
		bool CreateSamplerHeap		();
		bool CreateSrvHeap			();

		//~ with swap chain
		bool CreateRenderTarget();
		bool CreateDepthStencil();
		void CreateViewport	   ();

		//~ Helpers
		void LogAdapters() const;
		void LogMonitors(IDXGIAdapter3* adapter) const;

	public:
		DxWindowsManager* Windows;

		//~ DirectX related stuff
		Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
		std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter3>> m_adapters;
		Microsoft::WRL::ComPtr<ID3D12Device> Device;
		Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GfxQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GfxAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GfxCmd;

		//~ Heaps
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DsvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SrvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SamplerHeap;

		//~ formats
		struct SupportedMSAA
		{
			std::uint32_t samples;
			std::uint32_t quality;
		};
		std::vector<SupportedMSAA> m_supportedMSAA{};

		struct HeapSize
		{
			std::uint32_t RTV;
			std::uint32_t Sampler;
			std::uint32_t SRV;
			std::uint32_t DSV;
		} HeapSizes{};
		DXGI_FORMAT BackBufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
		static constexpr std::uint32_t BackBufferCount{ 2 };
		Microsoft::WRL::ComPtr<IDXGISwapChain3> SwapChain;

		//~ Render Resources
		Microsoft::WRL::ComPtr<ID3D12Resource>	SwapChainBuffer[BackBufferCount];
		D3D12_CPU_DESCRIPTOR_HANDLE				RtvDescriptorHandle[2]{};
		Microsoft::WRL::ComPtr<ID3D12Resource>	DepthStencilBuffer{};
		DXGI_FORMAT								DsvFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT };
		D3D12_VIEWPORT Viewport{};
		D3D12_RECT     ScissorRect{};
		std::uint32_t  FrameIndex{ 0u };
		std::uint64_t  FenceValue{ 0u };
	};
} // namespace framework

#endif //DIRECTX12_RENDER_MANAGER_H
