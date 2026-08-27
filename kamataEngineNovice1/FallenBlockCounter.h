#pragma once

#include "KamataEngine.h"

#include <array>
#include <cstddef>
#include <cstdint>

class FallenBlockCounter {
public:
	~FallenBlockCounter();

	void Initialize();
	void Update(std::size_t value);
	void Draw(const KamataEngine::Camera& camera) const;

private:
	enum class AnimationPhase {
		Stable,
		Shrinking,
		Growing,
	};

	void SetDisplayedValue(std::size_t value);
	void ApplyAnimationScale(float scaleRatio);

	static inline constexpr std::size_t kDigitModelCount = 10;
	static inline constexpr std::size_t kMaxDisplayDigits = 20;
	static inline constexpr float kModelScale = 0.7f;
	static inline constexpr float kOutlineExpansion = 0.5f / 32.0f;
	static inline constexpr float kDigitSpacing = 0.32f;
	static inline constexpr float kTopPositionY = 3.35f;
	static inline constexpr float kShrinkDuration = 0.08f;
	static inline constexpr float kGrowDuration = 0.10f;
	static inline constexpr float kDeltaTime = 1.0f / 60.0f;

	std::array<KamataEngine::Model*, kDigitModelCount> digitModels_ = {};
	std::array<KamataEngine::WorldTransform, kMaxDisplayDigits>
	    digitTransforms_ = {};
	std::array<KamataEngine::WorldTransform, kMaxDisplayDigits>
	    digitOutlineTransforms_ = {};
	std::array<std::size_t, kMaxDisplayDigits> displayedDigits_ = {};
	KamataEngine::ObjectColor outlineColor_;
	uint32_t textureHandle_ = 0;
	std::size_t visibleDigitCount_ = 0;
	std::size_t displayedValue_ = 0;
	std::size_t pendingValue_ = 0;
	AnimationPhase animationPhase_ = AnimationPhase::Stable;
	float animationTime_ = 0.0f;
};
