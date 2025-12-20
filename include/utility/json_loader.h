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

#ifndef DIRECTX12_JSON_LOADER_H
#define DIRECTX12_JSON_LOADER_H

#include <string>
#include <unordered_map>
#include <sstream>
#include <cstdint>

#include "file_system.h"

class JsonLoader
{
public:
    using ObjectType = std::unordered_map<std::string, JsonLoader>;

public:
     JsonLoader() = default;
    ~JsonLoader() = default;

    void Load(const std::string& filePath);
    void Save(const std::string& filePath);

    JsonLoader& operator=(const std::string& value);

    JsonLoader& operator=(const char* value)
    {
        m_value = value ? value : "";
        m_children.clear();
        return *this;
    }

    JsonLoader& operator=(int value)
    {
        m_value = std::to_string(value);
        m_children.clear();
        return *this;
    }

    JsonLoader& operator=(std::int64_t value)
    {
        m_value = std::to_string(value);
        m_children.clear();
        return *this;
    }

    JsonLoader& operator=(float value)
    {
        m_value = std::to_string(value);
        m_children.clear();
        return *this;
    }

    JsonLoader& operator=(double value)
    {
        m_value = std::to_string(value);
        m_children.clear();
        return *this;
    }

    JsonLoader& operator=(bool value)
    {
        m_value = value ? "true" : "false";
        m_children.clear();
        return *this;
    }

    //~ Child access

    const JsonLoader& operator[](const std::string& key) const;
    JsonLoader& operator[](const std::string& key)
    {
        return GetOrCreate(key);
    }

    //~ Rule of 5
    JsonLoader           (JsonLoader&&) noexcept = default;
    JsonLoader& operator=(JsonLoader&&) noexcept = default;

    JsonLoader           (const JsonLoader&) = default;
    JsonLoader& operator=(const JsonLoader&) = default;

    //~ Child management
    JsonLoader& GetOrCreate(const std::string& key);

    auto begin() { return m_children.begin(); }
    auto end() { return m_children.end(); }
    auto begin()  const { return m_children.begin(); }
    auto end()    const { return m_children.end(); }

    const std::string& GetValue() const { return m_value; }
    void               SetValue(const std::string& val) { m_value = val; }

    [[nodiscard]] bool Contains(const std::string& key) const;
    [[nodiscard]] bool Has(const std::string& key) const;

    //~ Serialization helpers
    std::string ToFormattedString(int indent = 0) const;
    void        FromStream(std::istream& input);

    //~ Typed access with optional defaults
    [[nodiscard]] float AsFloat(float defaultValue = 0.0f)   const;
    [[nodiscard]] int   AsInt(int   defaultValue = 0)      const;
    [[nodiscard]] std::uint32_t   AsUInt(std::uint32_t defaultValue = 0)      const;
    [[nodiscard]] bool  AsBool(bool  defaultValue = false)  const;

    [[nodiscard]] bool IsValid() const;
    void               Clear();

    // Object vs leaf convenience
    [[nodiscard]] bool IsObject() const { return !m_children.empty(); }
    [[nodiscard]] bool IsLeaf()   const { return m_children.empty(); }

private:
    void        Serialize(std::ostream& output, int indent) const;
    void        ParseObject(std::istream& input);
    static void SkipWhitespace(std::istream& input);
    static bool ConsumeChar(std::istream& input, char expected);
    static std::string ReadQuotedString(std::istream& input);
    static std::string EscapeString(const std::string& s);

private:
    std::string m_value;
    ObjectType  m_children;

    FileSystem m_fileSystem{};
};
#endif //DIRECTX12_JSON_LOADER_H