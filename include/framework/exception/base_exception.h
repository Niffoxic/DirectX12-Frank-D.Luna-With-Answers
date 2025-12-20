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

#ifndef DIRECTX12_BASE_EXCEPTION_H
#define DIRECTX12_BASE_EXCEPTION_H

#include <sal.h>
#include <stdexcept>
#include <string>

namespace framework
{
	class BaseException : public std::exception
	{
	public:
		BaseException(
			_In_z_     const char* file,
			_In_	   const int   line,
			_In_z_     const char* function,
			_In_opt_z_ const char* message
		) : m_szFilePath	(file),
			m_nLineNumber   (line),
			m_szFunctionName(function)
		{
			if (message) m_szErrorMessage = message;
			else m_szErrorMessage = "No error message provided";
		}

		_Ret_z_ _Ret_valid_ _Check_return_
		const char* what() const noexcept override
		{
			if (m_szWhatBuffer.empty())
			{
				m_szWhatBuffer =
					"[BaseException] " + m_szErrorMessage +
					"\nOn File Path: " + m_szFilePath +
					"\nAt Line Number: " + std::to_string(m_nLineNumber) +
					"\nFunction: " + m_szFunctionName;

			}
			return m_szWhatBuffer.c_str();
		}

	protected:
		std::string			m_szFilePath;
		std::string			m_szFunctionName;
		std::string			m_szErrorMessage;
		mutable std::string m_szWhatBuffer;
		int					m_nLineNumber;
	};
}

#define THROW() \
throw framework::BaseException(__FILE__, __LINE__, __FUNCTION__)

#define THROW_MSG(msg) \
throw framework::BaseException(__FILE__, __LINE__, __FUNCTION__, msg)

#endif //DIRECTX12_BASE_EXCEPTION_H
