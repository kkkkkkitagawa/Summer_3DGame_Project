#include "Player.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

void Player::Initialize(
    KamataEngine::Model* model, const KamataEngine::Vector3& position,
    float outlineThickness) {
	assert(model);
	assert(outlineThickness >= 0.0f);
	model_ = model;

	worldTransform_.Initialize();
	logicalPosition_ = position;
	outlineWorldTransform_.Initialize();
	outlineScaleExpansion_ =
	    outlineThickness / kModelSourceHalfSize;
	UpdateTransforms();
}

void Player::Update(
    float forwardSpeed, float maximumPositionX, float deltaTime) {
	const float previousPositionX = logicalPosition_.x;
	const bool wasKnockbackActive = isKnockbackActive_;
	if (wasKnockbackActive) {
		knockbackElapsed_ =
		    (std::min)(knockbackElapsed_ + deltaTime, knockbackDuration_);
		const float progress = std::clamp(
		    knockbackElapsed_ / knockbackDuration_, 0.0f, 1.0f);
		const float inverseProgress = 1.0f - progress;
		const float easedProgress =
		    1.0f - inverseProgress * inverseProgress * inverseProgress;
		logicalPosition_.x =
		    knockbackStartX_ - knockbackDistance_ * easedProgress;
		if (progress >= 1.0f) {
			isKnockbackActive_ = false;
		}
	} else {
		logicalPosition_.x = (std::min)(
		    maximumPositionX,
		    logicalPosition_.x + forwardSpeed * deltaTime);
	}

	const float visualTravel = wasKnockbackActive
	                               ? logicalPosition_.x - previousPositionX
	                               : forwardSpeed * deltaTime;
	const float visualRadius = kModelSourceHalfSize * kModelScale.y;
	rollingRotationZ_ -= visualTravel / visualRadius;
	UpdateTurnJump(deltaTime);
	UpdateTransforms();
}

void Player::UpdateGoalRun(float forwardSpeed, float deltaTime) {
	logicalPosition_.x += forwardSpeed * deltaTime;
	const float visualRadius = kModelSourceHalfSize * kModelScale.y;
	rollingRotationZ_ -= forwardSpeed * deltaTime / visualRadius;
	UpdateTurnJump(deltaTime);
	UpdateTransforms();
}

void Player::StartClearLaunch() {
	clearLaunchElapsed_ = 0.0f;
	clearVisualOffsetX_ = 0.0f;
	jumpVisualOffsetY_ = 0.0f;
	animationScale_ = {1.0f, 1.0f, 1.0f};
	isTurnJumpActive_ = false;
	isClearLaunchFinished_ = false;
	isVisible_ = true;
}

void Player::UpdateClearLaunch(float deltaTime) {
	if (isClearLaunchFinished_) {
		return;
	}
	clearLaunchElapsed_ =
	    (std::min)(clearLaunchElapsed_ + deltaTime, kClearLaunchDuration);
	const float progress = std::clamp(
	    clearLaunchElapsed_ / kClearLaunchDuration, 0.0f, 1.0f);
	const KamataEngine::Vector3 normalScale = {1.0f, 1.0f, 1.0f};
	const KamataEngine::Vector3 chargeScale = {1.12f, 0.72f, 1.12f};
	const KamataEngine::Vector3 launchScale = {0.76f, 1.34f, 0.76f};
	auto smoothStep = [](float value) {
		const float clampedValue = std::clamp(value, 0.0f, 1.0f);
		return clampedValue * clampedValue *
		       (3.0f - 2.0f * clampedValue);
	};
	auto lerpScale = [](
	                     const KamataEngine::Vector3& start,
	                     const KamataEngine::Vector3& end, float amount) {
		return KamataEngine::Vector3{
		    start.x + (end.x - start.x) * amount,
		    start.y + (end.y - start.y) * amount,
		    start.z + (end.z - start.z) * amount,
		};
	};

	if (progress < kClearChargeEnd) {
		const float phaseProgress =
		    smoothStep(progress / kClearChargeEnd);
		animationScale_ =
		    lerpScale(normalScale, chargeScale, phaseProgress);
		clearVisualOffsetX_ = 0.0f;
		jumpVisualOffsetY_ = 0.0f;
	} else {
		const float launchProgress = std::clamp(
		    (progress - kClearChargeEnd) / (1.0f - kClearChargeEnd),
		    0.0f, 1.0f);
		const float easedProgress =
		    launchProgress * launchProgress * launchProgress;
		animationScale_ = lerpScale(
		    chargeScale, launchScale, smoothStep(launchProgress));
		clearVisualOffsetX_ = kClearLaunchForwardDistance * easedProgress;
		jumpVisualOffsetY_ = kClearLaunchHeight * easedProgress;
		rollingRotationZ_ -= deltaTime * 12.0f;
	}
	UpdateTransforms();
	if (progress >= 1.0f) {
		isClearLaunchFinished_ = true;
		isVisible_ = false;
	}
}

void Player::Draw(const KamataEngine::Camera& camera) const {
	if (!isVisible_) {
		return;
	}
	model_->Draw(worldTransform_, camera);
}

