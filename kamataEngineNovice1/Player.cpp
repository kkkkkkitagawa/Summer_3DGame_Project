#include "Player.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>

void Player::Initialize(
    KamataEngine::Model* model, const KamataEngine::Vector3& position,
    float outlineThickness) {
	assert(model);
	assert(outlineThickness >= 0.0f);
	model_ = model;

	worldTransform_.Initialize();
	worldTransform_.scale_ = kModelScale;
	worldTransform_.translation_ = position;
	outlineWorldTransform_.Initialize();
	const float outlineScaleExpansion =
	    outlineThickness / kModelSourceHalfSize;
	outlineWorldTransform_.scale_ = {
	    kModelScale.x + outlineScaleExpansion,
	    kModelScale.y + outlineScaleExpansion,
	    kModelScale.z + outlineScaleExpansion,
	};
	outlineWorldTransform_.translation_ = position;
	UpdateTransforms();
}

void Player::Update(
    float forwardSpeed, float maximumPositionX, float deltaTime) {
	worldTransform_.translation_.x = (std::min)(
	    maximumPositionX,
	    worldTransform_.translation_.x + forwardSpeed * deltaTime);
	UpdateTransforms();
}

void Player::Draw(const KamataEngine::Camera& camera) const {
	model_->Draw(worldTransform_, camera);
}

void Player::DrawOutline(
    const KamataEngine::Camera& camera,
    const KamataEngine::ObjectColor& outlineColor) const {
	model_->Draw(outlineWorldTransform_, camera, &outlineColor);
}

KamataEngine::Vector3 Player::GetWorldPosition() const {
	return {
	    worldTransform_.matWorld_.m[3][0],
	    worldTransform_.matWorld_.m[3][1],
	    worldTransform_.matWorld_.m[3][2],
	};
}

AABB Player::GetAABB() const {
	return MakeAABB(GetWorldPosition(), kCollisionHalfSize);
}

void Player::SetPositionX(float positionX) {
	worldTransform_.translation_.x = positionX;
	UpdateTransforms();
}

void Player::UpdateTransforms() {
	outlineWorldTransform_.translation_ = worldTransform_.translation_;
	outlineWorldTransform_.rotation_ = worldTransform_.rotation_;
	WorldTransformUpdate(worldTransform_);
	WorldTransformUpdate(outlineWorldTransform_);
}
