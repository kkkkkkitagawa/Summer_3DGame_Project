#pragma once

#include "KamataEngine.h"

#include <array>
#include <cstddef>

class CountdownScene {
public:
	~CountdownScene();

	void Initialize();
	void Reset();
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;
	bool IsFinalExitPhase() const;
	float GetFinalExitProgress() const;
	bool IsFinished() const { return elapsedTime_ >= kTotalDuration; }

private:
	void UpdateCurrentNumberTransform();
	std::size_t GetCurrentNumberIndex() const;

	std::array<KamataEngine::Model*, 3> models_ = {};
	std::array<uint32_t, 3> textureHandles_ = {};
	std::array<KamataEngine::WorldTransform, 3> worldTransforms_ = {};
	float elapsedTime_ = 0.0f;

	static inline constexpr float kDeltaTime = 1.0f / 60.0f;
	static inline constexpr float kNumberDuration = 2.0f;
	static inline constexpr float kEnterRatio = 0.25f;
	static inline constexpr float kHoldRatio = 0.50f;
	static inline constexpr float kExitRatio = 0.25f;
	static inline constexpr float kEnterDuration =
	    kNumberDuration * kEnterRatio;
	static inline constexpr float kHoldDuration =
	    kNumberDuration * kHoldRatio;
	static inline constexpr float kExitDuration =
	    kNumberDuration * kExitRatio;
	static inline constexpr float kTotalDuration = kNumberDuration * 3.0f;
	static inline constexpr float kOffscreenY = 5.2f;
	static inline constexpr float kBaseY = 0.0f;
};
