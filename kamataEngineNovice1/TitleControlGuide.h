#pragma once

#include "KamataEngine.h"

#include <array>
#include <numbers>

class TitleControlGuide {
public:
	~TitleControlGuide();

	void Initialize();
	void Reset();
	void StartExit();
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;

private:
	enum class Phase {
		Visible,
		Fading,
		Hidden,
	};

	void UpdateTransforms();

	std::array<KamataEngine::Model*, 5> models_ = {};
	std::array<uint32_t, 5> textureHandles_ = {};
	std::array<KamataEngine::WorldTransform, 5> worldTransforms_ = {};
	std::array<KamataEngine::ObjectColor, 5> colors_ = {};
	Phase phase_ = Phase::Visible;
	float animationTime_ = 0.0f;
	float opacity_ = 1.0f;

	static inline constexpr std::size_t kKeyAIndex = 0;
	static inline constexpr std::size_t kKeyDIndex = 1;
	static inline constexpr std::size_t kSpaceIndex = 2;
	static inline constexpr std::size_t kArrowLeftIndex = 3;
	static inline constexpr std::size_t kArrowRightIndex = 4;
	static inline constexpr float kDeltaTime = 1.0f / 60.0f;
	static inline constexpr float kFacingRotationX =
	    90.0f * std::numbers::pi_v<float> / 180.0f;
	static inline constexpr float kSpaceBackwardTilt =
	    6.0f * std::numbers::pi_v<float> / 180.0f;
	// UI相机位于Z=-10；左右按键向相机中心内转，避免透视造成外撇。
	static inline constexpr float kKeyFaceCameraYaw =
	    26.565f * std::numbers::pi_v<float> / 180.0f;
	static inline constexpr float kKeyScale = 1.35f;
	static inline constexpr float kSpaceScale = 1.5f;
	static inline constexpr float kArrowScale = 0.80f;
	static inline constexpr float kArrowHorizontalOffset = -0.65f;
	static inline constexpr float kKeyPositionX = 5.0f;
	static inline constexpr float kKeyPositionY = -2.35f;
	static inline constexpr float kArrowPositionY = -1.45f;
	static inline constexpr float kArrowLeftLocalCenterX = -0.6052325f;
	static inline constexpr float kArrowRightLocalCenterX = -0.4443925f;
	static inline constexpr float kArrowLocalCenterZ = -0.17821f;
	static inline constexpr float kSpaceBreathAmplitude = 0.045f;
	static inline constexpr float kSpaceBreathCycle = 1.8f;
	static inline constexpr float kFadeDuration = 0.20f;
};
