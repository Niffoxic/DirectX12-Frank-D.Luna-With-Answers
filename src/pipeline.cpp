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
#include "framework/render_manager/components/pipeline.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include "imgui.h"
#include "framework/exception/dx_exception.h"

using namespace framework;

static D3D12_DEPTH_STENCIL_DESC MakeDefaultDepthStencil()
{
	D3D12_DEPTH_STENCIL_DESC depth{};
	depth.DepthEnable      = TRUE;
	depth.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
	depth.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
	depth.StencilEnable    = FALSE;
	depth.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;
	depth.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	return depth;
}

Pipeline::Pipeline()
{
	m_fillMode = EFillMode::Solid;
	m_cullMode = ECullMode::Back;

	m_rasterDesc.FillMode = GetFillMode(m_fillMode);
	m_rasterDesc.CullMode = GetCullMode(m_cullMode);

	// Default raster
	m_rasterDesc = {};
	m_rasterDesc.FillMode              = GetFillMode(m_fillMode);
	m_rasterDesc.CullMode              = GetCullMode(m_cullMode);
	m_rasterDesc.FrontCounterClockwise = FALSE;
	m_rasterDesc.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
	m_rasterDesc.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	m_rasterDesc.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	m_rasterDesc.DepthClipEnable       = TRUE;
	m_rasterDesc.MultisampleEnable     = FALSE;
	m_rasterDesc.AntialiasedLineEnable = FALSE;
	m_rasterDesc.ForcedSampleCount     = 0;
	m_rasterDesc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Default blend
	m_blendDesc = {};
	m_blendDesc.AlphaToCoverageEnable  = FALSE;
	m_blendDesc.IndependentBlendEnable = FALSE;

	auto& rt = m_blendDesc.RenderTarget[0];
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

	// Input layout empty by default
	m_inputLayout = {};
}

void Pipeline::Initialize(framework::DxRenderManager *renderManager)
{
	assert(renderManager && "renderManager is null");
	assert(renderManager->Device && "renderManager->Device is null");
	assert(m_pRootSignature && "RootSignature not set");
	assert(m_vertexShader.pShaderBytecode && m_vertexShader.BytecodeLength > 0 && "VS not set");

	m_bInitialized = true;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.Flags    = D3D12_PIPELINE_STATE_FLAG_NONE;
	desc.NodeMask = 0u;

	desc.InputLayout = m_inputLayout;

	desc.pRootSignature = m_pRootSignature;
	desc.VS = m_vertexShader;
	desc.PS = m_pixelShader;

	desc.RasterizerState   = m_rasterDesc;
	desc.BlendState        = m_blendDesc;
	desc.DepthStencilState = MakeDefaultDepthStencil();
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	desc.NumRenderTargets = 1u;
	desc.RTVFormats[0]    = renderManager->BackBufferFormat;
	desc.DSVFormat        = renderManager->DsvFormat;

	desc.SampleMask = UINT_MAX;
	desc.SampleDesc.Count   = 1;
	desc.SampleDesc.Quality = 0;

	// Create / recreate PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	THROW_DX_IF_FAILS(renderManager->Device->CreateGraphicsPipelineState(
		&desc, IID_PPV_ARGS(&pso)));

	m_pPipelineState = std::move(pso);
	m_bDirty = false;
}

void Pipeline::SetRootSignature(ID3D12RootSignature *rg)
{
	if (m_pRootSignature == rg) return;
	m_pRootSignature = rg;
	m_bDirty = true;
}

void Pipeline::SetVertexShader(D3D12_SHADER_BYTECODE bytecode)
{
	if (m_vertexShader.pShaderBytecode == bytecode.pShaderBytecode &&
	m_vertexShader.BytecodeLength  == bytecode.BytecodeLength)
		return;

	m_vertexShader = bytecode;
	m_bDirty = true;
}

void Pipeline::SetPixelShader(D3D12_SHADER_BYTECODE bytecode)
{
	if (m_pixelShader.pShaderBytecode == bytecode.pShaderBytecode &&
	m_pixelShader.BytecodeLength  == bytecode.BytecodeLength)
		return;

	m_pixelShader = bytecode;
	m_bDirty = true;
}