void Player::DrawOutline(
    const KamataEngine::Camera& camera,
    const KamataEngine::ObjectColor& outlineColor) const {
	if (!isVisible_) {
		return;
	}
	model_->Draw(outlineWorldTransform_, camera, &outlineColor);
}

KamataEngine::Vector3 Player::GetWorldPosition() const {
	return logicalPosition_;
}

AABB Player::GetAABB() const {
	return MakeAABB(GetWorldPosition(), kCollisionHalfSize);
}

void Player::SetPositionX(float positionX) {
	logicalPosition_.x = positionX;
	UpdateTransforms();
}

void Player::StartKnockback(float distance, float duration) {
	assert(distance > 0.0f);
	assert(duration > 0.0f);
	knockbackStartX_ = logicalPosition_.x;
	knockbackDistance_ = distance;
	knockbackDuration_ = duration;
	knockbackElapsed_ = 0.0f;
	isKnockbackActive_ = true;
}

void Player::StartTurnJump() {
	turnJumpElapsed_ = 0.0f;
	isTurnJumpActive_ = true;
}

void Player::UpdateTurnJump(float deltaTime) {
	if (!isTurnJumpActive_) {
		animationScale_ = {1.0f, 1.0f, 1.0f};
		jumpVisualOffsetY_ = 0.0f;
		return;
	}

	turnJumpElapsed_ =
	    (std::min)(turnJumpElapsed_ + deltaTime, kTurnJumpDuration);
	const float progress =
	    std::clamp(turnJumpElapsed_ / kTurnJumpDuration, 0.0f, 1.0f);
	const KamataEngine::Vector3 normalScale = {1.0f, 1.0f, 1.0f};
	const KamataEngine::Vector3 preSquashScale = {1.06f, 0.90f, 1.06f};
	const KamataEngine::Vector3 stretchScale = {0.96f, 1.08f, 0.96f};
	const KamataEngine::Vector3 landingSquashScale = {1.05f, 0.91f, 1.05f};
	auto smoothStep = [](float value) {
		const float clampedValue = std::clamp(value, 0.0f, 1.0f);
		return clampedValue * clampedValue *
		       (3.0f - 2.0f * clampedValue);
	};
	auto lerpScale = [](const KamataEngine::Vector3& start,
	                    const KamataEngine::Vector3& end, float amount) {
		return KamataEngine::Vector3{
		    start.x + (end.x - start.x) * amount,
		    start.y + (end.y - start.y) * amount,
		    start.z + (end.z - start.z) * amount,
		};
	};

	if (progress < kPreSquashEnd) {
		const float phaseProgress = smoothStep(progress / kPreSquashEnd);
		animationScale_ =
		    lerpScale(normalScale, preSquashScale, phaseProgress);
		jumpVisualOffsetY_ = 0.0f;
	} else if (progress < kAirborneEnd) {
		const float airborneProgress =
		    (progress - kPreSquashEnd) /
		    (kAirborneEnd - kPreSquashEnd);
		jumpVisualOffsetY_ =
		    kTurnJumpHeight *
		    std::sin(std::numbers::pi_v<float> * airborneProgress);
		if (airborneProgress < 0.5f) {
			const float phaseProgress =
			    smoothStep(airborneProgress * 2.0f);
			animationScale_ = lerpScale(
			    preSquashScale, stretchScale, phaseProgress);
		} else {
			const float phaseProgress =
			    smoothStep((airborneProgress - 0.5f) * 2.0f);
			animationScale_ = lerpScale(
			    stretchScale, landingSquashScale, phaseProgress);
		}
	} else {
		const float phaseProgress = smoothStep(
		    (progress - kAirborneEnd) / (1.0f - kAirborneEnd));
		animationScale_ =
		    lerpScale(landingSquashScale, normalScale, phaseProgress);
		jumpVisualOffsetY_ = 0.0f;
	}

	if (progress >= 1.0f) {
		isTurnJumpActive_ = false;
		animationScale_ = normalScale;
		jumpVisualOffsetY_ = 0.0f;
	}
}

void Player::UpdateTransforms() {
	worldTransform_.scale_ = {
	    kModelScale.x * animationScale_.x,
	    kModelScale.y * animationScale_.y,
	    kModelScale.z * animationScale_.z,
	};
	worldTransform_.rotation_.z = rollingRotationZ_;
	worldTransform_.translation_ = logicalPosition_;
	worldTransform_.translation_.x += clearVisualOffsetX_;
	worldTransform_.translation_.y += jumpVisualOffsetY_;
	outlineWorldTransform_.scale_ = {
	    worldTransform_.scale_.x + outlineScaleExpansion_,
	    worldTransform_.scale_.y + outlineScaleExpansion_,
	    worldTransform_.scale_.z + outlineScaleExpansion_,
	};
	outlineWorldTransform_.translation_ = worldTransform_.translation_;
	outlineWorldTransform_.rotation_ = worldTransform_.rotation_;
	WorldTransformUpdate(worldTransform_);
	WorldTransformUpdate(outlineWorldTransform_);
}
