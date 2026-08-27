#pragma once

#include "AmbientParticleSystem.h"
#include "CountdownScene.h"
#include "DifficultySelectScene.h"
#include "FallenBlockCounter.h"
#include "GameOverScene.h"
#include "GameClearScene.h"
#include "GameScene.h"
#include "KamataEngine.h"
#include "TitleScene.h"
#include "TitleControlGuide.h"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>

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
		DifficultySelect,
		DifficultySelectBlackFade,
		CountdownReveal,
		Countdown,
		Gameplay,
		GameClearDisplay,
		ClearWhiteFade,
		ClearReturnReveal,
		GameOverFade,
		GameOverDisplay,
		ReturnBlackFade,
		ReturnTitleReveal,
	};
	static inline constexpr std::size_t kDimGradientSliceCount = 96;

	void ResetGameScene(bool obstaclesEnabled);
	void LoadAudioResources();
	void PlayBgm(uint32_t soundHandle, bool loop);
	void PlaySfx(uint32_t soundHandle);
	void StopCurrentBgm();
	void UpdateClearPhaseBgm();
	void EnsureDimCanvas();
	void DestroyDimCanvas();
	void EnsureBlackCanvas();
	void EnsureWhiteCanvas();
	void DrawDimCanvas(float opacity) const;
	void DrawCanvas(KamataEngine::Sprite* sprite, float alpha) const;
	void DrawWhiteCanvas(float alpha) const;
	void BeginReturnToTitle();
	void ApplyDifficultyAndReturnToTitle(LevelDifficulty difficulty);
	void UnlockNextDifficultyAfterClear();

	std::unique_ptr<GameScene> gameScene_;
	TitleScene titleScene_;
	TitleControlGuide titleControlGuide_;
	CountdownScene countdownScene_;
	DifficultySelectScene difficultySelectScene_;
	GameOverScene gameOverScene_;
	GameClearScene gameClearScene_;
	FallenBlockCounter fallenBlockCounter_;
	AmbientParticleSystem ambientParticleSystem_;
	KamataEngine::Camera uiCamera_;
	std::array<KamataEngine::Sprite*, kDimGradientSliceCount>
	    dimCanvasSlices_ = {};
	KamataEngine::Sprite* blackCanvas_ = nullptr;
	KamataEngine::Sprite* whiteCanvas_ = nullptr;
	uint32_t whiteTextureHandle_ = 0;

	// BGM sound handles
	uint32_t clearPhaseBgmSoundHandle_ = 0;
	uint32_t gameSceneBgmSoundHandle_ = 0;
	uint32_t funnyBgmSoundHandle_ = 0;
	uint32_t titleSceneBgmSoundHandle_ = 0;
	uint32_t victoryScreenBgmSoundHandle_ = 0;
	std::optional<uint32_t> currentBgmVoiceHandle_;
	float clearPhaseBgmTimer_ = 0.0f;
	bool isClearPhaseBgmStarted_ = false;

	// SFX sound handles
	uint32_t countdownSfxSoundHandle_ = 0;
	uint32_t debugModeSfxSoundHandle_ = 0;
	uint32_t difficultyChangeSfxSoundHandle_ = 0;
	uint32_t failSoundHandle_ = 0;
	uint32_t gameOverSoundHandle_ = 0;
	uint32_t jumpSfxSoundHandle_ = 0;
	uint32_t returnToTitleSpaceSfxSoundHandle_ = 0;
	uint32_t slimeSfxSoundHandle_ = 0;
	uint32_t fireworksSfxSoundHandle_ = 0;
	uint32_t spaceDodgeSfxSoundHandle_ = 0;
	uint32_t stairJumpSfxSoundHandle_ = 0;
	uint32_t titleSpaceSfxSoundHandle_ = 0;

	State state_ = State::Title;
	float transitionTime_ = 0.0f;
	float dimCanvasAlpha_ = 1.0f;
	float blackCanvasAlpha_ = 0.0f;
	float whiteCanvasAlpha_ = 0.0f;
	bool gameplayStartedDuringCountdown_ = false;
	std::array<bool, 3> clearedDifficulties_ = {};
	bool allDifficultiesCleared_ = false;
	LevelDifficulty selectedDifficulty_ = LevelDifficulty::Easy;
	LevelDifficulty maximumUnlockedDifficulty_ = LevelDifficulty::Easy;

	static inline constexpr float kDeltaTime = 1.0f / 60.0f;
	static inline constexpr float kBgmVolume = 0.3f;
	// Fireworks begin 2.4 seconds after the clear sequence starts.
	static inline constexpr float kClearPhaseBgmStartDelay = 1.4f;
	static inline constexpr float kDimOpacity = 1.0f;
	static inline constexpr float kDimTopAlpha = 0.3f;
	static inline constexpr float kDimBottomAlpha = 1.0f;
	static inline constexpr float kGameOverFadeDuration = 0.75f;
	static inline constexpr float kDifficultyBlackFadeDuration = 0.40f;
	static inline constexpr float kCountdownRevealDuration = 0.35f;
	static inline constexpr float kReturnBlackFadeDuration = 0.75f;
	static inline constexpr float kReturnRevealDuration = 0.75f;
	static inline constexpr float kClearWhiteFadeDuration = 0.35f;
	static inline constexpr float kClearReturnRevealDuration = 0.45f;
};
