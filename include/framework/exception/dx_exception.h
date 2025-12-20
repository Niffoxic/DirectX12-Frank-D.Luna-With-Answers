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

#ifndef DIRECTX12_DX_EXCEPTION_H
#define DIRECTX12_DX_EXCEPTION_H

#include "base_exception.h"

#include <windows.h>
#include <comdef.h>
#include <string>
#include <sstream>
#include <iomanip>

namespace framework
{
    class DxException final : public BaseException
    {
    public:
        DxException(
            _In_z_ const char*   file,
            _In_   const int     line,
            _In_z_ const char*   function,
            _In_   const HRESULT hr
        ) noexcept
            : BaseException(file, line, function, "DirectX call failed")
            , m_nErrorCode(hr)
        {
            BuildErrorMessage();
        }

        _Ret_z_ _Ret_valid_ _Check_return_
        const char* what() const noexcept override
        {
            if (m_szWhatBuffer.empty())
            {
                std::ostringstream oss;

                oss << "[DxException] " << m_szErrorMessage
                    << "\n  HRESULT : 0x"
                    << std::hex << std::uppercase
                    << std::setw(8) << std::setfill('0')
                    << static_cast<unsigned long>(m_nErrorCode)
                    << std::dec
                    << "\n  File    : " << m_szFilePath
                    << "\n  Line    : " << m_nLineNumber
                    << "\n  Function: " << m_szFunctionName;

                m_szWhatBuffer = oss.str();
            }
            return m_szWhatBuffer.c_str();
        }

        HRESULT GetErrorCode() const noexcept
        {
            return m_nErrorCode;
        }

    private:
        void BuildErrorMessage() noexcept
        {
            const _com_error err(m_nErrorCode);

            if (const wchar_t* wideMsg = err.ErrorMessage(); wideMsg != nullptr)
            {
                const int requiredSize = ::WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    wideMsg,
                    -1,
                    nullptr,
                    0,
                    nullptr,
                    nullptr
                );

                if (requiredSize > 0)
                {
                    std::string utf8(static_cast<size_t>(requiredSize), '\0');

                    ::WideCharToMultiByte(
                        CP_UTF8,
                        0,
                        wideMsg,
                        -1,
                        utf8.data(),
                        requiredSize,
                        nullptr,
                        nullptr
                    );

                    if (!utf8.empty() && utf8.back() == '\0')
                        utf8.pop_back();

                    m_szErrorMessage = utf8;
                    return;
                }
            }
            m_szErrorMessage = "Unknown DirectX error.";
        }

    private:
        HRESULT m_nErrorCode{};
    };
} // namespace framework

#define THROW_DX_IF_FAILS(_hr_expr)                                  \
    do                                                               \
    {                                                                \
        const HRESULT _hr_internal_ = (_hr_expr);                    \
        if (FAILED(_hr_internal_))                                   \
        {                                                            \
            throw framework::DxException(                            \
                __FILE__,                                            \
                __LINE__,                                            \
                __FUNCTION__,                                        \
                _hr_internal_);                                      \
        }                                                            \
    } while (0)

#endif //DIRECTX12_DX_EXCEPTION_H
