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

#ifndef DIRECTX12_EVENTS_WINDOW_H
#define DIRECTX12_EVENTS_WINDOW_H
#include <cstddef>

typedef struct FULL_SCREEN_EVENT
{
	std::size_t Width;
	std::size_t Height;
} FULL_SCREEN_EVENT;

typedef struct WINDOWED_SCREEN_EVENT
{
	std::size_t Width;
	std::size_t Height;
} WINDOWED_SCREEN_EVENT;

typedef struct WINDOW_RESIZE_EVENT
{
	std::size_t Width;
	std::size_t Height;
} WINDOW_RESIZE_EVENT;


typedef struct WINDOW_PAUSE_EVENT
{
	bool Paused;
} WINDOW_PAUSE_EVENT;

#endif //DIRECTX12_EVENTS_WINDOW_H
