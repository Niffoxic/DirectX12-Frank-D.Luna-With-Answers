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

#ifndef DIRECTX12_WIN_EXCEPTION_H
#define DIRECTX12_WIN_EXCEPTION_H

#include "base_exception.h"
#include <windows.h>

namespace framework
{
	class WinException final : public BaseException
	{
	public:
		WinException(
			_In_z_ const char*  file,
			_In_   const int	line,
			_In_z_ const char*  function,
			_In_   const DWORD	hr = ::GetLastError()
		)
			: BaseException(file, line, function, "None"),
			m_nLastError(hr)
		{
			LPVOID buffer = nullptr;
			DWORD size = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				m_nLastError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPWSTR>(&buffer),
				0,
				nullptr
			);

			if (size && buffer)
			{
				m_szErrorMessage = static_cast<char*>(buffer);
				LocalFree(buffer);
			} else m_szErrorMessage = "UnRecognized Error Spotted (you're screw my boy)";

		}

		_Ret_z_ _Ret_valid_ _Check_return_
		const char* what() const noexcept override
		{
			if (m_szWhatBuffer.empty())
			{
				m_szWhatBuffer =
					"[WinException] "	 + m_szErrorMessage				 +
					"\nOn File Path: "	 + m_szFilePath					 +
					"\nAt Line Number: " + std::to_string(m_nLineNumber) +
					"\nFunction: "		 + m_szFunctionName;
			}
			return m_szWhatBuffer.c_str();
		}

	private:
		DWORD m_nLastError{};
	};
} // namespace framework

#define THROW_WIN() \
throw framework::WinException(__FILE__, __LINE__, __FUNCTION__)

#define THROW_WIN_IF_FAILS(_hr_expr) \
do { HRESULT _hr_internal = (_hr_expr); if (FAILED(_hr_internal)) { \
throw framework::WinException(__FILE__, __LINE__, __FUNCTION__, static_cast<DWORD>(_hr_internal)); \
} } while(0)

#endif //DIRECTX12_WIN_EXCEPTION_H