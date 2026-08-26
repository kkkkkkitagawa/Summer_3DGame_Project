#include "SceneManager.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace KamataEngine;

SceneManager::~SceneManager() {
	DestroyDimCanvas();
	delete blackCanvas_;
	delete whiteCanvas_;
}

void SceneManager::Initialize() {
	uiCamera_.farZ = 100.0f;
	uiCamera_.Initialize();
	uiCamera_.translation_ = {0.0f, 0.0f, -10.0f};
	uiCamera_.UpdateMatrix();

	titleScene_.Initialize();
	titleControlGuide_.Initialize();
	difficultySelectScene_.Initialize();
	countdownScene_.Initialize();
	gameOverScene_.Initialize();
	gameClearScene_.Initialize();
	fallenBlockCounter_.Initialize();

	whiteTextureHandle_ = TextureManager::Load("white1x1.png");
	EnsureDimCanvas();

	ResetGameScene(false);
	ambientParticleSystem_.Initialize();
	titleScene_.Reset();
	titleControlGuide_.Reset();
	state_ = State::Title;
	dimCanvasAlpha_ = kDimOpacity;
	blackCanvasAlpha_ = 0.0f;
	whiteCanvasAlpha_ = 0.0f;
}

void SceneManager::Update() {
	Input* input = Input::GetInstance();
	switch (state_) {
	case State::Title:
		gameScene_->Update(false);
		titleScene_.Update();
		titleControlGuide_.Update();
		if (input->TriggerKey(DIK_SPACE)) {
			titleScene_.StartExit();
			titleControlGuide_.StartExit();
			state_ = State::TitleExit;
		}
		break;
	case State::TitleExit:
		gameScene_->Update(false);
		titleScene_.Update();
		titleControlGuide_.Update();
		if (titleScene_.IsExitFinished()) {
			difficultySelectScene_.Reset(
			    gameScene_->IsDebugMode()
			        ? LevelDifficulty::Hard
			        : maximumUnlockedDifficulty_);
			if (allDifficultiesCleared_) {
				gameScene_->StartDifficultySelectCelebration();
			}
			state_ = State::DifficultySelect;
		}
		break;
	case State::DifficultySelect:
		gameScene_->Update(false);
		difficultySelectScene_.SetMaximumUnlockedDifficulty(
		    gameScene_->IsDebugMode()
		        ? LevelDifficulty::Hard
		        : maximumUnlockedDifficulty_);
		difficultySelectScene_.Update();
		if (difficultySelectScene_.IsReadyForInput()) {
			if (input->TriggerKey(DIK_A)) {
				difficultySelectScene_.SelectPrevious();
			}
			if (input->TriggerKey(DIK_D)) {
				difficultySelectScene_.SelectNext();
			}
			if (input->TriggerKey(DIK_SPACE)) {
				difficultySelectScene_.Confirm();
			}
		}
		if (difficultySelectScene_.IsConfirmationFinished()) {
			gameScene_->StopDifficultySelectCelebration();
			selectedDifficulty_ =
			    difficultySelectScene_.GetSelectedDifficulty();
			transitionTime_ = 0.0f;
			EnsureBlackCanvas();
			blackCanvasAlpha_ = 0.0f;
			state_ = State::DifficultySelectBlackFade;
		}
		break;
	case State::DifficultySelectBlackFade:
		gameScene_->Update(false);
		difficultySelectScene_.Update();
		transitionTime_ += kDeltaTime;
		blackCanvasAlpha_ = std::clamp(
		    transitionTime_ / kDifficultyBlackFadeDuration, 0.0f, 1.0f);
		if (transitionTime_ >= kDifficultyBlackFadeDuration) {
			blackCanvasAlpha_ = 1.0f;
			ResetGameScene(false);
			countdownScene_.Reset();
			gameplayStartedDuringCountdown_ = false;
			transitionTime_ = 0.0f;
			state_ = State::CountdownReveal;
		}
		break;
	case State::CountdownReveal:
		gameScene_->Update(false);
		transitionTime_ += kDeltaTime;
		blackCanvasAlpha_ = 1.0f - std::clamp(
		    transitionTime_ / kCountdownRevealDuration, 0.0f, 1.0f);
		if (transitionTime_ >= kCountdownRevealDuration) {
			blackCanvasAlpha_ = 0.0f;
			delete blackCanvas_;
			blackCanvas_ = nullptr;
			state_ = State::Countdown;
		}
		break;
	case State::Countdown:
		countdownScene_.Update();
		if (!gameplayStartedDuringCountdown_ &&
		    countdownScene_.IsFinalExitPhase()) {
			ResetGameScene(true);
			gameplayStartedDuringCountdown_ = true;
		}

		if (gameplayStartedDuringCountdown_) {
			gameScene_->Update(true);
			dimCanvasAlpha_ = kDimOpacity *
			                  (1.0f - countdownScene_.GetFinalExitProgress());
		} else {
			gameScene_->Update(false);
		}

		if (countdownScene_.IsFinished()) {
			dimCanvasAlpha_ = 0.0f;
			DestroyDimCanvas();
			state_ = State::Gameplay;
		}
		break;
	case State::Gameplay:
		gameScene_->Update(true);
		if (gameScene_->IsGameClearReady()) {
			UnlockNextDifficultyAfterClear();
			gameClearScene_.Start();
			EnsureWhiteCanvas();
			whiteCanvasAlpha_ = 0.0f;
			state_ = State::GameClearDisplay;
			break;
		}
		if (gameScene_->IsDeathSequenceReady()) {
			TriggerGameOver();
		}
		break;
	case State::GameClearDisplay:
		gameClearScene_.Update();
		whiteCanvasAlpha_ = gameClearScene_.GetCanvasOpacity();
		if (gameClearScene_.IsReadyForInput() &&
		    input->TriggerKey(DIK_SPACE)) {
			transitionTime_ = 0.0f;
			state_ = State::ClearWhiteFade;
		}
		break;
	case State::ClearWhiteFade:
		gameClearScene_.Update();
		transitionTime_ += kDeltaTime;
		whiteCanvasAlpha_ = 0.70f + 0.30f * std::clamp(
		    transitionTime_ / kClearWhiteFadeDuration, 0.0f, 1.0f);
		if (transitionTime_ >= kClearWhiteFadeDuration) {
			whiteCanvasAlpha_ = 1.0f;
			ResetGameScene(false);
			titleScene_.Reset();
			titleControlGuide_.Reset();
			EnsureDimCanvas();
			dimCanvasAlpha_ = kDimOpacity;
			transitionTime_ = 0.0f;
			state_ = State::ClearReturnReveal;
		}
		break;
	case State::ClearReturnReveal:
		gameScene_->Update(false);
		titleScene_.Update();
		titleControlGuide_.Update();
		transitionTime_ += kDeltaTime;
		whiteCanvasAlpha_ = 1.0f - std::clamp(
		    transitionTime_ / kClearReturnRevealDuration, 0.0f, 1.0f);
		if (transitionTime_ >= kClearReturnRevealDuration) {
			whiteCanvasAlpha_ = 0.0f;
			delete whiteCanvas_;
			whiteCanvas_ = nullptr;
			state_ = State::Title;
		}
		break;
	case State::GameOverFade:
		transitionTime_ += kDeltaTime;
		dimCanvasAlpha_ = kDimOpacity * std::clamp(
		    transitionTime_ / kGameOverFadeDuration, 0.0f, 1.0f);
		if (transitionTime_ >= kGameOverFadeDuration) {
			dimCanvasAlpha_ = kDimOpacity;
			gameOverScene_.Start();
			state_ = State::GameOverDisplay;
		}
		break;
	case State::GameOverDisplay:
		gameOverScene_.Update();
		if (gameOverScene_.IsReadyForInput() &&
		    input->TriggerKey(DIK_SPACE)) {
			BeginReturnToTitle();
		}
		break;
	case State::ReturnBlackFade:
		gameOverScene_.Update();
		transitionTime_ += kDeltaTime;
		blackCanvasAlpha_ = std::clamp(
		    transitionTime_ / kReturnBlackFadeDuration, 0.0f, 1.0f);
		if (transitionTime_ >= kReturnBlackFadeDuration) {
			blackCanvasAlpha_ = 1.0f;
			ResetGameScene(false);
			titleScene_.Reset();
			titleControlGuide_.Reset();
			dimCanvasAlpha_ = kDimOpacity;
			transitionTime_ = 0.0f;
			state_ = State::ReturnTitleReveal;
		}
		break;
	case State::ReturnTitleReveal:
		gameScene_->Update(false);
		titleScene_.Update();
		titleControlGuide_.Update();
		transitionTime_ += kDeltaTime;
		blackCanvasAlpha_ = 1.0f - std::clamp(
		    transitionTime_ / kReturnRevealDuration, 0.0f, 1.0f);
		if (transitionTime_ >= kReturnRevealDuration) {
			blackCanvasAlpha_ = 0.0f;
			delete blackCanvas_;
			blackCanvas_ = nullptr;
			state_ = State::Title;
		}
		break;
	}

	if (gameScene_) {
		gameScene_->UpdateCelebrationEffect();
		ambientParticleSystem_.Update(gameScene_->GetRenderCamera());
		const std::optional<LevelDifficulty> difficultyRequest =
		    gameScene_->ConsumeDifficultyChangeRequest();
		if (difficultyRequest) {
			ApplyDifficultyAndReturnToTitle(*difficultyRequest);
		}
		fallenBlockCounter_.Update(
		    gameScene_->GetFallenMapBlockCount());
	}
}

