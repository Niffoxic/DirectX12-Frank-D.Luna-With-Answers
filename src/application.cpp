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
#include "application/application.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include "application/define_scenes.h"
#include "framework/exception/dx_exception.h"

framework::Application::Application(const DX_FRAMEWORK_CONSTRUCT_DESC &desc) : IFramework(desc)
{
	IMGUI_CHECKVERSION	  ();
	ImGui::CreateContext  ();
	ImGui::StyleColorsDark();
}

framework::Application::~Application()
{
}

bool framework::Application::InitApplication()
{
	//~ Init Imgui
	CreateImguiSRVHeap();
	const auto cpuHandle = m_imguiHeap->GetCPUDescriptorHandleForHeapStart();
	const auto gpuHandle = m_imguiHeap->GetGPUDescriptorHandleForHeapStart();

	if (!ImGui_ImplDX12_Init(
	   m_renderManager.Device.Get(),
	   m_renderManager.BackBufferCount,
	   m_renderManager.BackBufferFormat,
	   m_imguiHeap.Get(),
	   cpuHandle,
	   gpuHandle))
	{
		return false;
	}

	const ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels = nullptr;
	int texWidth = 0, texHeight = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);

	//~ Init Scenes
	for (auto& name: RegistryScene::GetRegisteredNames())
	{
		m_scenes.emplace_back(std::move(RegistryScene::CreateScene(name, m_renderManager)));
	}

	if (m_scenes.empty())
	{
		m_activeSceneIndex = 0u;
		m_pendingSceneIndex = static_cast<size_t>(-1);
		return true;
	}

	m_activeSceneIndex = 0u;
	m_pendingSceneIndex = static_cast<size_t>(-1);

	return true;
}

void framework::Application::BeginPlay()
{
}

void framework::Application::Release()
{
}

void framework::Application::Tick(float deltaTime)
{
	if (m_latchedSceneIndex.has_value() && IsSceneIndexValid(*m_latchedSceneIndex))
	{
		m_activeSceneIndex = *m_latchedSceneIndex;
		m_latchedSceneIndex.reset();
	}

	if (m_scenes.empty())
	{
		ImguiView(deltaTime);
		return;
	}

	if (!IsSceneIndexValid(m_activeSceneIndex))
		m_activeSceneIndex = 0u;

	auto& scene = m_scenes[m_activeSceneIndex];

	scene->FrameBegin(deltaTime);

	ImguiView(deltaTime);

	scene->FrameEnd(deltaTime);

	if (m_requestedSceneIndex.has_value() && IsSceneIndexValid(*m_requestedSceneIndex))
	{
		if (*m_requestedSceneIndex != m_activeSceneIndex)
			m_latchedSceneIndex = *m_requestedSceneIndex;

		m_requestedSceneIndex.reset();
	}
}

void framework::Application::CreateImguiSRVHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NumDescriptors = 64u;
	desc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NodeMask		= 0u;
	THROW_DX_IF_FAILS(m_renderManager.Device->CreateDescriptorHeap(&desc,
		IID_PPV_ARGS(&m_imguiHeap)));
}

void framework::Application::ImguiView(float deltaTime)
{
	(void)deltaTime;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	DrawMainMenuBar();
	ImGui::Begin("App");

	if (!IsSceneIndexValid(m_activeSceneIndex))
		m_activeSceneIndex = 0u;

	if (m_requestedSceneIndex.has_value())
		ImGui::Text("Pending Scene Index: %zu (will apply after FrameEnd)", *m_requestedSceneIndex);
	else
		ImGui::Text("Pending Scene Index: none");

	ImGui::Separator();

	if (!m_scenes.empty() && IsSceneIndexValid(m_activeSceneIndex))
	{
		const auto& scene = m_scenes[m_activeSceneIndex];
		scene->ImguiView(deltaTime);
	}

	ImGui::End();

	ImGui::Render();

	ID3D12DescriptorHeap* heaps[] = { m_imguiHeap.Get() };
	m_renderManager.GfxCmd->SetDescriptorHeaps(1, heaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_renderManager.GfxCmd.Get());
}


void framework::Application::DrawMainMenuBar()
{
	if (!ImGui::BeginMainMenuBar())
		return;

	DrawSceneMenu();

	{
		ImGui::Separator();
		ImGui::Text("Active: %zu", m_activeSceneIndex);
	}

	ImGui::EndMainMenuBar();
}

void framework::Application::DrawSceneMenu()
{
	if (ImGui::BeginMenu("Chapters"))
	{
		const auto& names = RegistryScene::GetRegisteredNames();

		for (size_t i = 0; i < m_scenes.size(); ++i)
		{
			const bool isActive  = (i == m_activeSceneIndex);
			const bool isPending = m_requestedSceneIndex.has_value() && (i == *m_requestedSceneIndex);

			std::string label = (i < names.size()) ? names[i] : std::to_string(i);
			if (isPending) label += "  (pending)";

			if (ImGui::MenuItem(label.c_str(), nullptr, isActive))
			{
				if (i != m_activeSceneIndex)
					m_requestedSceneIndex = i;
			}
		}

		if (m_requestedSceneIndex.has_value())
		{
			ImGui::Separator();
			if (ImGui::MenuItem("Cancel pending switch"))
				m_requestedSceneIndex.reset();
		}

		ImGui::EndMenu();
	}
}

void framework::Application::ApplyPendingSceneSwitch()
{
	if (m_pendingSceneIndex == static_cast<size_t>(-1))
		return;

	if (!IsSceneIndexValid(m_pendingSceneIndex))
	{
		m_pendingSceneIndex = static_cast<size_t>(-1);
		return;
	}

	m_activeSceneIndex = m_pendingSceneIndex;
	m_pendingSceneIndex = static_cast<size_t>(-1);
}

bool framework::Application::IsSceneIndexValid(size_t index) const noexcept
{
	return index >= 0u && index < m_scenes.size();
}
