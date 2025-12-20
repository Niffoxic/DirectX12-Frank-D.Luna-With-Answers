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

#ifndef DIRECTX12_KEYBOARD_H
#define DIRECTX12_KEYBOARD_H

#include <windows.h>
#include <initializer_list>
#include <cstdint>
#include <sal.h>

namespace framework
{
	static constexpr unsigned int MAX_KEYBOARD_INPUTS = 256u;

	enum DxKeyboardMode : uint8_t
	{
		None  = 0,
		Ctrl  = 1,
		Shift = 1 << 1,
		Alt   = 1 << 2,
		Super = 1 << 3
	};

	class DxKeyboardInputs
	{
	public:
		DxKeyboardInputs () noexcept;
		~DxKeyboardInputs() noexcept = default;

		//~ No Copy or Move
		DxKeyboardInputs(_In_ const DxKeyboardInputs&) = delete;
		DxKeyboardInputs(_Inout_ DxKeyboardInputs&&)   = delete;

		DxKeyboardInputs& operator=(_In_ const DxKeyboardInputs&) = delete;
		DxKeyboardInputs& operator=(_Inout_ DxKeyboardInputs&&)   = delete;

		[[nodiscard]] _Check_return_ bool Initialize() noexcept;
		[[nodiscard]] _Check_return_ bool Release   () noexcept;

		_Check_return_ [[nodiscard]] bool ProcessMessage(
			_In_ UINT   message,
			_In_ WPARAM wParam,
			_In_ LPARAM lParam) noexcept;

		void OnFrameBegin(_In_ float deltaTime) noexcept;
		void OnFrameEnd  ()					    noexcept;

		//~ Queries
		_Check_return_ [[nodiscard]] bool IsKeyPressed  (int virtualKey) const noexcept;
		_Check_return_ [[nodiscard]] bool WasKeyPressed (int virtualKey) const noexcept;
		_Check_return_ [[nodiscard]] bool WasKeyReleased(int virtualKey) const noexcept;

		_Check_return_ bool WasChordPressed(
					int key,
			_In_	const DxKeyboardMode& mode = DxKeyboardMode::None) const noexcept;

		_Check_return_ bool WasMultipleKeyPressed(
			_In_reads_(keys.size()) std::initializer_list<int> keys) const noexcept;

	private:
		//~ Internal Helpers
		void ClearAll() noexcept;
		_Check_return_ [[nodiscard]] bool IsSetAutoRepeat(_In_ LPARAM lParam) noexcept;

		_Check_return_ [[nodiscard]] bool IsCtrlPressed () const noexcept;
		_Check_return_ [[nodiscard]] bool IsShiftPressed() const noexcept;
		_Check_return_ [[nodiscard]] bool IsAltPressed  () const noexcept;
		_Check_return_ [[nodiscard]] bool IsSuperPressed() const noexcept;

		_Check_return_ [[nodiscard]] bool IsInside(unsigned int virtualKey) const noexcept { return virtualKey >= 0u && virtualKey < MAX_KEYBOARD_INPUTS; }

	private:
		bool m_keyDown	  [ MAX_KEYBOARD_INPUTS ]{};
		bool m_keyPressed [ MAX_KEYBOARD_INPUTS ]{};
		bool m_keyReleased[ MAX_KEYBOARD_INPUTS ]{};
	};
} // namespace framework

#endif //DIRECTX12_KEYBOARD_H
