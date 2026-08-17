#include "Player.h"

#include "WorldTransformUpdate.h"

#include <cassert>

void Player::Initialize(
    KamataEngine::Model* model, const KamataEngine::Vector3& position) {
	assert(model);
	model_ = model;

	worldTransform_.Initialize();
	worldTransform_.scale_ = kModelScale;
	worldTransform_.translation_ = position;
	WorldTransformUpdate(worldTransform_);
}

void Player::Update() {
	WorldTransformUpdate(worldTransform_);
}

void Player::Draw(const KamataEngine::Camera& camera) const {
	model_->Draw(worldTransform_, camera);
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
