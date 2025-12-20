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
#include "utility/json_loader.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>

#include "utility/logger.h"

namespace
{
    const JsonLoader& GetInvalidNode()
    {
        static const JsonLoader invalid{};
        return invalid;
    }
}

void JsonLoader::Load(const std::string& filePath)
{
    Clear();

    if (!m_fileSystem.OpenForRead(filePath))
    {
         logger::error("JsonLoader::Load - Failed to open file for read: {}", filePath);
        return;
    }

    const std::uint64_t fileSize = m_fileSystem.GetFileSize();
    if (fileSize == 0ULL)
    {
        logger::warning("JsonLoader::Load - File is empty: {}", filePath);
        m_fileSystem.Close();
        return;
    }

    std::string content(fileSize, '\0');
    if (!m_fileSystem.ReadBytes(content.data(), fileSize))
    {
         logger::error("JsonLoader::Load - Failed to read bytes from file: {}", filePath);
        m_fileSystem.Close();
        return;
    }

    m_fileSystem.Close();

    std::istringstream iss(content);
    FromStream(iss);
}

void JsonLoader::Save(const std::string& filepath)
{
    if (!m_fileSystem.OpenForWrite(filepath))
    {
         logger::error("JsonLoader::Save - Failed to open file for write: {}", filepath);
        return;
    }

    std::ostringstream oss;
    Serialize(oss, 0);

    if (!m_fileSystem.WritePlainText(oss.str()))
    {
         logger::error("JsonLoader::Save - Failed to write to file: {}", filepath);
    }
    else
    {
         logger::error("JsonLoader::Save - Successfully saved JSON to: {}", filepath);
    }

    m_fileSystem.Close();
}

JsonLoader& JsonLoader::operator=(const std::string& value)
{
    m_value = value;
    m_children.clear();
    return *this;
}

const JsonLoader& JsonLoader::operator[](const std::string& key) const
{
    auto it = m_children.find(key);
    if (it != m_children.end())
    {
        return it->second;
    }
    return GetInvalidNode();
}

JsonLoader& JsonLoader::GetOrCreate(const std::string& key)
{
    return m_children[key];
}

bool JsonLoader::Contains(const std::string& key) const
{
    return m_children.find(key) != m_children.end();
}

bool JsonLoader::Has(const std::string& key) const
{
    return Contains(key);
}

std::string JsonLoader::ToFormattedString(int indent) const
{
    std::ostringstream oss;
    Serialize(oss, indent);
    return oss.str();
}

void JsonLoader::FromStream(std::istream& input)
{
    Clear();

    SkipWhitespace(input);

    if (input.peek() == '{')
    {
        ParseObject(input);
    }
    else if (input.peek() == '"')
    {
        m_value = ReadQuotedString(input);
    }
    else
    {
        std::string token;
        while (input.good())
        {
            char c = static_cast<char>(input.peek());
            if (std::isspace(static_cast<unsigned char>(c)) || c == ',' || c == '}' || c == EOF)
                break;

            input.get(c);
            token.push_back(c);
        }
        m_value = token;
    }
}

float JsonLoader::AsFloat(float defaultValue) const
{
    if (!IsValid())
        return defaultValue;

    try
    {
        return std::stof(m_value);
    }
    catch (...)
    {
        return defaultValue;
    }
}

int JsonLoader::AsInt(int defaultValue) const
{
    if (!IsValid())
        return defaultValue;

    try
    {
        return std::stoi(m_value);
    }
    catch (...)
    {
        return defaultValue;
    }
}

std::uint32_t JsonLoader::AsUInt(std::uint32_t defaultValue) const
{
    if (!IsValid())
        return defaultValue;

    try
    {
        return static_cast<std::uint32_t>(std::stoi(m_value));
    }
    catch (...)
    {
        return defaultValue;
    }
}

bool JsonLoader::AsBool(bool defaultValue) const
{
    if (!IsValid())
        return defaultValue;

    std::string val = m_value;
    std::transform(
        val.begin(), val.end(), val.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
    );

    if (val == "true" || val == "1")
        return true;
    if (val == "false" || val == "0")
        return false;

    return defaultValue;
}

bool JsonLoader::IsValid() const
{
    return !m_value.empty() || !m_children.empty();
}

void JsonLoader::Clear()
{
    m_value.clear();
    m_children.clear();
}

