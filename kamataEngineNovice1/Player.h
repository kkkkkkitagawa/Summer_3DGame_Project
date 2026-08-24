#pragma once

#include "Collision.h"

class Player {
public:
	void Initialize(
	    KamataEngine::Model* model, const KamataEngine::Vector3& position,
	    float outlineThickness);
	void Update(float forwardSpeed, float maximumPositionX, float deltaTime);
	void Draw(const KamataEngine::Camera& camera) const;
	void DrawOutline(
	    const KamataEngine::Camera& camera,
	    const KamataEngine::ObjectColor& outlineColor) const;

	KamataEngine::Vector3 GetWorldPosition() const;
	AABB GetAABB() const;
	void SetPositionX(float positionX);
	void StartKnockback(float distance, float duration);
	void StartTurnJump();
	bool IsKnockbackActive() const { return isKnockbackActive_; }

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
	float turnJumpElapsed_ = 0.0f;
	float knockbackStartX_ = 0.0f;
	float knockbackDistance_ = 0.0f;
	float knockbackDuration_ = 0.0f;
	float knockbackElapsed_ = 0.0f;
	bool isKnockbackActive_ = false;
	bool isTurnJumpActive_ = false;

	static inline constexpr float kTurnJumpDuration = 0.34f;
	static inline constexpr float kTurnJumpHeight = 0.20f;
	static inline constexpr float kPreSquashEnd = 0.18f;
	static inline constexpr float kAirborneEnd = 0.72f;
};
