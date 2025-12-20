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

#ifndef DIRECTX12_REGISTRY_SCENE_H
#define DIRECTX12_REGISTRY_SCENE_H

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "scene/interface_scene.h"
#include "framework/render_manager/render_manager.h"

class RegistryScene
{
public:

	using CreateFunc = std::function<std::unique_ptr<IScene>(framework::DxRenderManager& renderer)>;

	static void Register(const std::string& name, CreateFunc createFunc)
	{
		if (registry_.contains(name)) return;

		registry_[name] = std::move(createFunc);
		mNames.push_back(name);
	}
	static std::unique_ptr<IScene> CreateScene(const std::string& name, framework::DxRenderManager& renderer)
	{
		const auto it = registry_.find(name);
		return it != registry_.end() ? it->second(renderer) : nullptr;
	}
	static const std::vector<std::string>& GetRegisteredNames()
	{
		return mNames;
	}

private:
	inline static std::unordered_map<std::string, CreateFunc> registry_;
	inline static std::vector<std::string> mNames;
};

#define REGISTER_SCENE(CLASS_NAME) \
namespace { \
struct CLASS_NAME##Registrar { \
CLASS_NAME##Registrar() { \
RegistryScene::Register(#CLASS_NAME, \
[](framework::DxRenderManager& renderer) -> std::unique_ptr<IScene> { \
(void)renderer; \
return std::make_unique<CLASS_NAME>(renderer); \
}); \
} \
}; \
static CLASS_NAME##Registrar CLASS_NAME##_registrar; \
}


#endif //DIRECTX12_REGISTRY_SCENE_H
