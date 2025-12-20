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
#include "framework/exception/win_exception.h"

#include "framework/interface_framework.h"
#include <cstring>

int WINAPI WinMain(_In_		HINSTANCE hInstance,
				   _In_opt_ HINSTANCE prevInstance,
				   _In_		LPSTR lpCmdLine,
				   _In_		int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(prevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	try
	{
		DX12_WINDOWS_MANAGER_CREATE_DESC WindowsDesc{};
		WindowsDesc.Height      = 720u;
		WindowsDesc.Width       = 1280u;
		WindowsDesc.IconId      = 0u;
		WindowsDesc.WindowTitle = L"DirectX 12 Application";
		WindowsDesc.ScreenState = EScreenState::Windowed;

		DX_FRAMEWORK_CONSTRUCT_DESC engineDesc{};
		engineDesc.WindowsDesc = WindowsDesc;

		framework::Application application{ engineDesc };

		if (!application.Init()) return E_FAIL;

		return application.Execute();
	}
	catch (const framework::BaseException& ex)
	{
		std::wstring msg(ex.what(), ex.what() + std::strlen(ex.what()));
		MessageBox(nullptr, msg.c_str(), L"PixelFox Exception", MB_ICONERROR | MB_OK);
		return EXIT_FAILURE;
	}
	catch (const std::exception& ex)
	{
		std::wstring msg(ex.what(), ex.what() + std::strlen(ex.what()));
		MessageBox(nullptr, msg.c_str(), L"Standard Exception", MB_ICONERROR | MB_OK);
		return EXIT_FAILURE;
	}
	catch (...)
	{
		MessageBox(nullptr, L"Unknown fatal error occurred.", L"PixelFox Crash", MB_ICONERROR | MB_OK);
		return EXIT_FAILURE;
	}
}