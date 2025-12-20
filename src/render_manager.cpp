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
#include "framework/render_manager/render_manager.h"
#include "framework/windows_manager/windows_manager.h"

#include "framework/exception/dx_exception.h"
#include "utility/logger.h"

#include <dxgidebug.h>

framework::DxRenderManager::DxRenderManager(DxWindowsManager* windows)
	: Windows(windows)
{}

framework::DxRenderManager::~DxRenderManager()
{}

bool framework::DxRenderManager::Initialize()
{
	if (!CreateDirect3D()) return false;
	if (!CreateSwapChain()) return false;

	CreateRenderTarget();
	CreateDepthStencil();
	CreateViewport	  ();

	return true;
}

bool framework::DxRenderManager::Release()
{
	return true;
}

void framework::DxRenderManager::AttachWindows(DxWindowsManager *windows)
{
	Windows = windows;
}

D3D12_CPU_DESCRIPTOR_HANDLE framework::DxRenderManager::GetDSVBaseHandle() const
{
	return DsvHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE framework::DxRenderManager::GetBackBufferHandle(const std::uint32_t index) const
{
	const std::uint32_t safe = index % BackBufferCount;
	return RtvDescriptorHandle[safe];
}

ID3D12Resource * framework::DxRenderManager::GetBackBuffer(const std::uint32_t index) const
{
	const std::uint32_t safe = index % BackBufferCount;
	return SwapChainBuffer[safe].Get();
}

bool framework::DxRenderManager::CreateDirect3D()
{
	CreateFactory();
	CreateAdapter();
	if (!CreateDevice			()) return false;
	if (!CreateMSaa				()) return false;
	if (!CreateFence			()) return false;

	CreateCommandQueue	  ();
	CreateCommandAllocator();
	CreateCommandList	  ();

	CreateDepthHeap	  ();
	CreateRTVHeap	  ();
	CreateSamplerHeap ();
	CreateSrvHeap	  ();

	return true;
}

bool framework::DxRenderManager::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC desc{};
	desc.Flags = 0u;
	desc.BufferDesc.Format = BackBufferFormat;
	desc.BufferDesc.Height = Windows->GetWindowsHeight();
	desc.BufferDesc.Width = Windows->GetWindowsWidth();
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.BufferDesc.RefreshRate.Denominator = 1u;
	desc.BufferDesc.RefreshRate.Numerator = 60u;
	desc.SampleDesc.Count = 1u;
	desc.SampleDesc.Quality = 0u;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = BackBufferCount;
	desc.OutputWindow = Windows->GetWindowsHandle();
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Windowed = true;

	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	HRESULT hr = m_dxgiFactory->CreateSwapChain(
		GfxQueue.Get(),
		&desc,
		&swapChain);

	if (FAILED(hr))
	{
		logger::error("Failed to create swap chain");
		THROW_DX_IF_FAILS(hr);
		return false;
	}

	hr = swapChain.As(&SwapChain);
	if (FAILED(hr))
	{
		logger::error("Failed to convert to swap chain - 3");
		THROW_DX_IF_FAILS(hr);
		return false;
	}
	return true;
}

bool framework::DxRenderManager::CreateFactory()
{
	THROW_DX_IF_FAILS(CreateDXGIFactory2(0u, IID_PPV_ARGS(&m_dxgiFactory)));
	logger::success("Factory Created!");
	return true;
}

