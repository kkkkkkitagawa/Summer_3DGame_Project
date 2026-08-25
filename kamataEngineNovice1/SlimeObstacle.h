#pragma once

#include "Obstacle.h"

#include <cstdint>
#include <memory>
#include <random>
#include <vector>

class SlimeObstacle {
public:
	void Initialize(
	    KamataEngine::Model* innerModel,
	    KamataEngine::Model* outerModel, uint32_t innerTextureHandle,
	    uint32_t outerTextureHandle, uint32_t effectSeed,
	    BlockFace attachedFace,
	    KamataEngine::WorldTransform* parent, float blockHalfSize,
	    const KamataEngine::Vector3& size);
	void Update(float deltaTime, float gravity);
	bool TriggerHit();
	void StartClearRetraction(float duration);
	void DetachAndFall(
	    const KamataEngine::Vector3& inheritedVelocity,
	    float repulsionSpeed);
	void DrawInner(const KamataEngine::Camera& camera) const;
	void DrawOuter(const KamataEngine::Camera& camera) const;

	AABB GetAABBForParentTransform(
	    const KamataEngine::Matrix4x4& parentTransform) const;
	KamataEngine::Vector3 GetWorldPosition() const;
	ObstacleSurfaceRelation GetSurfaceRelation(
	    float parentRotationX,
	    const KamataEngine::Vector3& playerSurfaceNormal) const;
	bool IsCollisionEnabled() const { return isCollisionEnabled_; }
	bool IsAttached() const { return logicalWorldTransform_.parent_ != nullptr; }
	bool IsDead() const { return animationPhase_ == AnimationPhase::Dead; }

private:
	enum class AnimationPhase {
		Alive,
		Squashing,
		Expanding,
		Clearing,
		Bursting,
		Dead,
	};
	struct BurstFragment {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::Vector3 velocity = {};
		KamataEngine::Vector3 rotationVelocity = {};
		float elapsedTime = 0.0f;
	};

	static KamataEngine::Vector3 CalculateLocalPosition(
	    BlockFace attachedFace, float blockHalfSize,
	    const KamataEngine::Vector3& size);
	AABB CalculateAABB(
	    const KamataEngine::Matrix4x4& worldTransform) const;
	void UpdateAnimation(float deltaTime);
	void StartBurst();
	void UpdateBurstFragments(float deltaTime, float gravity);
	void UpdateVisualTransforms();

	KamataEngine::WorldTransform logicalWorldTransform_;
	KamataEngine::WorldTransform innerWorldTransform_;
	KamataEngine::WorldTransform outerWorldTransform_;
	KamataEngine::ObjectColor innerColor_;
	KamataEngine::ObjectColor outerColor_;
	KamataEngine::Model* innerModel_ = nullptr;
	KamataEngine::Model* outerModel_ = nullptr;
	uint32_t innerTextureHandle_ = 0;
	uint32_t outerTextureHandle_ = 0;
	BlockFace attachedFace_ = BlockFace::Top;
	KamataEngine::Vector3 logicalSize_ = {1.0f, 1.0f, 1.0f};
	KamataEngine::Vector3 velocity_ = {};
	std::vector<std::unique_ptr<BurstFragment>> burstFragments_;
	std::mt19937 effectRandomEngine_;
	float blockHalfSize_ = 0.5f;
	float visualScale_ = 1.0f;
	float opacity_ = 1.0f;
	float animationTime_ = 0.0f;
	float clearRetractionStartScale_ = 1.0f;
	float clearRetractionDuration_ = 0.4f;
	int hitPoints_ = 1;
	bool isCollisionEnabled_ = true;
	bool isFalling_ = false;
	AnimationPhase animationPhase_ = AnimationPhase::Alive;

	static inline constexpr float kSquashedScale = 0.8f;
	static inline constexpr float kExpandedScale = 1.2f;
	static inline constexpr float kSquashDuration = 0.06f;
	static inline constexpr float kExpandDuration = 0.08f;
	static inline constexpr float kMinimumClearScale = 0.001f;
	static inline constexpr float kClearCollisionDisableScale = 0.3f;
	static inline constexpr int kMinimumBurstFragmentCount = 4;
	static inline constexpr int kMaximumBurstFragmentCount = 8;
	static inline constexpr float kMinimumFragmentScale = 0.08f;
	static inline constexpr float kMaximumFragmentScale = 0.14f;
	static inline constexpr float kMinimumHorizontalBurstSpeed = 2.0f;
	static inline constexpr float kMaximumHorizontalBurstSpeed = 4.0f;
	static inline constexpr float kMinimumUpwardBurstSpeed = 3.0f;
	static inline constexpr float kMaximumUpwardBurstSpeed = 4.2f;
	static inline constexpr float kFragmentLifetime = 2.5f;
};
