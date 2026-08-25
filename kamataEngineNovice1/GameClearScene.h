#pragma once

#include "KamataEngine.h"

class GameClearScene {
public:
	~GameClearScene();

	void Initialize();
	void Start();
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;
	bool IsReadyForInput() const { return phase_ == Phase::Waiting; }
	float GetCanvasOpacity() const { return canvasOpacity_; }

private:
	enum class Phase {
		Hidden,
		Entering,
		Waiting,
	};

	void UpdateTransforms(float titleScaleRatio);
	void DrawPlayer(const KamataEngine::Camera& camera) const;
	void DrawTitle(const KamataEngine::Camera& camera) const;

	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::Model* titleModel_ = nullptr;
	uint32_t playerTextureHandle_ = 0;
	uint32_t titleTextureHandle_ = 0;
	KamataEngine::WorldTransform playerWorldTransform_;
	KamataEngine::WorldTransform playerOutlineWorldTransform_;
	KamataEngine::WorldTransform titleWorldTransform_;
	KamataEngine::WorldTransform titleOutlineWorldTransform_;
	KamataEngine::ObjectColor outlineColor_;
	Phase phase_ = Phase::Hidden;
	float animationTime_ = 0.0f;
	float canvasOpacity_ = 0.0f;
	float playerRotation_ = 0.0f;

	static inline constexpr float kDeltaTime = 1.0f / 60.0f;
	static inline constexpr float kCanvasFadeDuration = 0.30f;
	static inline constexpr float kPlayerDropDuration = 0.85f;
	static inline constexpr float kTitleEnterDuration = 0.72f;
	static inline constexpr float kCanvasTargetOpacity = 0.70f;
	static inline constexpr float kPlayerBaseScale = 0.75f;
	static inline constexpr float kPlayerOutlineExpansion = 2.0f / 32.0f;
	static inline constexpr float kPlayerStartY = 5.5f;
	static inline constexpr float kPlayerTargetX = 2.25f;
	static inline constexpr float kPlayerTargetY = -0.10f;
	static inline constexpr float kTitleBaseScale = 2.2f;
	static inline constexpr float kTitleOutlineExpansion = 0.6f / 32.0f;
	static inline constexpr float kTitleBounceAmplitude = 0.08f;
};
