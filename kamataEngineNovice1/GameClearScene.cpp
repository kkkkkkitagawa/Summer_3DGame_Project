#include "GameClearScene.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
float EaseOutBack(float value) {
	constexpr float overshoot = 1.70158f;
	const float shifted = value - 1.0f;
	return 1.0f + (overshoot + 1.0f) * shifted * shifted * shifted +
	       overshoot * shifted * shifted;
}

float EaseOutCubic(float value) {
	const float inverse = 1.0f - value;
	return 1.0f - inverse * inverse * inverse;
}
} // namespace

GameClearScene::~GameClearScene() {
	delete playerModel_;
	delete titleModel_;
}

void GameClearScene::Initialize() {
	playerModel_ = Model::CreateFromOBJ("player", false);
	assert(playerModel_);
	playerTextureHandle_ = TextureManager::Load("player/player.png");
	titleModel_ = Model::CreateFromOBJ("GameClear", false);
	assert(titleModel_);
	titleTextureHandle_ = TextureManager::Load("GameClear/GameOver.png");
	playerWorldTransform_.Initialize();
	playerOutlineWorldTransform_.Initialize();
	titleWorldTransform_.Initialize();
	titleOutlineWorldTransform_.Initialize();
	outlineColor_.Initialize();
	outlineColor_.SetColor({0.0f, 0.0f, 0.0f, 1.0f});
	UpdateTransforms(0.0f);
}

void GameClearScene::Start() {
	phase_ = Phase::Entering;
	animationTime_ = 0.0f;
	canvasOpacity_ = 0.0f;
	playerRotation_ = 0.0f;
	playerWorldTransform_.translation_ = {
	    kPlayerTargetX, kPlayerStartY, 0.0f};
	playerWorldTransform_.rotation_ = {};
	titleWorldTransform_.rotation_ = {
	    std::numbers::pi_v<float> * 0.5f,
	    0.0f,
	    4.0f * std::numbers::pi_v<float>,
	};
	UpdateTransforms(0.0f);
}

void GameClearScene::Update() {
	if (phase_ == Phase::Hidden) {
		return;
	}
	animationTime_ += kDeltaTime;
	canvasOpacity_ = kCanvasTargetOpacity * std::clamp(
	    animationTime_ / kCanvasFadeDuration, 0.0f, 1.0f);
	playerRotation_ += kDeltaTime * 0.85f;

	const float playerProgress = std::clamp(
	    animationTime_ / kPlayerDropDuration, 0.0f, 1.0f);
	playerWorldTransform_.translation_ = {
	    kPlayerTargetX,
	    kPlayerStartY + (kPlayerTargetY - kPlayerStartY) *
	                        EaseOutBack(playerProgress),
	    0.0f,
	};
	playerWorldTransform_.rotation_ = {
	    playerRotation_ * 0.35f,
	    playerRotation_,
	    -playerRotation_ * 0.20f,
	};

	const float titleProgress = std::clamp(
	    animationTime_ / kTitleEnterDuration, 0.0f, 1.0f);
	float titleScaleRatio = EaseOutBack(titleProgress);
	titleWorldTransform_.rotation_.x = std::numbers::pi_v<float> * 0.5f;
	titleWorldTransform_.rotation_.z =
	    (1.0f - titleProgress) * 4.0f * std::numbers::pi_v<float>;
	if (titleProgress >= 1.0f) {
		phase_ = Phase::Waiting;
		titleScaleRatio =
		    1.0f + std::sin(animationTime_ * 7.0f) * kTitleBounceAmplitude;
		titleWorldTransform_.rotation_.z = 0.0f;
	}
	UpdateTransforms(titleScaleRatio);
}

void GameClearScene::UpdateTransforms(float titleScaleRatio) {
	playerWorldTransform_.scale_ = {
	    kPlayerBaseScale, kPlayerBaseScale, kPlayerBaseScale};
	playerOutlineWorldTransform_.scale_ = {
	    kPlayerBaseScale + kPlayerOutlineExpansion,
	    kPlayerBaseScale + kPlayerOutlineExpansion,
	    kPlayerBaseScale + kPlayerOutlineExpansion,
	};
	playerOutlineWorldTransform_.translation_ =
	    playerWorldTransform_.translation_;
	playerOutlineWorldTransform_.rotation_ = playerWorldTransform_.rotation_;
	WorldTransformUpdate(playerWorldTransform_);
	WorldTransformUpdate(playerOutlineWorldTransform_);

	const float titleScale = kTitleBaseScale * titleScaleRatio;
	titleWorldTransform_.scale_ = {-titleScale, titleScale, titleScale};
	titleWorldTransform_.translation_ = {0.0f, 0.0f, 0.15f};
	const float outlineExpansion = kTitleOutlineExpansion * std::clamp(
	    std::abs(titleScaleRatio), 0.0f, 1.0f);
	titleOutlineWorldTransform_.scale_ = {
	    -(titleScale + outlineExpansion),
	    titleScale + outlineExpansion,
	    titleScale + outlineExpansion,
	};
	titleOutlineWorldTransform_.translation_ =
	    titleWorldTransform_.translation_;
	titleOutlineWorldTransform_.rotation_ = titleWorldTransform_.rotation_;
	WorldTransformUpdate(titleWorldTransform_);
	WorldTransformUpdate(titleOutlineWorldTransform_);
}

void GameClearScene::Draw(const Camera& camera) const {
	if (phase_ == Phase::Hidden) {
		return;
	}
	DrawPlayer(camera);
	DrawTitle(camera);
}

void GameClearScene::DrawPlayer(const Camera& camera) const {
	Model::PreDraw(
	    Model::CullingMode::kFront, Model::BlendMode::kNone,
	    Model::DepthTestMode::kOff);
	playerModel_->Draw(
	    playerOutlineWorldTransform_, camera, playerTextureHandle_,
	    &outlineColor_);
	Model::PostDraw();
	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kOff);
	playerModel_->Draw(playerWorldTransform_, camera, playerTextureHandle_);
	Model::PostDraw();
}

void GameClearScene::DrawTitle(const Camera& camera) const {
	Model::PreDraw(
	    Model::CullingMode::kFront, Model::BlendMode::kNone,
	    Model::DepthTestMode::kOff);
	titleModel_->Draw(
	    titleOutlineWorldTransform_, camera, titleTextureHandle_,
	    &outlineColor_);
	Model::PostDraw();
	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kOff);
	titleModel_->Draw(titleWorldTransform_, camera, titleTextureHandle_);
	Model::PostDraw();
}
