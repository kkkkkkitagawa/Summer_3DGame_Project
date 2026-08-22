#include "GameOverScene.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
float EaseOutCubic(float value) {
	const float inverse = 1.0f - value;
	return 1.0f - inverse * inverse * inverse;
}
} // namespace

GameOverScene::~GameOverScene() { delete model_; }

void GameOverScene::Initialize() {
	model_ = Model::CreateFromOBJ("GameOver", false);
	assert(model_);
	textureHandle_ = TextureManager::Load("GameOver/GameOver.png");
	worldTransform_.Initialize();
	worldTransform_.scale_ = {-2.7f, 2.7f, 2.7f};
	worldTransform_.rotation_.x = std::numbers::pi_v<float> * 0.5f;
	worldTransform_.translation_ = {0.0f, kOffscreenY, 0.0f};
	WorldTransformUpdate(worldTransform_);
}

void GameOverScene::Start() {
	phase_ = Phase::Entering;
	animationTime_ = 0.0f;
	worldTransform_.translation_ = {0.0f, kOffscreenY, 0.0f};
	WorldTransformUpdate(worldTransform_);
}

void GameOverScene::Update() {
	if (phase_ == Phase::Hidden) {
		return;
	}
	animationTime_ += kDeltaTime;
	if (phase_ == Phase::Entering) {
		const float progress = std::clamp(
		    animationTime_ / kEnterDuration, 0.0f, 1.0f);
		worldTransform_.translation_.y =
		    kOffscreenY + (0.0f - kOffscreenY) * EaseOutCubic(progress);
		if (progress >= 1.0f) {
			phase_ = Phase::Waiting;
			animationTime_ = 0.0f;
		}
	} else {
		worldTransform_.translation_.y = 0.0f;
		worldTransform_.translation_.z =
		    std::sin(
		        animationTime_ * 2.0f * std::numbers::pi_v<float> /
		        kDepthCycle) *
		    kDepthAmplitude;
	}
	WorldTransformUpdate(worldTransform_);
}

void GameOverScene::Draw(const Camera& camera) const {
	if (phase_ == Phase::Hidden) {
		return;
	}
	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kOff);
	model_->Draw(worldTransform_, camera, textureHandle_);
	Model::PostDraw();
}
