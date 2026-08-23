#pragma once

#include "CountdownScene.h"
#include "GameOverScene.h"
#include "GameScene.h"
#include "KamataEngine.h"
#include "TitleScene.h"

#include <array>
#include <cstddef>
#include <memory>

class SceneManager {
public:
	~SceneManager();

	void Initialize();
	void Update();
	void Draw();

	// 将来のプレイヤー死亡処理から呼ぶための入口。
	void TriggerGameOver();

private:
	enum class State {
		Title,
		TitleExit,
		Countdown,
		Gameplay,
		GameOverFade,
		GameOverDisplay,
		ReturnBlackFade,
		ReturnTitleReveal,
	};
	static inline constexpr std::size_t kDimGradientSliceCount = 96;

	void ResetGameScene(bool obstacleGenerationEnabled);
	void EnsureDimCanvas();
	void DestroyDimCanvas();
	void EnsureBlackCanvas();
	void DrawDimCanvas(float opacity) const;
	void DrawCanvas(KamataEngine::Sprite* sprite, float alpha) const;
	void BeginReturnToTitle();

	std::unique_ptr<GameScene> gameScene_;
	TitleScene titleScene_;
	CountdownScene countdownScene_;
	GameOverScene gameOverScene_;
	KamataEngine::Camera uiCamera_;
	std::array<KamataEngine::Sprite*, kDimGradientSliceCount>
	    dimCanvasSlices_ = {};
	KamataEngine::Sprite* blackCanvas_ = nullptr;
	uint32_t whiteTextureHandle_ = 0;
	State state_ = State::Title;
	float transitionTime_ = 0.0f;
	float dimCanvasAlpha_ = 1.0f;
	float blackCanvasAlpha_ = 0.0f;
	bool gameplayStartedDuringCountdown_ = false;

	static inline constexpr float kDeltaTime = 1.0f / 60.0f;
	static inline constexpr float kDimOpacity = 1.0f;
	static inline constexpr float kDimTopAlpha = 1.0f;
	static inline constexpr float kDimBottomAlpha = 0.3f;
	static inline constexpr float kGameOverFadeDuration = 0.75f;
	static inline constexpr float kReturnBlackFadeDuration = 0.75f;
	static inline constexpr float kReturnRevealDuration = 0.75f;
};
