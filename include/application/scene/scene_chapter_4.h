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

#ifndef DIRECTX12_SCENE_CHAPTER_4_H
#define DIRECTX12_SCENE_CHAPTER_4_H

#include "interface_scene.h"

class SceneChapter4 final: public IScene
{
public:
	SceneChapter4(framework::DxRenderManager& renderer);
	~SceneChapter4() override;

	bool Initialize() override;
	void Shutdown  () override;

	void FrameBegin	  (float deltaTime) override;
	void FrameEnd	  (float deltaTime) override;
	void ImguiView(float deltaTime) override;

private:
	void LoadData();
	void SaveData() const;

private:
	std::uint32_t m_frameIndex{ 0u };
	std::uint64_t m_fenceValue{ 0u };
	HANDLE m_waitEvent;
	float m_colors[4]{1.f, 1.f, 1.f, 1.f};
};

#endif // DIRECTX12_SCENE_CHAPTER_4_H