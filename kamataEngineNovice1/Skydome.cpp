#include "Skydome.h"

#include "WorldTransformUpdate.h"

#include <cassert>

void Skydome::Initialize(KamataEngine::Model* model) {
	assert(model);
	model_ = model;
	textureHandle_ =
	    KamataEngine::TextureManager::Load("SkyDome/sky_sphere.png");

	// A sky sphere should reproduce its texture without directional-light
	// shading. Explicitly normalize every imported material so a replacement
	// texture is not darkened by Blender's exported material values.
	for (const std::unique_ptr<KamataEngine::Mesh>& mesh : model_->GetMeshes()) {
		KamataEngine::Material* material = mesh->GetMaterial();
		if (!material) {
			continue;
		}
		material->ambient_ = {1.0f, 1.0f, 1.0f};
		material->diffuse_ = {0.0f, 0.0f, 0.0f};
		material->specular_ = {0.0f, 0.0f, 0.0f};
		material->Update();
	}

	worldTransform_.Initialize();
	WorldTransformUpdate(worldTransform_);
}

void Skydome::Update() {
	WorldTransformUpdate(worldTransform_);
}

void Skydome::Draw(const KamataEngine::Camera& camera) const {
	model_->Draw(worldTransform_, camera, textureHandle_);
}
