#pragma once

#include "KamataEngine.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

// Decorative floating cubes placed in the lower part of the player camera.
class FloatingBlockSystem {
public:
	FloatingBlockSystem();
	~FloatingBlockSystem();

	void Initialize(
	    const KamataEngine::Camera& playerCamera, float mapGroundHeight);
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;

private:
	struct FloatingBlock {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::Vector3 basePosition = {};
		KamataEngine::Vector3 angularVelocity = {};
		float bobPhase = 0.0f;
		float bobSpeed = 0.0f;
		float bobAmplitude = 0.0f;
		float collisionRadius = 0.0f;
		float screenX = 0.0f;
		float screenY = 0.0f;
		float screenRadius = 0.0f;
	};

	struct CameraBasis {
		KamataEngine::Vector3 right;
		KamataEngine::Vector3 up;
		KamataEngine::Vector3 forward;
	};

	CameraBasis CalculateCameraBasis(
	    const KamataEngine::Camera& camera) const;
	bool CanPlaceBlock(const FloatingBlock& candidate) const;
	float RandomFloat(float minimum, float maximum);

	static inline constexpr std::size_t kBlockCount = 11;
	static inline constexpr float kMinimumScale = 0.40f;
	static inline constexpr float kMaximumScale = 1.16f;
	static inline constexpr float kMinimumDepth = 15.0f;
	static inline constexpr float kMaximumDepth = 25.0f;
	static inline constexpr float kMinimumWorldGap = 0.24f;
	static inline constexpr float kMinimumScreenGap = 0.018f;
	static inline constexpr float kMapClearance = 0.20f;
	static inline constexpr float kDeltaTime = 1.0f / 60.0f;

	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	std::vector<std::unique_ptr<FloatingBlock>> blocks_;
	std::mt19937 randomEngine_;
};