void Pipeline::SetRasterizerState(const D3D12_RASTERIZER_DESC &desc)
{
	m_rasterDesc = desc;

	switch (desc.FillMode)
	{
		case D3D12_FILL_MODE_WIREFRAME: m_fillMode = EFillMode::WireFrame; break;
		case D3D12_FILL_MODE_SOLID:     m_fillMode = EFillMode::Solid;     break;
		default:                        m_fillMode = EFillMode::Solid;     break;
	}

	switch (desc.CullMode)
	{
		case D3D12_CULL_MODE_NONE:  m_cullMode = ECullMode::None;  break;
		case D3D12_CULL_MODE_FRONT: m_cullMode = ECullMode::Front; break;
		case D3D12_CULL_MODE_BACK:  m_cullMode = ECullMode::Back;  break;
		default:                    m_cullMode = ECullMode::Back;  break;
	}

	m_bDirty = true;
}

void Pipeline::SetBlendState(const D3D12_BLEND_DESC &desc)
{
	m_blendDesc = desc;
	m_bDirty = true;
}

void Pipeline::SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC> &inputs)
{
	m_inputElements					 = inputs;
	m_inputLayout.NumElements        = static_cast<UINT>(m_inputElements.size());
	m_inputLayout.pInputElementDescs = m_inputElements.empty() ? nullptr : m_inputElements.data();
	m_bDirty = true;
}

void Pipeline::SetCullMode(const ECullMode mode)
{
	if (m_cullMode == mode) return;

	m_cullMode = mode;
	m_rasterDesc.CullMode = GetCullMode(mode);

	m_bDirty = true;
}

void Pipeline::SetFillMode(const EFillMode mode)
{
	if (m_fillMode == mode) return;

	m_fillMode = mode;
	m_rasterDesc.FillMode = GetFillMode(mode);

	m_bDirty = true;
}

static bool ComboFillMode(const char* label, EFillMode& v)
{
	constexpr EFillMode modes[] = { EFillMode::WireFrame, EFillMode::Solid };
	const std::string preview = ToString(v);
	bool changed = false;

	if (ImGui::BeginCombo(label, preview.c_str()))
	{
		for (auto m : modes)
		{
			const bool selected = (m == v);
			if (ImGui::Selectable(ToString(m).c_str(), selected))
			{
				v = m;
				changed = true;
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return changed;
}

static bool ComboCullMode(const char* label, ECullMode& v)
{
	constexpr ECullMode modes[] = { ECullMode::None, ECullMode::Front, ECullMode::Back };
	const std::string preview = ToString(v);
	bool changed = false;

	if (ImGui::BeginCombo(label, preview.c_str()))
	{
		for (auto m : modes)
		{
			const bool selected = (m == v);
			if (ImGui::Selectable(ToString(m).c_str(), selected))
			{
				v = m;
				changed = true;
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return changed;
}

void Pipeline::ImguiView()
{
	ImGui::PushID(this);

	if (ImGui::CollapsingHeader("Pipeline", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		ImGui::Text("Initialized: %s", m_bInitialized ? "Yes" : "No");
		ImGui::Text("Dirty:       %s", m_bDirty ? "Yes" : "No");
		ImGui::Text("PSO:         %s", m_pPipelineState ? "Built" : "NULL");

		ImGui::Separator();

		{
			if (EFillMode fm = m_fillMode; ComboFillMode("Fill Mode", fm))
				SetFillMode(fm);

			if (ECullMode cm = m_cullMode; ComboCullMode("Cull Mode", cm))
				SetCullMode(cm);
		}

		ImGui::Unindent();
	}

	ImGui::PopID();
}

ID3D12PipelineState * Pipeline::GetNative() const
{
	return m_pPipelineState.Get();
}

bool Pipeline::IsInitialized() const
{
	return m_bInitialized;
}

bool Pipeline::IsDirty() const
{
	return m_bDirty;
}
