#pragma once

#include "KamataEngine.h"

class Skydome {
public:
	void Initialize(KamataEngine::Model* model);
	void Update(bool shouldRotate);
	void Draw(const KamataEngine::Camera& camera) const;

private:
	static inline constexpr float kReverseRotationSpeed = 0.018f;
	static inline constexpr float kModelScale = 0.12f;
	static inline constexpr float kDeltaTime = 1.0f / 60.0f;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
};
