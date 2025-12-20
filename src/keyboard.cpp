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
#include "framework/windows_manager/inputs/keyboard.h"

#include <cstring>

namespace framework
{
    DxKeyboardInputs::DxKeyboardInputs() noexcept
    {
        ClearAll();
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        switch (message)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            const int key = static_cast<int>(wParam);
            if (!IsInside(key)) return false;

            if (!m_keyDown[ key ]) // key was pressed before
            {
                if (!IsSetAutoRepeat(lParam))
                    m_keyPressed[ key ] = true;
                m_keyDown[ key ] = true;
            }
            return true;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            const int key = static_cast<int>(wParam);
            if (!IsInside(key)) return false;

            if (m_keyDown[ key ])
            {
                m_keyPressed[ key ] = true;
                m_keyDown   [ key ] = false;
            }
            return true;
        }
        case WM_KILLFOCUS:
        case WM_SETFOCUS:
        {
            ClearAll();
            return false;
        }
        default:
            return false;
        }

        return false;
    }

    _Use_decl_annotations_
    void DxKeyboardInputs::OnFrameBegin(float deltaTime) noexcept
    {
    }

    void DxKeyboardInputs::OnFrameEnd() noexcept
    {
        std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
        std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::IsKeyPressed(int virtualKey) const noexcept
    {
        return IsInside(virtualKey) ? m_keyDown[ virtualKey ] : false;
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::WasKeyPressed(int virtualKey) const noexcept
    {
        return IsInside(virtualKey) ? m_keyPressed[ virtualKey ] : false;
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::WasKeyReleased(int virtualKey) const noexcept
    {
        return IsInside(virtualKey) ? m_keyReleased[ virtualKey ] : false;
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::WasChordPressed(int key, const DxKeyboardMode& mode) const noexcept
    {
        if (!WasKeyPressed(key)) return false;

        if ((mode & DxKeyboardMode::Ctrl) && !IsCtrlPressed()) return false;
        if ((mode & DxKeyboardMode::Shift) && !IsShiftPressed()) return false;
        if ((mode & DxKeyboardMode::Alt) && !IsAltPressed()) return false;
        if ((mode & DxKeyboardMode::Super) && !IsSuperPressed()) return false;

        return true;
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::WasMultipleKeyPressed(std::initializer_list<int> keys) const noexcept
    {
        bool anyPressed = false;
        for (int key : keys)
        {
            if (!IsKeyPressed(key)) return false;
            anyPressed = anyPressed || WasKeyPressed(key);
        }
        return anyPressed;
    }

    void DxKeyboardInputs::ClearAll() noexcept
    {
        std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
        std::memset(m_keyDown, 0, sizeof(m_keyDown));
        std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::IsSetAutoRepeat(LPARAM lParam) noexcept
    {
        return (lParam & (1 << 30)) != 0;
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::IsCtrlPressed() const noexcept
    {
        return m_keyDown[ VK_CONTROL ] || m_keyDown[ VK_LCONTROL ] || m_keyDown[ VK_RCONTROL ];
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::IsShiftPressed() const noexcept
    {
        return m_keyDown[ VK_SHIFT ] || m_keyDown[ VK_LSHIFT ] || m_keyDown[ VK_RSHIFT ];
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::IsAltPressed() const noexcept
    {
        return m_keyDown[ VK_MENU ] || m_keyDown[ VK_LMENU ] || m_keyDown[ VK_RMENU ];
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::IsSuperPressed() const noexcept
    {
        return m_keyDown[ VK_LWIN ] || m_keyDown[ VK_RWIN ];
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::Initialize() noexcept
    {
        return true;
    }

    _Use_decl_annotations_
    bool DxKeyboardInputs::Release() noexcept
    {
        return true;
    }

} // namespace framework
