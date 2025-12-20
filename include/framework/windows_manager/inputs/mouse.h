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

#ifndef DIRECTX12_MOUSE_H
#define DIRECTX12_MOUSE_H

#include <windows.h>

namespace framework
{
	class DxMouseInputs
	{
	public:
		DxMouseInputs();
		~DxMouseInputs() noexcept = default;

		void AttachWindowHandle(_In_ HWND hWnd);

		[[nodiscard]] _Check_return_ bool Initialize() noexcept;
		[[nodiscard]] _Check_return_ bool Release   () noexcept;

		[[nodiscard]] _Check_return_
		bool ProcessMessage(
			_In_ UINT message,
			_In_ WPARAM wParam,
			_In_ LPARAM lParam) noexcept;

		void OnFrameBegin(_In_ float deltaTime) noexcept;
		void OnFrameEnd  ()				        noexcept;

		//~ Modifiers
		void HideCursor		   ();
		void UnHideCursor	   ();
		void LockCursorToWindow() const;
		void UnlockCursor      () const;

		//~ Queries
		__forceinline
		void GetMousePosition(_In_ int& x, _In_ int& y) const
		{
			x = m_pointPosition.x; y = m_pointPosition.y;
		}

		__forceinline
		void GetMouseDelta(_In_ int& dx, _In_ int& dy) const
		{
			dx = m_nRawDeltaX; dy = m_nRawDeltaY;
		}

		[[nodiscard]] _Check_return_
		bool IsMouseButtonPressed(const int type) const
		{
			return (type >= 0 && type < 3) ? m_bButtonDown[ type ] : false;
		}

		[[nodiscard]] _Check_return_
		bool WasButtonPressed(const int type) const
		{
			return (type >= 0 && type < 3) ? m_bButtonPressed[ type ] : false;
		}

		[[nodiscard]] _Check_return_
		int GetMouseWheelDelta() const
		{
			return m_nMouseWheelDelta;
		}

	private:
		HWND  m_pWindowHandle{ nullptr };
		bool  m_bButtonDown   [ 3 ];
		bool  m_bButtonPressed[ 3 ];
		POINT m_pointPosition;
		int   m_nRawDeltaX;
		int	  m_nRawDeltaY;
		int   m_nMouseWheelDelta;
		bool  m_bCursorVisible;
	};
} // namespace framework

#endif //DIRECTX12_MOUSE_H