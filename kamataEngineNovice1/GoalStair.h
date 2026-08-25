#pragma once

#include "Obstacle.h"

class GoalStair {
public:
	void Initialize(
	    KamataEngine::Model* model, BlockFace attachedFace,
	    KamataEngine::WorldTransform* parent, float blockHalfSize,
	    float outlineThickness);
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;
	void DrawOutline(
	    const KamataEngine::Camera& camera,
	    const KamataEngine::ObjectColor& outlineColor) const;

private:
	KamataEngine::Matrix4x4 CreateLocalTransform(float scale) const;
	static KamataEngine::Vector3 Cross(
	    const KamataEngine::Vector3& first,
	    const KamataEngine::Vector3& second);

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::WorldTransform outlineWorldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::WorldTransform* parent_ = nullptr;
	BlockFace attachedFace_ = BlockFace::Top;
	float modelScale_ = 0.5f;
	float surfaceOffset_ = 0.65f;
	float outlineThickness_ = 0.0f;

	static inline constexpr float kModelLowestY = -0.3f;
};
