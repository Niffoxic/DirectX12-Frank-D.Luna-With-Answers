//
// Created by Niffoxic (Aka Harsh Dubey) on 1/5/2026.
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
// Copyright (c) 2026 Niffoxic
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
#include "application/scene/common_scene_data.h"
#include <imgui.h>

void RiverUpdateParam::ImguiView()
{
	if (!ImGui::CollapsingHeader("River Parameters", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	// Wave Shape
	if (ImGui::CollapsingHeader("Waves", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SliderFloat("Amp 1", &amp1, 0.0f, 0.50f);
		ImGui::SliderFloat("Amp 2", &amp2, 0.0f, 0.50f);

		ImGui::SliderFloat("Freq 1", &freq1, 0.0f, 10.0f);
		ImGui::SliderFloat("Freq 2", &freq2, 0.0f, 10.0f);

		ImGui::SliderFloat("WaveLen 1", &waveLen1, 0.0f, 5.0f);
		ImGui::SliderFloat("WaveLen 2", &waveLen2, 0.0f, 5.0f);

		ImGui::SliderFloat("Flow Speed", &flowSpeed, 0.0f, 10.0f);
	}

	// Small Waves / Ripples
	if (ImGui::CollapsingHeader("Ripples", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SliderInt  ("Octaves", &octaves, 1, 8);
		ImGui::SliderFloat("Base Amp", &octaveBaseAmp, 0.0f, 0.10f);
		ImGui::SliderFloat("Base Freq", &octaveBaseFreq, 0.0f, 5.0f);
		ImGui::SliderFloat("Base WaveLen", &octaveBaseWaveLen, 0.0f, 5.0f);

		ImGui::SliderFloat("Edge Noise", &edgeNoiseStrength, 0.0f, 1.5f);
	}

	if (ImGui::CollapsingHeader("Height", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SliderFloat("Height Scale", &heightScale, 0.0f, 10.0f);
		ImGui::SliderFloat("Height Bias", &heightBias, -1.0f, 1.0f);
		ImGui::SliderFloat("Max Height", &maxHeight, 0.0f, 2.0f);
		ImGui::SliderFloat("Foam Threshold", &foamHeightThreshold, -1.0f, 1.0f);
	}

	// River Shape
	if (ImGui::CollapsingHeader("River Shape", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SliderFloat("Half Width", &halfWidth, 0.0f, 50.0f);
		ImGui::DragFloatRange2("Z Range", &minZ, &maxZ, 0.05f);
	}

	// Color Gradient
	if (ImGui::CollapsingHeader("Color Gradient", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit3("Left",      &leftColor.x);
		ImGui::ColorEdit3("Right",     &rightColor.x);
		ImGui::ColorEdit3("Down Left", &downLeftColor.x);
		ImGui::ColorEdit3("Down Right",&downRightColor.x);
	}

	// Water Tinting
	if (ImGui::CollapsingHeader("Water Tint", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit3("Shallow", &shallowColor.x);
		ImGui::ColorEdit3("Deep",    &deepColor.x);
		ImGui::ColorEdit3("Foam",    &foamColor.x);

		ImGui::SliderFloat("Foam Strength", &foamStrength, 0.0f, 4.0f);
		ImGui::SliderFloat("Shimmer Strength", &shimmerStrength, 0.0f, 1.0f);
	}
}