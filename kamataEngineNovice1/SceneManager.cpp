#include "SceneManager.h"

#include <algorithm>
#include <cassert>

using namespace KamataEngine;

SceneManager::~SceneManager() {
	delete dimCanvas_;
	delete blackCanvas_;
}

void SceneManager::Initialize() {
	uiCamera_.farZ = 100.0f;
	uiCamera_.Initialize();
	uiCamera_.translation_ = {0.0f, 0.0f, -10.0f};
	uiCamera_.UpdateMatrix();

	titleScene_.Initialize();
	countdownScene_.Initialize();
	gameOverScene_.Initialize();

	whiteTextureHandle_ = TextureManager::Load("white1x1.png");
	EnsureDimCanvas();

	ResetGameScene(false);
	titleScene_.Reset();
	state_ = State::Title;
	dimCanvasAlpha_ = kDimAlpha;
	blackCanvasAlpha_ = 0.0f;
}

void SceneManager::Update() {
	Input* input = Input::GetInstance();
	switch (state_) {
	case State::Title:
		gameScene_->Update(false);
		titleScene_.Update();
		if (input->TriggerKey(DIK_SPACE)) {
			titleScene_.StartExit();
			state_ = State::TitleExit;
		}
		break;
	case State::TitleExit:
		gameScene_->Update(false);
		titleScene_.Update();
		if (titleScene_.IsExitFinished()) {
			countdownScene_.Reset();
			gameplayStartedDuringCountdown_ = false;
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
			dimCanvasAlpha_ = kDimAlpha *
			                  (1.0f - countdownScene_.GetFinalExitProgress());
		} else {
			gameScene_->Update(false);
		}

		if (countdownScene_.IsFinished()) {
			dimCanvasAlpha_ = 0.0f;
			delete dimCanvas_;
			dimCanvas_ = nullptr;
			state_ = State::Gameplay;
		}
		break;
	case State::Gameplay:
		gameScene_->Update(true);
		// 仮の死亡入口。正式な死亡処理を追加した後はTriggerGameOverを呼ぶ。
		if (gameScene_->IsDebugMode() && input->TriggerKey(DIK_SPACE)) {
			TriggerGameOver();
		}
		break;
	case State::GameOverFade:
		transitionTime_ += kDeltaTime;
		dimCanvasAlpha_ = kDimAlpha * std::clamp(
		    transitionTime_ / kGameOverFadeDuration, 0.0f, 1.0f);
		if (transitionTime_ >= kGameOverFadeDuration) {
			dimCanvasAlpha_ = kDimAlpha;
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
			dimCanvasAlpha_ = kDimAlpha;
			transitionTime_ = 0.0f;
			state_ = State::ReturnTitleReveal;
		}
		break;
	case State::ReturnTitleReveal:
		gameScene_->Update(false);
		titleScene_.Update();
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
}

void SceneManager::Draw() {
	gameScene_->Draw();
	DrawCanvas(dimCanvas_, dimCanvasAlpha_);

	switch (state_) {
	case State::Title:
	case State::TitleExit:
	case State::ReturnTitleReveal:
		titleScene_.Draw(uiCamera_);
		break;
	case State::Countdown:
		countdownScene_.Draw(uiCamera_);
		break;
	case State::GameOverDisplay:
	case State::ReturnBlackFade:
		gameOverScene_.Draw(uiCamera_);
		break;
	case State::Gameplay:
	case State::GameOverFade:
		break;
	}

	DrawCanvas(blackCanvas_, blackCanvasAlpha_);
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
	gameScene_ = std::make_unique<GameScene>();
	gameScene_->Initialize(obstacleGenerationEnabled);
}

void SceneManager::EnsureDimCanvas() {
	if (dimCanvas_) {
		return;
	}
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	dimCanvas_ = Sprite::Create(whiteTextureHandle_, {0.0f, 0.0f});
	assert(dimCanvas_);
	dimCanvas_->SetSize({
	    static_cast<float>(dxCommon->GetBackBufferWidth()),
	    static_cast<float>(dxCommon->GetBackBufferHeight()),
	});
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

void SceneManager::DrawCanvas(Sprite* sprite, float alpha) const {
	if (!sprite || alpha <= 0.0f) {
		return;
	}
	sprite->SetColor({0.0f, 0.0f, 0.0f, std::clamp(alpha, 0.0f, 1.0f)});
	Sprite::PreDraw();
	sprite->Draw();
	Sprite::PostDraw();
}

void SceneManager::BeginReturnToTitle() {
	transitionTime_ = 0.0f;
	EnsureBlackCanvas();
	blackCanvasAlpha_ = 0.0f;
	state_ = State::ReturnBlackFade;
}
