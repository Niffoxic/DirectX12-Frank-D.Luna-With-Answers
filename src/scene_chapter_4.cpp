//
// Created by Niffoxic (Aka Harsh Dubey) on 12/19/2025.
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
#include "application/scene/scene_chapter_4.h"
#include "framework/render_manager/render_manager.h"
#include "framework/exception/dx_exception.h"

#include <d3d12.h>
#include <imgui.h>

#include "utility/json_loader.h"

SceneChapter4::SceneChapter4(framework::DxRenderManager& renderer)
	: IScene(renderer)
{
	LoadData();
	m_waitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

SceneChapter4::~SceneChapter4()
{
	SaveData();
}

bool SceneChapter4::Initialize()
{
	return true;
}

void SceneChapter4::Shutdown()
{
}

void SceneChapter4::FrameBegin(float deltaTime)
{
	ResetEvent(m_waitEvent);
	THROW_DX_IF_FAILS(Render.GfxAllocator->Reset());
	THROW_DX_IF_FAILS(Render.GfxCmd->Reset(
		Render.GfxAllocator.Get(),
		nullptr));

	ID3D12DescriptorHeap* heaps[] = { Render.SrvHeap.Get() };
	Render.GfxCmd->SetDescriptorHeaps(1, heaps);

	const auto frameIndex = Render.GetFrameIndex();
	auto* back = Render.SwapChainBuffer[frameIndex].Get();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource	= back;
	barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_RENDER_TARGET;
	Render.GfxCmd->ResourceBarrier(1u, &barrier);

	const auto dsvHandle = Render.GetDSVBaseHandle(); // initialized on 1st slot;
	const auto rtvHandle = Render.GetBackBufferHandle(frameIndex);

	Render.GfxCmd->ClearDepthStencilView(dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.f, 0u, 0u, nullptr);

	Render.GfxCmd->ClearRenderTargetView(rtvHandle,
			m_colors,
			0u, nullptr);
	Render.GfxCmd->OMSetRenderTargets(
		1u,
		&rtvHandle,
		TRUE,
		&dsvHandle);

	Render.GfxCmd->RSSetScissorRects(1u, &Render.ScissorRect);
	Render.GfxCmd->RSSetViewports(1u, &Render.Viewport);
}

void SceneChapter4::FrameEnd(float deltaTime)
{
	const auto frameIndex = Render.GetFrameIndex();

	ID3D12DescriptorHeap* heaps[] = { Render.SrvHeap.Get() };
	Render.GfxCmd->SetDescriptorHeaps(1u, heaps);

	auto* back = Render.SwapChainBuffer[frameIndex].Get();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource	= back;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_PRESENT;
	Render.GfxCmd->ResourceBarrier(1u, &barrier);

	THROW_DX_IF_FAILS(Render.GfxCmd->Close());
	ID3D12CommandList* lists[]{ Render.GfxCmd.Get() };
	Render.GfxQueue->ExecuteCommandLists(1u, lists);

	THROW_DX_IF_FAILS(Render.SwapChain->Present(0u, 0u));

	Render.IncrementFenceValue();
	const auto fenceValue = Render.GetFenceValue();

	THROW_DX_IF_FAILS(Render.GfxQueue->Signal(Render.Fence.Get(), fenceValue));

	if (Render.Fence->GetCompletedValue() < fenceValue)
	{
		THROW_DX_IF_FAILS(Render.Fence->SetEventOnCompletion(
			fenceValue,
			m_waitEvent));
		WaitForSingleObject(m_waitEvent, INFINITE);
	}

	Render.IncrementFrameIndex();
}

void SceneChapter4::ImguiView(float deltaTime)
{
	(void)deltaTime;
	ImGui::Begin("Chapter 4 Settings");
	ImGui::Text("Clear Color");
	ImGui::ColorEdit4("Color", m_colors);
	ImGui::End();
}

void SceneChapter4::LoadData()
{
	JsonLoader loader{};
	loader.Load("save/chapter_4.json");

	if (!loader.IsValid())
		return;

	if (!loader.Contains("Color"))
		return;

	auto& color = loader["Color"];

	if (color.Contains("R")) m_colors[0] = color["R"].AsFloat();
	if (color.Contains("G")) m_colors[1] = color["G"].AsFloat();
	if (color.Contains("B")) m_colors[2] = color["B"].AsFloat();
	if (color.Contains("A")) m_colors[3] = color["A"].AsFloat();
}

void SceneChapter4::SaveData() const
{
	JsonLoader saver{};

	saver["Color"]["R"] = m_colors[0];
	saver["Color"]["G"] = m_colors[1];
	saver["Color"]["B"] = m_colors[2];
	saver["Color"]["A"] = m_colors[3];

	saver.Save("save/chapter_4.json");
}
