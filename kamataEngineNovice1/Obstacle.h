#pragma once

#include "Collision.h"

enum class BlockFace {
	Top,
	Bottom,
	Front,
	Back,
};

enum class ObstacleSurfaceRelation {
	PlayerFace,
	PositiveSide,
	NegativeSide,
	OppositeFace,
};

struct ObstacleInteractionRules {
	bool pushesPlayerOnPlayerFace = true;
	bool blocksRotationFromSide = true;
};

class Obstacle {
public:
	void Initialize(
	    KamataEngine::Model* model, BlockFace attachedFace,
	    KamataEngine::WorldTransform* parent, float blockHalfSize,
	    const KamataEngine::Vector3& size,
	    ObstacleInteractionRules interactionRules, float visualHeightScale,
	    float visualGrowthDuration);
	void Update(
	    float deltaTime, float gravity, float timeUntilGrowthDeadline,
	    bool canStartVisualGrowth);
	void DetachAndFall(
	    const KamataEngine::Vector3& inheritedVelocity,
	    float repulsionSpeed);
	void Draw(const KamataEngine::Camera& camera) const;

	AABB GetAABB() const;
	AABB GetAABBForParentTransform(
	    const KamataEngine::Matrix4x4& parentTransform) const;
	KamataEngine::Vector3 GetWorldPosition() const;
	static KamataEngine::Vector3 GetFaceNormal(BlockFace attachedFace);
	ObstacleSurfaceRelation GetSurfaceRelation(
	    float parentRotationX,
	    const KamataEngine::Vector3& playerSurfaceNormal) const;
	BlockFace GetAttachedFace() const { return attachedFace_; }
	const ObstacleInteractionRules& GetInteractionRules() const {
		return interactionRules_;
	}
	bool IsAttached() const { return worldTransform_.parent_ != nullptr; }
	bool IsFalling() const { return isFalling_; }
	bool IsCollisionEnabled() const { return isCollisionEnabled_; }
	void SetCollisionEnabled(bool isEnabled) {
		isCollisionEnabled_ = isEnabled;
	}

private:
	static KamataEngine::Vector3 CalculateLocalPosition(
	    BlockFace attachedFace, float blockHalfSize,
	    const KamataEngine::Vector3& size);
	void UpdateVisualTransform();
	AABB CalculateAABB(const KamataEngine::Matrix4x4& worldTransform) const;
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::WorldTransform visualWorldTransform_;
	KamataEngine::Model* model_ = nullptr;
	BlockFace attachedFace_ = BlockFace::Top;
	ObstacleInteractionRules interactionRules_;
	KamataEngine::Vector3 logicalSize_ = {1.0f, 1.0f, 1.0f};
	KamataEngine::Vector3 velocity_ = {};
	float blockHalfSize_ = 0.5f;
	float visualHeightScale_ = 1.0f;
	float visualGrowthDuration_ = 1.0f;
	float visualGrowthElapsed_ = 0.0f;
	bool isVisualGrowthStarted_ = false;
	bool isCollisionEnabled_ = true;
	bool isFalling_ = false;

	// A truly zero scale makes the lighting inverse matrix singular. This value
	// is visually flat while keeping the render transform valid.
	static inline constexpr float kMinimumVisualHeightScale = 0.001f;
};