void JsonLoader::Serialize(std::ostream& output, int indent) const
{
    const std::string indentStr(static_cast<std::size_t>(indent), '\t');

    if (!m_children.empty())
    {
        output << "{\n";
        bool first = true;

        for (const auto& [key, child] : m_children)
        {
            if (!first)
                output << ",\n";
            first = false;

            output << indentStr << '\t'
                << "\"" << EscapeString(key) << "\": ";
            child.Serialize(output, indent + 1);
        }

        output << '\n' << indentStr << '}';
    }
    else
    {
        output << "\"" << EscapeString(m_value) << "\"";
    }
}

void JsonLoader::SkipWhitespace(std::istream& input)
{
    while (input.good() && std::isspace(static_cast<unsigned char>(input.peek())))
    {
        input.get();
    }
}

bool JsonLoader::ConsumeChar(std::istream& input, char expected)
{
    if (!input.good())
        return false;

    const int c = input.get();
    if (c != expected)
    {
         logger::error(
            "JsonLoader::ConsumeChar - Expected '{}' but got '{}'",
            expected,
            static_cast<char>(c)
        );
        return false;
    }
    return true;
}

std::string JsonLoader::ReadQuotedString(std::istream& input)
{
    SkipWhitespace(input);

    if (!ConsumeChar(input, '"'))
    {
        return {};
    }

    std::string result;
    while (input.good())
    {
        char c = static_cast<char>(input.get());
        if (c == '"')
        {
            break;
        }

        if (c == '\\')
        {
            if (!input.good())
                break;

            char esc = static_cast<char>(input.get());
            switch (esc)
            {
            case '"':  result.push_back('"');  break;
            case '\\': result.push_back('\\'); break;
            case '/':  result.push_back('/');  break;
            case 'b':  result.push_back('\b'); break;
            case 'f':  result.push_back('\f'); break;
            case 'n':  result.push_back('\n'); break;
            case 'r':  result.push_back('\r'); break;
            case 't':  result.push_back('\t'); break;
            default:
                // Unknown escape, keep as-is
                result.push_back(esc);
                break;
            }
        }
        else
        {
            result.push_back(c);
        }
    }

    return result;
}

std::string JsonLoader::EscapeString(const std::string& s)
{
    std::string escaped;
    escaped.reserve(s.size());

    for (char c : s)
    {
        switch (c)
        {
        case '"':  escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b";  break;
        case '\f': escaped += "\\f";  break;
        case '\n': escaped += "\\n";  break;
        case '\r': escaped += "\\r";  break;
        case '\t': escaped += "\\t";  break;
        default:
            escaped.push_back(c);
            break;
        }
    }

    return escaped;
}

void JsonLoader::ParseObject(std::istream& input)
{
    SkipWhitespace(input);

    if (!ConsumeChar(input, '{'))
        return;

    while (true)
    {
        SkipWhitespace(input);

        if (input.peek() == '}')
        {
            input.get();
            break;
        }

        std::string key = ReadQuotedString(input);
        if (key.empty() && !input.good())
        {
             logger::error("JsonLoader::ParseObject - Failed to read key");
            return;
        }

        SkipWhitespace(input);
        if (!ConsumeChar(input, ':'))
        {
             logger::error("JsonLoader::ParseObject - Expected ':' after key '{}'", key);
            return;
        }

        SkipWhitespace(input);

        JsonLoader child;

        if (input.peek() == '{')
        {
            child.ParseObject(input);
        }
        else if (input.peek() == '"')
        {
            child.m_value = ReadQuotedString(input);
        }
        else
        {
            std::string token;
            while (input.good())
            {
                char c = static_cast<char>(input.peek());
                if (std::isspace(static_cast<unsigned char>(c)) ||
                    c == ',' || c == '}' || c == EOF)
                    break;

                input.get(c);
                token.push_back(c);
            }
            child.m_value = token;
        }

        m_children.emplace(std::move(key), std::move(child));

        SkipWhitespace(input);
        const int next = input.peek();
        if (next == ',')
        {
            input.get();
            continue;
        }
        else if (next == '}')
        {
            continue;
        }
        else
        {
            if (input.good())
            {
                 logger::warning(
                    "JsonLoader::ParseObject - Unexpected character '{}' while parsing object",
                    static_cast<char>(next)
                );
            }
            break;
        }
    }
}
