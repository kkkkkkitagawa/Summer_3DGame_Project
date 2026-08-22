#pragma once

#include "KamataEngine.h"

class GameOverScene {
public:
	~GameOverScene();

	void Initialize();
	void Start();
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;
	bool IsReadyForInput() const { return phase_ == Phase::Waiting; }

private:
	enum class Phase {
		Hidden,
		Entering,
		Waiting,
	};

	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	KamataEngine::WorldTransform worldTransform_;
	Phase phase_ = Phase::Hidden;
	float animationTime_ = 0.0f;

	static inline constexpr float kDeltaTime = 1.0f / 60.0f;
	static inline constexpr float kEnterDuration = 0.75f;
	static inline constexpr float kOffscreenY = -5.2f;
	static inline constexpr float kDepthAmplitude = 0.45f;
	static inline constexpr float kDepthCycle = 2.2f;
};
