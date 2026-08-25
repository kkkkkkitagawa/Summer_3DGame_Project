#pragma once

#include "KamataEngine.h"
#include "LevelGenerator.h"

#include <array>
#include <cstddef>

class DifficultySelectScene {
public:
	~DifficultySelectScene();

	void Initialize();
	void Reset(LevelDifficulty maximumUnlockedDifficulty);
	void SetMaximumUnlockedDifficulty(
	    LevelDifficulty maximumUnlockedDifficulty);
	void SelectPrevious();
	void SelectNext();
	void Confirm();
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;

	LevelDifficulty GetSelectedDifficulty() const;
	bool IsReadyForInput() const { return phase_ == Phase::Selecting; }
	bool IsConfirmationFinished() const { return phase_ == Phase::Finished; }

private:
	enum class Phase {
		Entering,
		Selecting,
		Confirming,
		Finished,
	};

	void UpdateTransforms(float positionY);
	float GetTargetScaleRatio(std::size_t index) const;
	static std::size_t ToIndex(LevelDifficulty difficulty);
	static LevelDifficulty ToDifficulty(std::size_t index);

	std::array<KamataEngine::Model*, 3> models_ = {};
	std::array<uint32_t, 3> textureHandles_ = {};
	std::array<KamataEngine::WorldTransform, 3> worldTransforms_ = {};
	std::array<KamataEngine::WorldTransform, 3> outlineWorldTransforms_ = {};
	std::array<KamataEngine::ObjectColor, 3> colors_ = {};
	KamataEngine::ObjectColor outlineColor_;
	std::array<float, 3> currentScaleRatios_ = {};
	std::size_t selectedIndex_ = 0;
	std::size_t maximumUnlockedIndex_ = 0;
	Phase phase_ = Phase::Entering;
	float animationTime_ = 0.0f;
	float positionY_ = 0.0f;

	static inline constexpr float kDeltaTime = 1.0f / 60.0f;
	static inline constexpr float kOffscreenY = 5.2f;
	static inline constexpr float kBaseY = 0.0f;
	static inline constexpr float kEnterDuration = 0.5f;
	static inline constexpr float kConfirmShrinkDuration = 0.12f;
	static inline constexpr float kConfirmReturnDuration = 0.20f;
	static inline constexpr float kBaseModelScale = 1.35f;
	static inline constexpr float kSelectedScaleRatio = 1.2f;
	static inline constexpr float kUnselectedScaleRatio = 0.9f;
	static inline constexpr float kConfirmShrinkScaleRatio = 1.0f;
	static inline constexpr float kScaleTransitionSpeed = 10.0f;
	static inline constexpr float kLockedBrightness = 0.32f;
	static inline constexpr float kLockedOpacity = 0.65f;
	static inline constexpr float kOutlineExpansion = 0.025f;
	static inline constexpr std::array<float, 3> kPositionX = {
	    -3.4f,
	    0.0f,
	    3.4f,
	};
};