bool framework::DxRenderManager::CreateAdapter()
{
	UINT index = 0u;
	Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
	while (m_dxgiFactory->EnumAdapters(index, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter3;
		if (const HRESULT hr = adapter.As(&adapter3); SUCCEEDED(hr))
		{
			m_adapters.emplace_back(adapter3);
		}else logger::error("Failed to convert to adapter3!");
		++index;
	}
	logger::info("Adapters Found: {}", m_adapters.size());
	LogAdapters();
	return true;
}

bool framework::DxRenderManager::CreateMSaa()
{
	uint32_t count = 1;
	while (count <= 32u)
	{
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS feature{};
		feature.Flags			 = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		feature.SampleCount		 = count;
		feature.Format			 = BackBufferFormat;
		feature.NumQualityLevels = count - 1u;
		const HRESULT hr = Device->CheckFeatureSupport(
			D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
			&feature,
			sizeof(feature));
		count <<= 1;
		if (SUCCEEDED(hr))
		{
			SupportedMSAA msaa{};
			msaa.quality = feature.NumQualityLevels;
			msaa.samples = feature.SampleCount;
			m_supportedMSAA.emplace_back(msaa);

			logger::success("MSAA Supported: {}", msaa.samples);
		}
	}
	if (m_supportedMSAA.empty()) logger::error("No MSAA supported!");
	return !m_supportedMSAA.empty();
}

bool framework::DxRenderManager::CreateDevice()
{
#if defined(DEBUG) || defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	THROW_DX_IF_FAILS(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
#endif

#if defined(_DEBUG)
	{
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(
				DXGI_DEBUG_ALL,
				DXGI_DEBUG_RLO_ALL
			);
		}
	}
#endif


	D3D_FEATURE_LEVEL levels[]
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	auto fn = [](const D3D_FEATURE_LEVEL level)
	{
		switch (level)
		{
			case D3D_FEATURE_LEVEL_12_2:
				return "D3D_FEATURE_LEVEL_12_2";
			case D3D_FEATURE_LEVEL_12_1:
				return "D3D_FEATURE_LEVEL_12_1";
			case D3D_FEATURE_LEVEL_12_0:
				return "D3D_FEATURE_LEVEL_12_0";
			case D3D_FEATURE_LEVEL_11_1:
				return "D3D_FEATURE_LEVEL_11_1";
			case D3D_FEATURE_LEVEL_11_0:
				return "D3D_FEATURE_LEVEL_11_0";
			default: break;
		}
		return "NONE";
	};

	for (const auto& level : levels)
	{
		if (const HRESULT hr = D3D12CreateDevice(m_adapters[0].Get(), level, IID_PPV_ARGS(&Device)); SUCCEEDED(hr))
		{
			logger::success("Device Created! with {}", fn(level));

			HeapSizes.RTV		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			HeapSizes.DSV		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			HeapSizes.SRV		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			HeapSizes.Sampler = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			return true;
		}
	}
	return false;
}

bool framework::DxRenderManager::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	queueDesc.Type		= D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.NodeMask	= 0u;
	THROW_DX_IF_FAILS(Device->CreateCommandQueue(&queueDesc,
					  IID_PPV_ARGS(&GfxQueue)));
	logger::success("Created Command Queue!");
	return true;
}

bool framework::DxRenderManager::CreateCommandAllocator()
{
	THROW_DX_IF_FAILS(Device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&GfxAllocator)
	));
	logger::success("Created Command Allocator!");
	return true;
}

bool framework::DxRenderManager::CreateCommandList()
{
	THROW_DX_IF_FAILS(
		Device->CreateCommandList(
			0u,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			GfxAllocator.Get(),
			nullptr, IID_PPV_ARGS(&GfxCmd))
	);

	(void)GfxCmd->Close();
	logger::success("Created Command List!");
	return true;
}

bool framework::DxRenderManager::CreateFence()
{
	const HRESULT hr = Device->CreateFence(0u,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&Fence));

	if (FAILED(hr))
	{
		logger::error("Failed to create fence!");
		return false;
	}
	logger::success("Created Fences!");
	return true;
}

bool framework::DxRenderManager::CreateDepthHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	desc.NumDescriptors = 16u;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0u;
	THROW_DX_IF_FAILS(Device->CreateDescriptorHeap(&desc,
		IID_PPV_ARGS(&DsvHeap)));
	return true;
}

bool framework::DxRenderManager::CreateRTVHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.NumDescriptors = 16u;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0u;
	THROW_DX_IF_FAILS(Device->CreateDescriptorHeap(&desc,
		IID_PPV_ARGS(&RtvHeap)));
	logger::success("Created rtv Descriptor Heap!");
	return true;
}

bool framework::DxRenderManager::CreateSamplerHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	desc.NumDescriptors = 16u;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0u;
	THROW_DX_IF_FAILS(Device->CreateDescriptorHeap(&desc,
		IID_PPV_ARGS(&SamplerHeap)));
	logger::success("Created sampler Descriptor Heap!");
	return true;
}

