//
// Created by Niffoxic (Aka Harsh Dubey) on 12/19/2025.
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
#include "utility/helpers.h"
#include <algorithm>
#include <cwctype>

_Use_decl_annotations_

std::string helpers::ToLowerAscii(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

_Use_decl_annotations_
 std::wstring helpers::ToLowerAscii(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return s;
}

_Use_decl_annotations_
 std::string helpers::WideToAnsi(std::wstring_view wstr)
{
    if (wstr.empty()) return {};

    if (wstr.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return {};

    const int srcLength = static_cast<int>(wstr.size());

    const int requiredSize = ::WideCharToMultiByte(
        CP_ACP,
        0,
        wstr.data(),
        srcLength,
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (requiredSize <= 0)
        return {};

    std::string result(static_cast<size_t>(requiredSize), '\0');

    const int converted = ::WideCharToMultiByte(
        CP_ACP,
        0,
        wstr.data(),
        srcLength,
        result.data(),
        requiredSize,
        nullptr,
        nullptr
    );

    if (converted <= 0)
        return {};

    if (converted != requiredSize)
        result.resize(static_cast<size_t>(converted));

    return result;
}

_Use_decl_annotations_
 std::wstring helpers::AnsiToWide(std::string_view str)
{
    if (str.empty())
        return {};

    if (str.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return {};

    const int srcLength = static_cast<int>(str.size());

    const int requiredSize = ::MultiByteToWideChar(
        CP_ACP,
        0,
        str.data(),
        srcLength,
        nullptr,
        0
    );

    if (requiredSize <= 0)
        return {};

    std::wstring result(static_cast<size_t>(requiredSize), L'\0');

    const int converted = ::MultiByteToWideChar(
        CP_ACP,
        0,
        str.data(),
        srcLength,
        result.data(),
        requiredSize
    );

    if (converted <= 0)
        return {};

    if (converted != requiredSize)
        result.resize(static_cast<size_t>(converted));

    return result;
}

_Use_decl_annotations_
bool helpers::IsPathExists(const std::wstring& path)
{
    DWORD attr = GetFileAttributes(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES);
}

_Use_decl_annotations_
bool helpers::IsPathExists(const std::string& path)
{
    std::wstring w_path = helpers::AnsiToWide(path);
    return helpers::IsPathExists(w_path);
}

_Use_decl_annotations_
bool helpers::IsDirectory(const std::string& path)
{
    std::wstring w_path = helpers::AnsiToWide(path);
    DWORD attr = GetFileAttributes(w_path.c_str());

    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

_Use_decl_annotations_
bool helpers::IsFile(const std::string& path)
{
    const std::wstring w_path = helpers::AnsiToWide(path);
    return IsFile(w_path);
}

_Use_decl_annotations_
bool helpers::IsFile(const std::wstring &path)
{
    const DWORD attr = GetFileAttributes(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

_Use_decl_annotations_
bool helpers::CopyFiles(const std::string& source, const std::string& destination, bool overwrite)
{
    if (!helpers::IsPathExists(source))
    {
        return false;
    }

    std::wstring srcW = helpers::AnsiToWide(source);
    std::wstring dstW = helpers::AnsiToWide(destination);

    return CopyFile(srcW.c_str(), dstW.c_str(), overwrite);
}

_Use_decl_annotations_
bool helpers::MoveFiles(const std::string& source, const std::string& destination)
{
    if (!helpers::IsPathExists(source))
    {
        return false;
    }

    std::wstring srcW = helpers::AnsiToWide(source);
    std::wstring dstW = helpers::AnsiToWide(destination);

    return MoveFileW(srcW.c_str(), dstW.c_str());
}

_Use_decl_annotations_
helpers::DIRECTORY_AND_FILE_NAME helpers::SplitPathFile(const std::string& fullPath)
{
    size_t lastSlash = fullPath.find_last_of("/\\");
    if (lastSlash == std::string::npos)
    {
        return { "", fullPath };
    }

    return
    {
        fullPath.substr(0, lastSlash),
        fullPath.substr(lastSlash + 1)
    };
}