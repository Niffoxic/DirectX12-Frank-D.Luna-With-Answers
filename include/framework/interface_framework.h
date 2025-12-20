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

#ifndef DIRECTX12_INTERFACE_FRAMEWORK_H
#define DIRECTX12_INTERFACE_FRAMEWORK_H

#include "windows_manager/windows_manager.h"
#include "render_manager/render_manager.h"
#include "utility/timer.h"
#include "types.h"
#include <memory>

namespace framework
{
	class IFramework
	{
	public:
		IFramework(_In_ const DX_FRAMEWORK_CONSTRUCT_DESC& desc);
		virtual ~IFramework();

		_Check_return_ _Success_(return != false)
		bool Init();

		_Success_(return == S_OK)
		HRESULT Execute();

	protected:
		//~ Application Must Implement them
		_Check_return_
		virtual bool InitApplication() = 0;
		virtual void BeginPlay		() = 0;
		virtual void Release		() = 0;

		virtual void Tick(_In_ float deltaTime) = 0;

	private:
		_Check_return_
		bool CreateManagers(_In_ const DX_FRAMEWORK_CONSTRUCT_DESC& desc);

		void CreateUtilities     ();
		void InitManagers		 ();
		void ReleaseManagers     ();
		void ManagerFrameBegin   (_In_ float deltaTime);
		void ManagerFrameEnd     ();
		void SubscribeToEvents	 ();

	protected:
		GameTimer m_timer{};
		std::unique_ptr<DxWindowsManager> m_pWindowsManager{ nullptr };
		DxRenderManager m_renderManager {};

	private:
		bool m_bEnginePaused{ false };
	};
} // namespace framework

#endif //DIRECTX12_INTERFACE_FRAMEWORK_H
