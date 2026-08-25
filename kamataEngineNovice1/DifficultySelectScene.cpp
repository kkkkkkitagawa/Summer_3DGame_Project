#include "DifficultySelectScene.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
float SmoothStep(float value) {
	const float clampedValue = std::clamp(value, 0.0f, 1.0f);
	return clampedValue * clampedValue *
	       (3.0f - 2.0f * clampedValue);
}

float EaseOutCubic(float value) {
	const float inverse = 1.0f - value;
	return 1.0f - inverse * inverse * inverse;
}
} // namespace

DifficultySelectScene::~DifficultySelectScene() {
	for (Model* model : models_) {
		delete model;
	}
}

void DifficultySelectScene::Initialize() {
	constexpr std::array<const char*, 3> modelNames = {
	    "Easy", "Normal", "Hard"};
	constexpr std::array<const char*, 3> texturePaths = {
	    "Easy/Easy.png", "Normal/Normal.png", "Hard/Hard.png"};
	for (std::size_t index = 0; index < models_.size(); ++index) {
		models_[index] = Model::CreateFromOBJ(modelNames[index], false);
		assert(models_[index]);
		textureHandles_[index] = TextureManager::Load(texturePaths[index]);
		worldTransforms_[index].Initialize();
		worldTransforms_[index].rotation_.x =
		    std::numbers::pi_v<float> * 0.5f;
		outlineWorldTransforms_[index].Initialize();
		outlineWorldTransforms_[index].rotation_.x =
		    std::numbers::pi_v<float> * 0.5f;
		colors_[index].Initialize();
	}
	outlineColor_.Initialize();
	outlineColor_.SetColor({0.0f, 0.0f, 0.0f, 1.0f});
	Reset(LevelDifficulty::Easy);
}

void DifficultySelectScene::Reset(
    LevelDifficulty maximumUnlockedDifficulty) {
	selectedIndex_ = 0;
	maximumUnlockedIndex_ = ToIndex(maximumUnlockedDifficulty);
	phase_ = Phase::Entering;
	animationTime_ = 0.0f;
	positionY_ = kOffscreenY;
	for (std::size_t index = 0; index < currentScaleRatios_.size(); ++index) {
		currentScaleRatios_[index] = GetTargetScaleRatio(index);
	}
	UpdateTransforms(positionY_);
}

void DifficultySelectScene::SetMaximumUnlockedDifficulty(
    LevelDifficulty maximumUnlockedDifficulty) {
	maximumUnlockedIndex_ = ToIndex(maximumUnlockedDifficulty);
	selectedIndex_ = (std::min)(selectedIndex_, maximumUnlockedIndex_);
}

void DifficultySelectScene::SelectPrevious() {
	if (phase_ != Phase::Selecting || selectedIndex_ == 0) {
		return;
	}
	--selectedIndex_;
}

void DifficultySelectScene::SelectNext() {
	if (phase_ != Phase::Selecting ||
	    selectedIndex_ >= maximumUnlockedIndex_) {
		return;
	}
	++selectedIndex_;
}

void DifficultySelectScene::Confirm() {
	if (phase_ != Phase::Selecting) {
		return;
	}
	phase_ = Phase::Confirming;
	animationTime_ = 0.0f;
}

void DifficultySelectScene::Update() {
	animationTime_ += kDeltaTime;
	if (phase_ == Phase::Entering) {
		const float progress = std::clamp(
		    animationTime_ / kEnterDuration, 0.0f, 1.0f);
		positionY_ = kOffscreenY +
		             (kBaseY - kOffscreenY) * EaseOutCubic(progress);
		if (progress >= 1.0f) {
			positionY_ = kBaseY;
			phase_ = Phase::Selecting;
			animationTime_ = 0.0f;
		}
	} else if (phase_ == Phase::Selecting) {
		const float transitionAmount =
		    std::clamp(kScaleTransitionSpeed * kDeltaTime, 0.0f, 1.0f);
		for (std::size_t index = 0; index < currentScaleRatios_.size(); ++index) {
			const float targetScale = GetTargetScaleRatio(index);
			currentScaleRatios_[index] +=
			    (targetScale - currentScaleRatios_[index]) * transitionAmount;
		}
	} else if (phase_ == Phase::Confirming) {
		const float totalDuration =
		    kConfirmShrinkDuration + kConfirmReturnDuration;
		if (animationTime_ < kConfirmShrinkDuration) {
			const float progress = SmoothStep(
			    animationTime_ / kConfirmShrinkDuration);
			currentScaleRatios_[selectedIndex_] =
			    kSelectedScaleRatio +
			    (kConfirmShrinkScaleRatio - kSelectedScaleRatio) * progress;
		} else {
			const float progress = SmoothStep(
			    (animationTime_ - kConfirmShrinkDuration) /
			    kConfirmReturnDuration);
			currentScaleRatios_[selectedIndex_] =
			    kConfirmShrinkScaleRatio +
			    (kSelectedScaleRatio - kConfirmShrinkScaleRatio) * progress;
		}
		if (animationTime_ >= totalDuration) {
			currentScaleRatios_[selectedIndex_] = kSelectedScaleRatio;
			phase_ = Phase::Finished;
		}
	}

	UpdateTransforms(positionY_);
}

void DifficultySelectScene::Draw(const Camera& camera) const {
	// 选中项单独使用反转外壳描边，未选中项不描边。
	Model::PreDraw(
	    Model::CullingMode::kFront, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kOff);
	models_[selectedIndex_]->Draw(
	    outlineWorldTransforms_[selectedIndex_], camera,
	    &outlineColor_);
	Model::PostDraw();

	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kOff);
	for (std::size_t index = 0; index < models_.size(); ++index) {
		models_[index]->Draw(
		    worldTransforms_[index], camera, textureHandles_[index],
		    &colors_[index]);
	}
	Model::PostDraw();
}

LevelDifficulty DifficultySelectScene::GetSelectedDifficulty() const {
	return ToDifficulty(selectedIndex_);
}

void DifficultySelectScene::UpdateTransforms(float positionY) {
	for (std::size_t index = 0; index < worldTransforms_.size(); ++index) {
		const float scale = kBaseModelScale * currentScaleRatios_[index];
		WorldTransform& transform = worldTransforms_[index];
		// Blender文字の鏡像を補正し、画面へ正対させる。
		transform.scale_ = {-scale, scale, scale};
		transform.translation_ = {kPositionX[index], positionY, 0.0f};
		WorldTransformUpdate(transform);
		WorldTransform& outlineTransform = outlineWorldTransforms_[index];
		outlineTransform.scale_ = {
		    -(scale + kOutlineExpansion),
		    scale + kOutlineExpansion,
		    scale + kOutlineExpansion,
		};
		outlineTransform.translation_ = transform.translation_;
		WorldTransformUpdate(outlineTransform);

		if (index <= maximumUnlockedIndex_) {
			colors_[index].SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		} else {
			colors_[index].SetColor({
			    kLockedBrightness,
			    kLockedBrightness,
			    kLockedBrightness,
			    kLockedOpacity,
			});
		}
	}
}

float DifficultySelectScene::GetTargetScaleRatio(std::size_t index) const {
	return index == selectedIndex_
	           ? kSelectedScaleRatio
	           : kUnselectedScaleRatio;
}

std::size_t DifficultySelectScene::ToIndex(LevelDifficulty difficulty) {
	return static_cast<std::size_t>(difficulty);
}

LevelDifficulty DifficultySelectScene::ToDifficulty(std::size_t index) {
	assert(index <= static_cast<std::size_t>(LevelDifficulty::Hard));
	return static_cast<LevelDifficulty>(index);
}
