#include "Skydome.h"

#include "WorldTransformUpdate.h"

#include <cassert>

void Skydome::Initialize(KamataEngine::Model* model) {
	assert(model);
	model_ = model;

	worldTransform_.Initialize();
	WorldTransformUpdate(worldTransform_);
}

void Skydome::Update() {
	WorldTransformUpdate(worldTransform_);
}

void Skydome::Draw(const KamataEngine::Camera& camera) const {
	model_->Draw(worldTransform_, camera);
}
