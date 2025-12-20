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

#ifndef DIRECTX12_FILE_SYSTEM_H
#define DIRECTX12_FILE_SYSTEM_H

#include <string>
#include <cstdint>
#include <memory>

class FileSystem
{
public:
	FileSystem();
	~FileSystem();

	FileSystem(const FileSystem&) = delete;
	FileSystem(FileSystem&&);

	FileSystem& operator=(const FileSystem&) = delete;
	FileSystem& operator=(FileSystem&&);

	[[nodiscard]] bool OpenForRead (_In_ const std::string& path);
	[[nodiscard]] bool OpenForWrite(_In_ const std::string& path);

	void Close();
	[[nodiscard]] bool ReadBytes(_Out_writes_bytes_all_(size) void*  dest,
							 _In_                         size_t size) const;
	[[nodiscard]] bool WriteBytes(_In_reads_bytes_(size) const void* data,
							 _In_                    size_t      size) const;

	[[nodiscard]] bool ReadUInt32    (_Out_      std::uint32_t& value) const;
	[[nodiscard]] bool WriteUInt32   (_In_       std::uint32_t value ) const;
	[[nodiscard]] bool ReadString    (_Out_      std::string& outStr ) const;
	[[nodiscard]] bool WriteString   (_In_ const std::string& str    ) const;
	[[nodiscard]] bool WritePlainText(_In_ const std::string& str    ) const;

	[[nodiscard]] bool          IsOpen     () const;
	[[nodiscard]] std::uint64_t GetFileSize() const;

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif //DIRECTX12_FILE_SYSTEM_H