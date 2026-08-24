#include "FallenBlockCounter.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <numbers>
#include <string>

using namespace KamataEngine;

FallenBlockCounter::~FallenBlockCounter() {
	for (Model* model : digitModels_) {
		delete model;
	}
}

void FallenBlockCounter::Initialize() {
	for (std::size_t digit = 0; digit < digitModels_.size(); ++digit) {
		digitModels_[digit] =
		    Model::CreateFromOBJ("num" + std::to_string(digit), false);
		assert(digitModels_[digit]);
	}
	textureHandle_ = TextureManager::Load("number/num/num.png");

	for (WorldTransform& transform : digitTransforms_) {
		transform.Initialize();
		// Blender文字のX-Z平面を画面へ正対させる。
		transform.rotation_.x = std::numbers::pi_v<float> * 0.5f;
	}
	pendingValue_ = 0;
	animationPhase_ = AnimationPhase::Stable;
	animationTime_ = 0.0f;
	SetDisplayedValue(0);
	ApplyAnimationScale(1.0f);
}

void FallenBlockCounter::Update(std::size_t value) {
	pendingValue_ = value;
	if (animationPhase_ == AnimationPhase::Stable) {
		if (pendingValue_ == displayedValue_) {
			return;
		}
		animationPhase_ = AnimationPhase::Shrinking;
		animationTime_ = 0.0f;
	}

	animationTime_ += kDeltaTime;
	if (animationPhase_ == AnimationPhase::Shrinking) {
		const float progress =
		    std::clamp(animationTime_ / kShrinkDuration, 0.0f, 1.0f);
		// Ease-in cubic: the old number quickly contracts into its center.
		ApplyAnimationScale(1.0f - progress * progress * progress);
		if (progress >= 1.0f) {
			SetDisplayedValue(pendingValue_);
			animationPhase_ = AnimationPhase::Growing;
			animationTime_ = 0.0f;
			ApplyAnimationScale(0.0f);
		}
		return;
	}

	const float progress =
	    std::clamp(animationTime_ / kGrowDuration, 0.0f, 1.0f);
	const float inverseProgress = 1.0f - progress;
	// Ease-out cubic: the new number rapidly returns to its normal size.
	ApplyAnimationScale(
	    1.0f - inverseProgress * inverseProgress * inverseProgress);
	if (progress >= 1.0f) {
		ApplyAnimationScale(1.0f);
		animationTime_ = 0.0f;
		animationPhase_ = pendingValue_ == displayedValue_
		                      ? AnimationPhase::Stable
		                      : AnimationPhase::Shrinking;
	}
}

void FallenBlockCounter::SetDisplayedValue(std::size_t value) {
	displayedValue_ = value;
	const std::string text = std::to_string(value);
	visibleDigitCount_ = (std::min)(text.size(), digitTransforms_.size());
	const float totalWidth =
	    static_cast<float>(visibleDigitCount_ - 1) * kDigitSpacing;

	for (std::size_t index = 0; index < visibleDigitCount_; ++index) {
		displayedDigits_[index] =
		    static_cast<std::size_t>(text[index] - '0');
		WorldTransform& transform = digitTransforms_[index];
		transform.translation_ = {
		    -totalWidth * 0.5f + static_cast<float>(index) * kDigitSpacing,
		    kTopPositionY,
		    0.0f,
		};
		WorldTransformUpdate(transform);
	}
}

void FallenBlockCounter::ApplyAnimationScale(float scaleRatio) {
	const float animatedScale = kModelScale * scaleRatio;
	for (std::size_t index = 0; index < visibleDigitCount_; ++index) {
		WorldTransform& transform = digitTransforms_[index];
		transform.scale_ = {
		    -animatedScale, animatedScale, animatedScale};
		WorldTransformUpdate(transform);
	}
}

void FallenBlockCounter::Draw(const Camera& camera) const {
	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kOff);
	for (std::size_t index = 0; index < visibleDigitCount_; ++index) {
		digitModels_[displayedDigits_[index]]->Draw(
		    digitTransforms_[index], camera, textureHandle_);
	}
	Model::PostDraw();
}
