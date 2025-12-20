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
#include "framework/interface_framework.h"


#include "framework/exception/base_exception.h"
#include "framework/event/event_queue.h"
#include "framework/event/events_window.h"

#include "utility/logger.h"

namespace framework
{
	_Use_decl_annotations_
	IFramework::IFramework(const DX_FRAMEWORK_CONSTRUCT_DESC& desc)
	{
		if (!CreateManagers(desc))
		{
			logger::error("Failure in building manager!");
			return;
		}
		CreateUtilities();
		SubscribeToEvents();
	}

	IFramework::~IFramework()
	{
		if (m_pWindowsManager && !m_pWindowsManager->Release())
		{
			// TODO: Create Log record
		}
		logger::close();
	}

	_Use_decl_annotations_
	bool IFramework::Init()
	{
		InitManagers();

		if (!InitApplication())
		{
			logger::error("Failed to initialize application!");
			THROW_MSG("Failed to initialize application!");
			return false;
		}

		return true;
	}

	_Use_decl_annotations_
	HRESULT IFramework::Execute()
	{
		m_timer.ResetTime();
		logger::info("Starting Game Loop!");
		BeginPlay();
		while (true)
		{
			float dt = m_timer.Tick();
			if (m_bEnginePaused) dt = 0.0f;

			if (DxWindowsManager::ProcessMessages() == EProcessedMessageState::ExitMessage)
			{
				ReleaseManagers();
				return S_OK;
			}
			ManagerFrameBegin(dt);
			Tick(dt);
			ManagerFrameEnd();

#if defined(DEBUG) || defined(_DEBUG)
			static float passed = 0.0f;
			static int   frame = 0;
			static float avg_frames = 0.0f;
			static float last_time_elapsed = 0.0f;

			frame++;
			passed += dt;

			if (passed >= 1.0f)
			{
				avg_frames += frame;
				last_time_elapsed = m_timer.TimeElapsed();

				std::wstring message =
					L"Time Elapsed: " +
					std::to_wstring(last_time_elapsed) +
					L" Frame Rate: " +
					std::to_wstring(frame) +
					L" per second (Avg = " +
					std::to_wstring(avg_frames / last_time_elapsed) +
					L")";

				m_pWindowsManager->SetWindowMessageOnTitle(message);

				passed = 0.0f;
				frame = 0;
			}
#endif
			EventQueue::DispatchAll();
		}
		return S_OK;
	}

	_Use_decl_annotations_
	bool IFramework::CreateManagers(const DX_FRAMEWORK_CONSTRUCT_DESC& desc)
	{
		m_pWindowsManager = std::make_unique<DxWindowsManager>(desc.WindowsDesc);
		m_renderManager.AttachWindows(m_pWindowsManager.get());
		return true;
	}

	void IFramework::CreateUtilities()
	{
#if defined(_DEBUG) || defined(DEBUG)
		LOGGER_CREATE_DESC cfg{};
		cfg.TerminalName = "DirectX 12 Logger";
		logger::init(cfg);
#endif
	}

	void IFramework::InitManagers()
	{
		if (m_pWindowsManager && !m_pWindowsManager->Initialize())
		{
			logger::error("Failed to initialize Windows Manager!");
		}

		if (!m_renderManager.Initialize())
		{
			logger::error("Failed to Initialize Render Manager");
		}

		logger::success("All Managers initialized.");
	}

	void framework::IFramework::ReleaseManagers()
	{
		logger::warning("Closing Application!");

		if (m_pWindowsManager && !m_pWindowsManager->Release())
		{
			logger::error("Failed to Release Windows Manager!");
		}

		if (!m_renderManager.Release())
		{
			logger::error("Failed to release render manager");
		}

		logger::close();
	}

	_Use_decl_annotations_
	void IFramework::ManagerFrameBegin(float deltaTime)
	{
		if (m_pWindowsManager)
		{
			m_pWindowsManager->OnFrameBegin(deltaTime);
		}
	}

	void IFramework::ManagerFrameEnd()
	{
		if (m_pWindowsManager)
		{
			m_pWindowsManager->OnFrameEnd();
		}
	}

	void IFramework::SubscribeToEvents()
	{
		auto token = EventQueue::Subscribe<WINDOW_PAUSE_EVENT>(
			[&](const WINDOW_PAUSE_EVENT& event)
		{
			if (event.Paused) m_bEnginePaused = true;
			else
			{
				m_bEnginePaused = false;
				m_timer.ResetTime();
			}

			logger::debug("Window Drag Event Recevied with {}", event.Paused);
		});
	}

} // namespace framework