void SceneManager::Draw() {
	gameScene_->Draw();
	ambientParticleSystem_.Draw(gameScene_->GetRenderCamera());
	DrawDimCanvas(dimCanvasAlpha_);

	switch (state_) {
	case State::Title:
	case State::TitleExit:
	case State::ReturnTitleReveal:
	case State::ClearReturnReveal:
		titleScene_.Draw(uiCamera_);
		titleControlGuide_.Draw(uiCamera_);
		break;
	case State::DifficultySelect:
	case State::DifficultySelectBlackFade:
		difficultySelectScene_.Draw(uiCamera_);
		break;
	case State::CountdownReveal:
	case State::Countdown:
		countdownScene_.Draw(uiCamera_);
		break;
	case State::GameOverDisplay:
	case State::ReturnBlackFade:
		gameOverScene_.Draw(uiCamera_);
		break;
	case State::GameClearDisplay:
		DrawWhiteCanvas(whiteCanvasAlpha_);
		gameClearScene_.Draw(uiCamera_);
		break;
	case State::ClearWhiteFade:
		gameClearScene_.Draw(uiCamera_);
		DrawWhiteCanvas(whiteCanvasAlpha_);
		break;
	case State::Gameplay:
	case State::GameOverFade:
		break;
	}

	const bool showFallenBlockCounter =
	    state_ == State::Gameplay || state_ == State::GameOverFade ||
	    state_ == State::GameOverDisplay || state_ == State::ReturnBlackFade ||
	    (state_ == State::Countdown && gameplayStartedDuringCountdown_);
	if (showFallenBlockCounter) {
		fallenBlockCounter_.Draw(uiCamera_);
	}
	gameScene_->DrawCelebrationEffect();

	DrawCanvas(blackCanvas_, blackCanvasAlpha_);
	if (state_ == State::ClearReturnReveal) {
		DrawWhiteCanvas(whiteCanvasAlpha_);
	}
}