bool framework::DxRenderManager::CreateSrvHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 2048u;
	desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask		= 0u;
	THROW_DX_IF_FAILS(Device->CreateDescriptorHeap(&desc,
		IID_PPV_ARGS(&SrvHeap)));
	logger::success("Created Srv Descriptor Heap!");
	return true;
}

bool framework::DxRenderManager::CreateRenderTarget()
{
	const auto base = RtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < BackBufferCount; i++)
	{
		THROW_DX_IF_FAILS(SwapChain->GetBuffer(i,
			IID_PPV_ARGS(&SwapChainBuffer[i])));

		D3D12_CPU_DESCRIPTOR_HANDLE h = base;
		h.ptr += static_cast<SIZE_T>(i) *static_cast<SIZE_T>(HeapSizes.RTV);

		Device->CreateRenderTargetView(SwapChainBuffer[i].Get(),
			nullptr, h);

		RtvDescriptorHandle[i] = h;
	}

	logger::success("Created Render Target Buffer and View!");
	return true;
}

bool framework::DxRenderManager::CreateDepthStencil()
{
	D3D12_RESOURCE_DESC resource{};
	resource.Flags  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resource.Format = DsvFormat;
	resource.Height = Windows->GetWindowsHeight();
	resource.Width = Windows->GetWindowsWidth();
	resource.Alignment = 0u;
	resource.DepthOrArraySize = 1u;
	resource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resource.MipLevels = 1u;
	resource.SampleDesc.Count = 1u;
	resource.SampleDesc.Quality = 0u;
	resource.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_CLEAR_VALUE clearFlag{};
	clearFlag.Format = DsvFormat;
	clearFlag.DepthStencil.Depth   = 1.f;
	clearFlag.DepthStencil.Stencil = 0u;

	D3D12_HEAP_PROPERTIES properties{};
	properties.Type					= D3D12_HEAP_TYPE_DEFAULT;
	properties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.VisibleNodeMask		= 1u;
	properties.CreationNodeMask		= 1u;

	THROW_DX_IF_FAILS(Device->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&resource,
		D3D12_RESOURCE_STATE_COMMON,
		&clearFlag,
		IID_PPV_ARGS(&DepthStencilBuffer)
		));

	Device->CreateDepthStencilView(
		DepthStencilBuffer.Get(),
		nullptr,
		GetDSVBaseHandle());

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = DepthStencilBuffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	GfxCmd->ResourceBarrier(1u, &barrier);

	logger::success("Created Depth Stencil View and Buffer!");
	return true;
}

void framework::DxRenderManager::CreateViewport()
{
	Viewport.Width = Windows->GetWindowsWidth();
	Viewport.Height = Windows->GetWindowsHeight();
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;

	const long width_half = Windows->GetWindowsWidth() / 2;
	const long height_half = Windows->GetWindowsHeight() / 2;
	ScissorRect = {0, 0, width_half , height_half };

	logger::success("Created Scissor and Viewport!");
}

void framework::DxRenderManager::LogAdapters() const
{
	UINT count = 1;
	for (auto& adapter: m_adapters)
	{
		DXGI_ADAPTER_DESC2 desc;
		const HRESULT hr = adapter->GetDesc2(&desc);
		++count;
		if (FAILED(hr)) continue;

		std::wstring w_str(desc.Description);
		auto str = std::string(w_str.begin(), w_str.end());
		logger::info("Adapter {}, Description: {}",
			count, str);
		LogMonitors(adapter.Get());
	}
}

void framework::DxRenderManager::LogMonitors(IDXGIAdapter3* adapter) const
{
	Microsoft::WRL::ComPtr<IDXGIOutput> output;
	UINT count = 0;
	while(adapter->EnumOutputs(count, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		const HRESULT hr = output->GetDesc(&desc);
		++count;
		if (FAILED(hr)) continue;
		std::wstring w_str(desc.DeviceName);
		auto str = std::string(w_str.begin(), w_str.end());
		logger::info("Output Description: {}",str);
	}
}
