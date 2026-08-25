#include "DeathHazardLine.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <numbers>

using namespace KamataEngine;

DeathHazardLine::~DeathHazardLine() { delete model_; }

void DeathHazardLine::Initialize(const Vector3& position) {
	model_ = Model::CreateFromOBJ("DDL_Hazard", false);
	assert(model_);
	textureHandle_ = TextureManager::Load("DDL_Hazard/DDL_Hazard.png");

	worldTransform_.Initialize();
	// OBJ的长边沿X轴。转向Z轴后，警戒线会横跨地图行进方向。
	worldTransform_.rotation_.y = std::numbers::pi_v<float> * 0.5f;
	worldTransform_.translation_ = position;
	WorldTransformUpdate(worldTransform_);

	color_.Initialize();
	color_.SetColor({1.0f, 1.0f, 1.0f, 0.0f});
}

void DeathHazardLine::Update(float playerPositionX) {
	if (playerPositionX > kVisibleStartX) {
		opacity_ = 0.0f;
	} else {
		const float progress = std::clamp(
		    (kVisibleStartX - playerPositionX) /
		        (kVisibleStartX - kMaximumOpacityX),
		    0.0f, 1.0f);
		opacity_ = kMinimumOpacity + (1.0f - kMinimumOpacity) * progress;
	}
	color_.SetColor({1.0f, 1.0f, 1.0f, opacity_});
}

void DeathHazardLine::Draw(const Camera& camera) const {
	if (opacity_ <= 0.0f) {
		return;
	}

	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kReadOnly);
	model_->Draw(worldTransform_, camera, textureHandle_, &color_);
	Model::PostDraw();
}