void SceneManager::TriggerGameOver() {
	if (state_ != State::Gameplay) {
		return;
	}
	transitionTime_ = 0.0f;
	EnsureDimCanvas();
	dimCanvasAlpha_ = 0.0f;
	state_ = State::GameOverFade;
}

void SceneManager::ResetGameScene(bool obstacleGenerationEnabled) {
	gameScene_ = std::make_unique<GameScene>(selectedDifficulty_);
	gameScene_->Initialize(obstacleGenerationEnabled);
}

void SceneManager::EnsureDimCanvas() {
	if (dimCanvasSlices_.front()) {
		return;
	}
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	const float screenWidth =
	    static_cast<float>(dxCommon->GetBackBufferWidth());
	const float screenHeight =
	    static_cast<float>(dxCommon->GetBackBufferHeight());
	for (std::size_t index = 0; index < kDimGradientSliceCount; ++index) {
		const float sliceTop = std::round(
		    screenHeight * static_cast<float>(index) /
		    static_cast<float>(kDimGradientSliceCount));
		const float sliceBottom = std::round(
		    screenHeight * static_cast<float>(index + 1) /
		    static_cast<float>(kDimGradientSliceCount));
		dimCanvasSlices_[index] = Sprite::Create(
		    whiteTextureHandle_, {0.0f, sliceTop});
		assert(dimCanvasSlices_[index]);
		dimCanvasSlices_[index]->SetSize(
		    {screenWidth, sliceBottom - sliceTop});
	}
}

void SceneManager::DestroyDimCanvas() {
	for (Sprite*& slice : dimCanvasSlices_) {
		delete slice;
		slice = nullptr;
	}
}

