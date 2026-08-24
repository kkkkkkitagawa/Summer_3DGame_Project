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
	bool IsKnockbackActive() const { return isKnockbackActive_; }

	static inline constexpr float kCollisionScale = 0.9f;
	static inline const KamataEngine::Vector3 kCollisionHalfSize = {
	    0.5f * kCollisionScale,
	    0.5f * kCollisionScale,
	    0.5f * kCollisionScale,
	};
	static inline const KamataEngine::Vector3 kModelScale = {
	    2.0f,
	    2.0f,
	    2.0f,
	};
	static inline constexpr float kModelSourceHalfSize = 0.25f;

private:
	void UpdateTransforms();
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::WorldTransform outlineWorldTransform_;
	KamataEngine::Model* model_ = nullptr;
	float knockbackStartX_ = 0.0f;
	float knockbackDistance_ = 0.0f;
	float knockbackDuration_ = 0.0f;
	float knockbackElapsed_ = 0.0f;
	bool isKnockbackActive_ = false;
};
