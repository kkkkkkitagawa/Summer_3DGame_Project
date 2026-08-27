#include "TitleScene.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
float EaseInBack(float value) {
	constexpr float overshoot = 1.70158f;
	const float extendedOvershoot = overshoot + 1.0f;
	return extendedOvershoot * value * value * value -
	       overshoot * value * value;
}
} // namespace

TitleScene::~TitleScene() { delete model_; }

void TitleScene::Initialize() {
	model_ = Model::CreateFromOBJ("Title", false);
	assert(model_);
	textureHandle_ = TextureManager::Load("Title/title.png");
	worldTransform_.Initialize();
	worldTransform_.scale_ = {-kModelScale, kModelScale, kModelScale};
	// Blender由来の文字はローカルX-Z平面にあるため画面へ正対させる。
	// +90度では裏面になるため、X反転も組み合わせて鏡像を補正する。
	worldTransform_.rotation_.x = std::numbers::pi_v<float> * 0.5f;
	Reset();
}

void TitleScene::Reset() {
	phase_ = Phase::Idle;
	animationTime_ = 0.0f;
	exitStartY_ = kBaseY;
	worldTransform_.translation_ = {0.0f, kBaseY, 0.0f};
	WorldTransformUpdate(worldTransform_);
}

void TitleScene::StartExit() {
	if (phase_ != Phase::Idle) {
		return;
	}
	phase_ = Phase::Exiting;
	animationTime_ = 0.0f;
	exitStartY_ = worldTransform_.translation_.y;
}

void TitleScene::Update() {
	animationTime_ += kDeltaTime;
	switch (phase_) {
	case Phase::Idle:
		worldTransform_.translation_.y =
		    kBaseY + std::sin(
		                 animationTime_ * 2.0f *
		                 std::numbers::pi_v<float> / kFloatingCycle) *
		                 kFloatingAmplitude;
		break;
	case Phase::Exiting: {
		const float progress = std::clamp(
		    animationTime_ / kExitDuration, 0.0f, 1.0f);
		const float easedProgress = EaseInBack(progress);
		worldTransform_.translation_.y =
		    exitStartY_ + (kOffscreenY - exitStartY_) * easedProgress;
		if (progress >= 1.0f) {
			phase_ = Phase::Finished;
		}
		break;
	}
	case Phase::Finished:
		break;
	}
	WorldTransformUpdate(worldTransform_);
}

void TitleScene::Draw(const Camera& camera) const {
	if (phase_ == Phase::Finished) {
		return;
	}
	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kOff);
	model_->Draw(worldTransform_, camera, textureHandle_);
	Model::PostDraw();
}
