#include "CountdownScene.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
float EaseOutCubic(float value) {
	const float inverse = 1.0f - value;
	return 1.0f - inverse * inverse * inverse;
}

float EaseInBack(float value) {
	constexpr float overshoot = 1.70158f;
	const float extendedOvershoot = overshoot + 1.0f;
	return extendedOvershoot * value * value * value -
	       overshoot * value * value;
}
} // namespace

CountdownScene::~CountdownScene() {
	for (Model* model : models_) {
		delete model;
	}
}

void CountdownScene::Initialize(uint32_t countdownSfxSoundHandle) {
	countdownSfxSoundHandle_ = countdownSfxSoundHandle;
	// 描画順は3、2、1。
	models_[0] = Model::CreateFromOBJ("three", false);
	models_[1] = Model::CreateFromOBJ("two", false);
	models_[2] = Model::CreateFromOBJ("one", false);
	textureHandles_[0] = TextureManager::Load("number/three/three.png");
	textureHandles_[1] = TextureManager::Load("number/two/two.png");
	textureHandles_[2] = TextureManager::Load("number/one/one.png");
	for (std::size_t index = 0; index < models_.size(); ++index) {
		assert(models_[index]);
		worldTransforms_[index].Initialize();
		worldTransforms_[index].scale_ = {-4.0f, 4.0f, 4.0f};
		worldTransforms_[index].rotation_.x =
		    std::numbers::pi_v<float> * 0.5f;
	}
	Reset();
}

void CountdownScene::Reset() {
	elapsedTime_ = 0.0f;
	lastPlayedNumberIndex_ = models_.size();
	for (WorldTransform& transform : worldTransforms_) {
		transform.translation_ = {0.0f, kOffscreenY, 0.0f};
		WorldTransformUpdate(transform);
	}
	UpdateCurrentNumberTransform();
}

void CountdownScene::Update() {
	elapsedTime_ = (std::min)(elapsedTime_ + kDeltaTime, kTotalDuration);
	if (!IsFinished()) {
		const std::size_t currentNumberIndex = GetCurrentNumberIndex();
		if (currentNumberIndex != lastPlayedNumberIndex_) {
			Audio::GetInstance()->PlayWave(
			    countdownSfxSoundHandle_, false, 1.0f);
			lastPlayedNumberIndex_ = currentNumberIndex;
		}
		UpdateCurrentNumberTransform();
	}
}

void CountdownScene::Draw(const Camera& camera) const {
	if (IsFinished()) {
		return;
	}
	const std::size_t index = GetCurrentNumberIndex();
	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kOff);
	models_[index]->Draw(
	    worldTransforms_[index], camera, textureHandles_[index]);
	Model::PostDraw();
}

bool CountdownScene::IsFinalExitPhase() const {
	const float finalExitStart =
	    kNumberDuration * 2.0f + kEnterDuration + kHoldDuration;
	return elapsedTime_ >= finalExitStart && elapsedTime_ < kTotalDuration;
}

float CountdownScene::GetFinalExitProgress() const {
	const float finalExitStart =
	    kNumberDuration * 2.0f + kEnterDuration + kHoldDuration;
	return std::clamp(
	    (elapsedTime_ - finalExitStart) / kExitDuration, 0.0f, 1.0f);
}

void CountdownScene::UpdateCurrentNumberTransform() {
	const std::size_t index = GetCurrentNumberIndex();
	const float localTime =
	    elapsedTime_ - static_cast<float>(index) * kNumberDuration;
	float positionY = kBaseY;
	if (localTime < kEnterDuration) {
		const float progress =
		    std::clamp(localTime / kEnterDuration, 0.0f, 1.0f);
		positionY = kOffscreenY +
		            (kBaseY - kOffscreenY) * EaseOutCubic(progress);
	} else if (localTime < kEnterDuration + kHoldDuration) {
		positionY = kBaseY;
	} else {
		const float progress = std::clamp(
		    (localTime - kEnterDuration - kHoldDuration) / kExitDuration,
		    0.0f, 1.0f);
		positionY =
		    kBaseY + (kOffscreenY - kBaseY) * EaseInBack(progress);
	}
	worldTransforms_[index].translation_ = {0.0f, positionY, 0.0f};
	WorldTransformUpdate(worldTransforms_[index]);
}

std::size_t CountdownScene::GetCurrentNumberIndex() const {
	const std::size_t index = static_cast<std::size_t>(
	    elapsedTime_ / kNumberDuration);
	return (std::min)(index, models_.size() - 1);
}
