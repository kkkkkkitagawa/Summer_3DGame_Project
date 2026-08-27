#pragma once

#include "KamataEngine.h"

class TitleScene {
public:
	~TitleScene();

	void Initialize();
	void Reset();
	void StartExit();
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;
	bool IsExitFinished() const { return phase_ == Phase::Finished; }

private:
	enum class Phase {
		Idle,
		Exiting,
		Finished,
	};

	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	KamataEngine::WorldTransform worldTransform_;
	Phase phase_ = Phase::Idle;
	float animationTime_ = 0.0f;
	float exitStartY_ = 0.0f;

	static inline constexpr float kDeltaTime = 1.0f / 60.0f;
	// The updated title is wider than the original model, so keep it framed
	// while placing it above the control guide.
	static inline constexpr float kModelScale = 0.90f;
	static inline constexpr float kBaseY = 1.1f;
	static inline constexpr float kOffscreenY = 5.2f;
	static inline constexpr float kFloatingAmplitude = 0.16f;
	static inline constexpr float kFloatingCycle = 2.4f;
	static inline constexpr float kExitDuration = 0.5f;
};
