//
// Created by Niffoxic (Aka Harsh Dubey) on 12/29/2025.
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

#include "framework/animation/anim.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "utility/logger.h"
#include "utility/helpers.h"


void foxanim::FxAnim::Test()
{
	constexpr auto modelPath = "assets/model/animations/idle.fbx";
	if (!helpers::IsFile((modelPath)))
	{
		logger::error("Model Path Does not exit!");
		return;
	}

	Assimp::Importer importer;

	constexpr unsigned int flags =
	aiProcess_Triangulate |
	aiProcess_GenNormals |
	aiProcess_CalcTangentSpace |
	aiProcess_ImproveCacheLocality;

	const aiScene* scene = importer.ReadFile(modelPath, flags);

	if (!scene)
	{
		logger::error("Assimp ReadFile failed! path='{}' error='{}'", modelPath, importer.GetErrorString());
		return;
	}

	if (!scene->mRootNode)
	{
		logger::error("Assimp scene has no root node! path='{}'", modelPath);
		return;
	}

	if (scene->mNumMeshes == 0 || !scene->mMeshes)
	{
		logger::error("Assimp scene has no meshes! path='{}'", modelPath);
		return;
	}

	logger::info("Assimp load OK: '{}'", modelPath);
	logger::info("Meshes: {} | Materials: {} | Animations: {} | Textures(embedded): {}",
		scene->mNumMeshes,
		scene->mNumMaterials,
		scene->mNumAnimations,
		scene->mNumTextures
	);

	logger::warning("Scene HasSkeletons(): {}", scene->HasSkeletons());
	logger::warning("Animations: {}", scene->mNumAnimations);

	bool hasAnyMeshBones = false;
	for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
	{
		const aiMesh* mesh = scene->mMeshes[mi];
		if (mesh && mesh->mNumBones > 0)
		{
			hasAnyMeshBones = true;
			break;
		}
	}
	logger::warning("Any mesh has bones (mesh->mNumBones > 0): {}", hasAnyMeshBones);

	if (scene->mNumAnimations > 0)
	{
		const aiAnimation* anim = scene->mAnimations[0];
		logger::info("Anim[0] name='{}' duration={} ticksPerSecond={} channels={}",
			(anim->mName.length ? anim->mName.C_Str() : "<no-name>"),
			anim->mDuration,
			anim->mTicksPerSecond,
			anim->mNumChannels
		);

		// print first few channel node names
		const unsigned int show = (anim->mNumChannels < 10) ? anim->mNumChannels : 10;
		for (unsigned int c = 0; c < show; ++c)
		{
			const aiNodeAnim* ch = anim->mChannels[c];
			logger::info("  Channel[{}] node='{}' posKeys={} rotKeys={} scaleKeys={}",
				c,
				ch->mNodeName.C_Str(),
				ch->mNumPositionKeys,
				ch->mNumRotationKeys,
				ch->mNumScalingKeys
			);
		}
	}
}

void foxanim::FxAnim::ImguiView()
{

}
