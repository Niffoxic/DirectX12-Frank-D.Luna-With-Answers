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

#ifndef DIRECTX12_APPLICATION_H
#define DIRECTX12_APPLICATION_H

#include <sal.h>
#include <vector>
#include <memory>

#include "framework/interface_framework.h"
#include "scene/interface_scene.h"
#include "framework/types.h"

namespace framework
{
	class Application final : public IFramework
	{
	public:
		explicit Application(_In_opt_ const DX_FRAMEWORK_CONSTRUCT_DESC& desc);
		~Application() override;

	protected:
		//~ Framework interface Impl
		_Check_return_
		bool InitApplication() override;
		void BeginPlay		() override;
		void Release		() override;

		void Tick(_In_ float deltaTime) override;

	private:
		void CreateImguiSRVHeap();
		void ImguiView(float deltaTime);

		//~ UI helpers
		void DrawMainMenuBar();
		void DrawSceneMenu();

		//~ Scene switching (deferred)
		void ApplyPendingSceneSwitch();
		_Check_return_ bool IsSceneIndexValid(size_t index) const noexcept;

	private:
		std::vector<std::unique_ptr<IScene>> m_scenes;
		std::optional<size_t> m_requestedSceneIndex;
		std::optional<size_t> m_latchedSceneIndex;
		size_t m_activeSceneIndex { 0u };
		size_t m_pendingSceneIndex{ static_cast<size_t>(-1) };
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_imguiHeap;
	};
}

#endif //DIRECTX12_APPLICATION_H
