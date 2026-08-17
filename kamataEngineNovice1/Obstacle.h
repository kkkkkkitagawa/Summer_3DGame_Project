#pragma once

#include "Collision.h"

enum class BlockFace {
	Top,
	Bottom,
	Front,
	Back,
};

class Obstacle {
public:
	void Initialize(
	    KamataEngine::Model* model, BlockFace attachedFace,
	    KamataEngine::WorldTransform* parent, float blockHalfSize,
	    const KamataEngine::Vector3& size);
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;

	AABB GetAABB() const;
	BlockFace GetAttachedFace() const { return attachedFace_; }
	bool IsCollisionEnabled() const { return isCollisionEnabled_; }
	void SetCollisionEnabled(bool isEnabled) {
		isCollisionEnabled_ = isEnabled;
	}

private:
	static KamataEngine::Vector3 CalculateLocalPosition(
	    BlockFace attachedFace, float blockHalfSize,
	    const KamataEngine::Vector3& size);

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	BlockFace attachedFace_ = BlockFace::Top;
	bool isCollisionEnabled_ = true;
};
