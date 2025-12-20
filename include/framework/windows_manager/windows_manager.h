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

#ifndef DIRECTX12_WINDOWS_MANAGER_H
#define DIRECTX12_WINDOWS_MANAGER_H


#include "inputs/keyboard.h"
#include "inputs/mouse.h"

#include "framework/types.h"

namespace framework
{
	class DxWindowsManager
	{
	public:
		//~ ctor and dtor
		 explicit DxWindowsManager(_In_ const DX12_WINDOWS_MANAGER_CREATE_DESC& desc);
		 ~DxWindowsManager() noexcept;
		//~ copy and move
		DxWindowsManager(_In_ const DxWindowsManager&) = delete;
		DxWindowsManager(_Inout_ DxWindowsManager&&)   = delete;

		DxWindowsManager& operator=(_In_ const DxWindowsManager&) = delete;
		DxWindowsManager& operator=(_Inout_ DxWindowsManager&&)	  = delete;

		[[nodiscard]] _Check_return_ _Success_(return != EProcessedMessageState::Unknown)
		static EProcessedMessageState ProcessMessages() noexcept;

		[[nodiscard]] _Check_return_ _Success_(return != false)
		bool Initialize();

		[[nodiscard]] _Check_return_ _Success_(return != false)
		bool Release   () noexcept;

		void OnFrameBegin(_In_ float deltaTime) noexcept;
		void OnFrameEnd() noexcept;

		DxMouseInputs	 Mouse{};
		DxKeyboardInputs Keyboard{};

		//~ Getters
		[[nodiscard]] _Ret_maybenull_ _Success_(return != nullptr)
		HWND GetWindowsHandle		() const noexcept;

		[[nodiscard]] _Ret_maybenull_ _Success_(return != nullptr)
		HINSTANCE GetWindowsInstance() const noexcept;

		[[nodiscard]] _Check_return_
		float GetAspectRatio		() const noexcept;

		[[nodiscard]] _Check_return_ __forceinline
		EScreenState GetScreenState	() const noexcept { return m_config.ScreenState;   }

		[[nodiscard]] _Check_return_ __forceinline
		std::uint32_t GetWindowsWidth			() const noexcept { return m_config.Width;  }

		[[nodiscard]] _Check_return_ __forceinline
		std::uint32_t GetWindowsHeight		() const noexcept { return m_config.Height; }

		//~ Setters
		void SetScreenState			(_In_ EScreenState state)			 noexcept;
		void SetWindowTitle			(_In_ const std::wstring& title)		 noexcept;
		void SetWindowMessageOnTitle(_In_ const std::wstring& message) const noexcept;

	private:
		[[nodiscard]] _Check_return_ _Must_inspect_result_ _Success_(return != 0)
		bool InitWindowScreen();

		void TransitionToFullScreen	   ()		noexcept;
		void TransitionToWindowedScreen() const noexcept;

		//~ message proc
		[[nodiscard]]
		LRESULT MessageHandler(
			_In_ HWND   hwnd,
			_In_ UINT   msg,
			_In_ WPARAM wParam,
			_In_ LPARAM lParam) noexcept;

		_Function_class_(WINDOWS_CALLBACK)
		static LRESULT CALLBACK WindowProcThunk(
			_In_ HWND   hwnd,
			_In_ UINT   msg,
			_In_ WPARAM wParam,
			_In_ LPARAM lParam) noexcept;

		_Function_class_(WINDOWS_CALLBACK)
		static LRESULT CALLBACK WindowProcSetup(
				_In_ HWND   hwnd,
				_In_ UINT   message,
				_In_ WPARAM wParam,
				_In_ LPARAM lParam);

	private:
		struct
		{
			std::wstring Title		{ L"DirectX 12 Application" };
			std::wstring ClassName	{ L"DXFramework" };
			UINT		 Width		{ 0u };
			UINT		 Height		{ 0u };
			UINT	     IconID		{ 0u };
			EScreenState ScreenState{ EScreenState::Windowed };
		} m_config;

		HWND			m_pWindowsHandle  { nullptr };
		HINSTANCE		m_pWindowsInstance{ nullptr };
		WINDOWPLACEMENT m_WindowPlacement { sizeof(m_WindowPlacement) };
	};

} // namespace framework


#endif //DIRECTX12_WINDOWS_MANAGER_H
