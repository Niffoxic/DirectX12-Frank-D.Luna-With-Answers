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
#include "framework/windows_manager/inputs/mouse.h"

#include <windowsx.h>
#include <vector>

namespace framework
{
    DxMouseInputs::DxMouseInputs()
        : m_nRawDeltaX(0), m_nRawDeltaY(0),
        m_nMouseWheelDelta(0), m_bCursorVisible(true)
    {
        ZeroMemory(m_bButtonDown, sizeof(m_bButtonDown));
        ZeroMemory(m_bButtonPressed, sizeof(m_bButtonPressed));
        m_pointPosition = { 0, 0 };
    }

    _Use_decl_annotations_
    void DxMouseInputs::AttachWindowHandle(HWND hWnd)
    {
        if (hWnd)
        {
            RAWINPUTDEVICE device = { 0x01, 0x02, 0, hWnd };
            RegisterRawInputDevices(&device, 1u, sizeof(device));
            m_pWindowHandle = hWnd;
        }
    }

    _Use_decl_annotations_
    bool DxMouseInputs::Initialize() noexcept
    {
        return true;
    }

    _Use_decl_annotations_
    bool DxMouseInputs::Release() noexcept
    {
        return true;
    }

    _Use_decl_annotations_
    bool DxMouseInputs::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        switch (message)
        {
        case WM_MOUSEMOVE:
        {
            //~ extract mouse position
            m_pointPosition.x = GET_X_LPARAM(lParam);
            m_pointPosition.y = GET_Y_LPARAM(lParam);
            return true;
        }
        case WM_LBUTTONDOWN:
        {
            //~ left mouse button clicked
            if (!m_bButtonDown[ 0 ])
            {
                m_bButtonPressed[ 0 ] = true;
                m_bButtonDown[ 0 ] = true;
            }
            return true;
        }
        case WM_LBUTTONUP:
        {
            //~ left mouse button click up
            m_bButtonDown[ 0 ] = false;
            return true;
        }
        case WM_RBUTTONDOWN:
        {
            //~ right button clicked
            if (!m_bButtonDown[ 1 ])
            {
                m_bButtonPressed[ 1 ] = true;
                m_bButtonDown[ 1 ] = true;
            }
            return true;
        }
        case WM_RBUTTONUP:
        {
            //~ right button click up
            m_bButtonDown[ 1 ] = false;
            return true;
        }
        case WM_MBUTTONDOWN:
        {
            //~ wheeler button down
            if (!m_bButtonDown[ 2 ])
            {
                m_bButtonPressed[ 2 ] = true;
                m_bButtonDown[ 2 ] = true;
            }
            return true;
        }
        case WM_MBUTTONUP:
        {
            //~ wheeler button click up
            m_bButtonDown[ 2 ] = false;
            return true;
        }
        case WM_MOUSEWHEEL:
        {
            //~ Wheeler in action
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            m_nMouseWheelDelta += delta;
            return true;
        }
        case WM_INPUT:
        {
            //~ thank you microsoft documentation
            UINT size = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
            if (size > 0)
            {
                std::vector<BYTE> data(size);
                if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data.data(), &size, sizeof(RAWINPUTHEADER)) == size)
                {
                    if (const auto* raw = reinterpret_cast<RAWINPUT*>(data.data()); raw->header.dwType == RIM_TYPEMOUSE)
                    {
                        m_nRawDeltaX += raw->data.mouse.lLastX;
                        m_nRawDeltaY += raw->data.mouse.lLastY;
                    }
                }
            }
            return true;
        }
            default: break;
        }
        return false;
    }

    _Use_decl_annotations_
    void DxMouseInputs::OnFrameBegin(float deltaTime) noexcept
    {
    }

    void DxMouseInputs::OnFrameEnd() noexcept
    {
        m_nRawDeltaX = 0;
        m_nRawDeltaY = 0;
        m_nMouseWheelDelta = 0;
        ZeroMemory(m_bButtonPressed, sizeof(m_bButtonPressed));
    }

    void DxMouseInputs::HideCursor()
    {
        if (m_bCursorVisible)
        {
            ShowCursor(FALSE);
            m_bCursorVisible = false;
        }
    }

    void DxMouseInputs::UnHideCursor()
    {
        if (!m_bCursorVisible)
        {
            ShowCursor(TRUE);
            m_bCursorVisible = true;
        }
    }

    void DxMouseInputs::LockCursorToWindow() const
    {
        RECT rect;
        if (GetClientRect(m_pWindowHandle, &rect))
        {
            POINT leftTop{ rect.left,  rect.top };
            POINT rightBottom{ rect.right, rect.bottom };
            ClientToScreen(m_pWindowHandle, &leftTop);
            ClientToScreen(m_pWindowHandle, &rightBottom);

            rect = { leftTop.x, leftTop.y, rightBottom.x, rightBottom.y };
            ClipCursor(&rect);

            //~ force it to the center only
            int centerX = (leftTop.x + rightBottom.x) / 2;
            int centerY = (leftTop.y + rightBottom.y) / 2;
            SetCursorPos(centerX, centerY);
        }
    }

    void DxMouseInputs::UnlockCursor() const
    {
        ClipCursor(nullptr);
    }

} // namespace framework