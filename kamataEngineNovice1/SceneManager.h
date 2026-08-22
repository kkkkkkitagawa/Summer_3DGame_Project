#pragma once

#include "CountdownScene.h"
#include "GameOverScene.h"
#include "GameScene.h"
#include "KamataEngine.h"
#include "TitleScene.h"

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

	void ResetGameScene(bool obstacleGenerationEnabled);
	void EnsureDimCanvas();
	void EnsureBlackCanvas();
	void DrawCanvas(KamataEngine::Sprite* sprite, float alpha) const;
	void BeginReturnToTitle();

	std::unique_ptr<GameScene> gameScene_;
	TitleScene titleScene_;
	CountdownScene countdownScene_;
	GameOverScene gameOverScene_;
	KamataEngine::Camera uiCamera_;
	KamataEngine::Sprite* dimCanvas_ = nullptr;
	KamataEngine::Sprite* blackCanvas_ = nullptr;
	uint32_t whiteTextureHandle_ = 0;
	State state_ = State::Title;
	float transitionTime_ = 0.0f;
	float dimCanvasAlpha_ = 0.5f;
	float blackCanvasAlpha_ = 0.0f;
	bool gameplayStartedDuringCountdown_ = false;

	static inline constexpr float kDeltaTime = 1.0f / 60.0f;
	static inline constexpr float kDimAlpha = 0.5f;
	static inline constexpr float kGameOverFadeDuration = 0.75f;
	static inline constexpr float kReturnBlackFadeDuration = 0.75f;
	static inline constexpr float kReturnRevealDuration = 0.75f;
};
