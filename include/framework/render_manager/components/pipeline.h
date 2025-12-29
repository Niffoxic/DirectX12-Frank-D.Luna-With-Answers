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
#ifndef DIRECTX12_PIPELINE_H
#define DIRECTX12_PIPELINE_H

#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl/client.h>

#include "framework/render_manager/render_manager.h"

enum class EFillMode : std::uint8_t
{
    WireFrame = 0,
    Solid,
};

inline static D3D12_FILL_MODE GetFillMode(const EFillMode mode)
{
    switch (mode)
    {
    case EFillMode::WireFrame: return D3D12_FILL_MODE_WIREFRAME;
    case EFillMode::Solid:     return D3D12_FILL_MODE_SOLID;
    default:                   return D3D12_FILL_MODE_SOLID;
    }
}

inline static std::string ToString(const EFillMode mode)
{
    switch (mode)
    {
    case EFillMode::WireFrame: return "WireFrame";
    case EFillMode::Solid:     return "Solid";
    default:                  return "Unknown";
    }
}

enum class ECullMode : std::uint8_t
{
    None,
    Front,
    Back
};

inline static D3D12_CULL_MODE GetCullMode(const ECullMode mode)
{
    switch (mode)
    {
    case ECullMode::None:  return D3D12_CULL_MODE_NONE;
    case ECullMode::Front: return D3D12_CULL_MODE_FRONT;
    case ECullMode::Back:  return D3D12_CULL_MODE_BACK;
    default:               return D3D12_CULL_MODE_BACK;
    }
}

inline static std::string ToString(const ECullMode mode)
{
    switch (mode)
    {
    case ECullMode::None:  return "None";
    case ECullMode::Front: return "Front";
    case ECullMode::Back:  return "Back";
    default:               return "Unknown";
    }
}

namespace framework
{
	class Pipeline
	{
	public:
		 Pipeline();
		~Pipeline() = default;

		void Initialize(framework::DxRenderManager* renderManager);

		void SetRootSignature	(ID3D12RootSignature* rg);
		void SetVertexShader	(D3D12_SHADER_BYTECODE bytecode);
		void SetPixelShader		(D3D12_SHADER_BYTECODE bytecode);
		void SetRasterizerState	(const D3D12_RASTERIZER_DESC& desc);
		void SetBlendState		(const D3D12_BLEND_DESC& desc);
		void SetInputLayout		(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputs);

		void SetCullMode(ECullMode mode);
		void SetFillMode(EFillMode mode);

		void ImguiView();

		ID3D12PipelineState* GetNative() const;

		bool IsInitialized() const;
		bool IsDirty	  () const;

	private:
		bool m_bInitialized	{ false };
		bool m_bDirty		{ false };
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState{ nullptr };
		//~ non owning
		ID3D12RootSignature*    m_pRootSignature{ nullptr };
		D3D12_SHADER_BYTECODE   m_vertexShader{};
		D3D12_SHADER_BYTECODE   m_pixelShader {};
		D3D12_RASTERIZER_DESC   m_rasterDesc  {};
		D3D12_BLEND_DESC	    m_blendDesc	  {};
		D3D12_INPUT_LAYOUT_DESC m_inputLayout {};
		std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputElements;

		//~ configs
		EFillMode m_fillMode{ EFillMode::Solid };
		ECullMode m_cullMode{ ECullMode::Back  };
	};
} // namespace framework

#endif //DIRECTX12_PIPELINE_H