void SceneManager::EnsureBlackCanvas() {
	if (blackCanvas_) {
		return;
	}
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	blackCanvas_ = Sprite::Create(whiteTextureHandle_, {0.0f, 0.0f});
	assert(blackCanvas_);
	blackCanvas_->SetSize({
	    static_cast<float>(dxCommon->GetBackBufferWidth()),
	    static_cast<float>(dxCommon->GetBackBufferHeight()),
	});
}

void SceneManager::EnsureWhiteCanvas() {
	if (whiteCanvas_) {
		return;
	}
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	whiteCanvas_ = Sprite::Create(whiteTextureHandle_, {0.0f, 0.0f});
	assert(whiteCanvas_);
	whiteCanvas_->SetSize({
	    static_cast<float>(dxCommon->GetBackBufferWidth()),
	    static_cast<float>(dxCommon->GetBackBufferHeight()),
	});
}

void SceneManager::DrawCanvas(Sprite* sprite, float alpha) const {
	if (!sprite || alpha <= 0.0f) {
		return;
	}
	sprite->SetColor({0.0f, 0.0f, 0.0f, std::clamp(alpha, 0.0f, 1.0f)});
	Sprite::PreDraw();
	sprite->Draw();
	Sprite::PostDraw();
}

void SceneManager::DrawWhiteCanvas(float alpha) const {
	if (!whiteCanvas_ || alpha <= 0.0f) {
		return;
	}
	whiteCanvas_->SetColor(
	    {1.0f, 1.0f, 1.0f, std::clamp(alpha, 0.0f, 1.0f)});
	Sprite::PreDraw();
	whiteCanvas_->Draw();
	Sprite::PostDraw();
}

void SceneManager::DrawDimCanvas(float opacity) const {
	if (!dimCanvasSlices_.front() || opacity <= 0.0f) {
		return;
	}

	const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);
	Sprite::PreDraw();
	for (std::size_t index = 0; index < kDimGradientSliceCount; ++index) {
		const float gradientPosition =
		    static_cast<float>(index) /
		    static_cast<float>(kDimGradientSliceCount - 1);
		const float gradientAlpha =
		    kDimTopAlpha +
		    (kDimBottomAlpha - kDimTopAlpha) * gradientPosition;
		dimCanvasSlices_[index]->SetColor(
		    {0.0f, 0.0f, 0.0f, gradientAlpha * clampedOpacity});
		dimCanvasSlices_[index]->Draw();
	}
	Sprite::PostDraw();
}

void SceneManager::BeginReturnToTitle() {
	transitionTime_ = 0.0f;
	EnsureBlackCanvas();
	blackCanvasAlpha_ = 0.0f;
	state_ = State::ReturnBlackFade;
}

void SceneManager::ApplyDifficultyAndReturnToTitle(
    LevelDifficulty difficulty) {
	selectedDifficulty_ = difficulty;
	transitionTime_ = 0.0f;
	gameplayStartedDuringCountdown_ = false;

	delete blackCanvas_;
	blackCanvas_ = nullptr;
	blackCanvasAlpha_ = 0.0f;
	delete whiteCanvas_;
	whiteCanvas_ = nullptr;
	whiteCanvasAlpha_ = 0.0f;
	EnsureDimCanvas();
	dimCanvasAlpha_ = kDimOpacity;

	ResetGameScene(false);
	titleScene_.Reset();
	titleControlGuide_.Reset();
	state_ = State::Title;
}

void SceneManager::UnlockNextDifficultyAfterClear() {
	const std::size_t clearedIndex =
	    static_cast<std::size_t>(selectedDifficulty_);
	assert(clearedIndex < clearedDifficulties_.size());
	clearedDifficulties_[clearedIndex] = true;
	allDifficultiesCleared_ = std::all_of(
	    clearedDifficulties_.begin(), clearedDifficulties_.end(),
	    [](bool isCleared) { return isCleared; });

	if (selectedDifficulty_ == LevelDifficulty::Easy &&
	    maximumUnlockedDifficulty_ == LevelDifficulty::Easy) {
		maximumUnlockedDifficulty_ = LevelDifficulty::Normal;
		return;
	}
	if (selectedDifficulty_ == LevelDifficulty::Normal &&
	    maximumUnlockedDifficulty_ == LevelDifficulty::Normal) {
		maximumUnlockedDifficulty_ = LevelDifficulty::Hard;
	}
}
