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

#ifndef DIRECTX12_TYPES_H
#define DIRECTX12_TYPES_H

#include <cstdint>
#include <sal.h>
#include <string>

enum class EScreenState: bool
{
	Windowed   = false,
	Fullscreen = true
};

enum class EProcessedMessageState : std::uint8_t
{
	ExitMessage = 0,
	ExecuteMessage,
	Unknown
};

typedef struct DX12_WINDOWS_MANAGER_CREATE_DESC
{
	std::wstring  WindowTitle{ L"DirectX12" };
	std::uint32_t Width      { 1280u };
	std::uint32_t Height     { 720u };
	std::uint32_t IconId     { 0u }; //~ 0 means no icon is attached
	EScreenState ScreenState{ EScreenState::Windowed };
} DX12_WINDOWS_MANAGER_CREATE_DESC;


typedef struct DX_FRAMEWORK_CONSTRUCT_DESC
{
	_In_ DX12_WINDOWS_MANAGER_CREATE_DESC WindowsDesc;
} DX_FRAMEWORK_CONSTRUCT_DESC;

#endif // DIRECTX12_TYPES_H
