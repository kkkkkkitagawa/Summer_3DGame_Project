#pragma once

#include "Collision.h"

class Player {
public:
	void Initialize(
	    KamataEngine::Model* model, const KamataEngine::Vector3& position,
	    float outlineThickness);
	void Update(float forwardSpeed, float maximumPositionX, float deltaTime);
	void UpdateGoalRun(float forwardSpeed, float deltaTime);
	void StartClearLaunch();
	void UpdateClearLaunch(float deltaTime);
	void StartDeathFall();
	void UpdateDeathFall(float deltaTime);
	void Draw(const KamataEngine::Camera& camera) const;
	void DrawOutline(
	    const KamataEngine::Camera& camera,
	    const KamataEngine::ObjectColor& outlineColor) const;

	KamataEngine::Vector3 GetWorldPosition() const;
	AABB GetAABB() const;
	void SetPositionX(float positionX);
	void StartKnockback(float distance, float duration);
	void StartSlowdown(float duration, float speedMultiplier);
	void StartTurnJump();
	bool IsKnockbackActive() const { return isKnockbackActive_; }
	bool IsSlowdownActive() const { return isSlowdownActive_; }
	bool IsClearLaunchFinished() const { return isClearLaunchFinished_; }
	bool IsDeathFallFinished() const { return isDeathFallFinished_; }

	static inline constexpr float kCollisionScale = 0.9f;
	static inline const KamataEngine::Vector3 kCollisionHalfSize = {
	    0.5f * kCollisionScale,
	    0.5f * kCollisionScale,
	    0.5f * kCollisionScale,
	};
	static inline const KamataEngine::Vector3 kModelScale = {
	    0.5f,
	    0.5f,
	    0.5f,
	};
	static inline constexpr float kModelSourceHalfSize = 1.0f;

private:
	void UpdateSlowdown(float deltaTime);
	void UpdateTurnJump(float deltaTime);
	void UpdateTransforms();
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::WorldTransform outlineWorldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Vector3 logicalPosition_ = {};
	KamataEngine::Vector3 animationScale_ = {1.0f, 1.0f, 1.0f};
	float outlineScaleExpansion_ = 0.0f;
	float rollingRotationZ_ = 0.0f;
	float jumpVisualOffsetY_ = 0.0f;
	float clearVisualOffsetX_ = 0.0f;
	float deathVisualOffsetX_ = 0.0f;
	float deathVisualOffsetY_ = 0.0f;
	float turnJumpElapsed_ = 0.0f;
	float clearLaunchElapsed_ = 0.0f;
	float deathFallElapsed_ = 0.0f;
	float knockbackStartX_ = 0.0f;
	float knockbackDistance_ = 0.0f;
	float knockbackDuration_ = 0.0f;
	float knockbackElapsed_ = 0.0f;
	float slowdownDuration_ = 0.0f;
	float slowdownElapsed_ = 0.0f;
	float slowdownSpeedMultiplier_ = 1.0f;
	bool isKnockbackActive_ = false;
	bool isSlowdownActive_ = false;
	bool isTurnJumpActive_ = false;
	bool isClearLaunchFinished_ = false;
	bool isDeathFallFinished_ = false;
	bool isVisible_ = true;

	static inline constexpr float kTurnJumpDuration = 0.34f;
	static inline constexpr float kTurnJumpHeight = 0.20f;
	static inline constexpr float kPreSquashEnd = 0.18f;
	static inline constexpr float kAirborneEnd = 0.72f;
	static inline constexpr float kClearLaunchDuration = 0.85f;
	static inline constexpr float kClearChargeEnd = 0.26f;
	static inline constexpr float kClearLaunchHeight = 8.0f;
	static inline constexpr float kClearLaunchForwardDistance = 2.2f;
	static inline constexpr float kDeathFallDuration = 1.5f;
	static inline constexpr float kDeathBackwardSpeed = 1.15f;
	static inline constexpr float kDeathInitialUpwardSpeed = 2.6f;
	static inline constexpr float kDeathGravity = 6.5f;
	static inline constexpr float kDeathSpinSpeed = 8.0f;
};
