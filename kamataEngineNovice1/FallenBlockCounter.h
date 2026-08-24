#pragma once

#include "KamataEngine.h"

#include <array>
#include <cstddef>
#include <cstdint>

class FallenBlockCounter {
public:
	~FallenBlockCounter();

	void Initialize();
	void SetValue(std::size_t value);
	void Draw(const KamataEngine::Camera& camera) const;

private:
	static inline constexpr std::size_t kDigitModelCount = 10;
	static inline constexpr std::size_t kMaxDisplayDigits = 20;
	static inline constexpr float kModelScale = 0.5f;
	static inline constexpr float kDigitSpacing = 0.32f;
	static inline constexpr float kTopPositionY = 3.35f;

	std::array<KamataEngine::Model*, kDigitModelCount> digitModels_ = {};
	std::array<KamataEngine::WorldTransform, kMaxDisplayDigits>
	    digitTransforms_ = {};
	std::array<std::size_t, kMaxDisplayDigits> displayedDigits_ = {};
	uint32_t textureHandle_ = 0;
	std::size_t visibleDigitCount_ = 0;
};
