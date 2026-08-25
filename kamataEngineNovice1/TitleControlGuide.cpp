#include "TitleControlGuide.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace KamataEngine;

TitleControlGuide::~TitleControlGuide() {
	for (Model* model : models_) {
		delete model;
	}
}

void TitleControlGuide::Initialize() {
	constexpr std::array<const char*, 5> modelNames = {
	    "KeyA", "KeyD", "KeySpace", "ArrowL", "ArrowR"};
	constexpr std::array<const char*, 5> texturePaths = {
	    "Key/Key.png", "Key/Key.png", "Key/Key.png",
	    "Arrow/Arrow.png", "Arrow/Arrow.png"};
	for (std::size_t index = 0; index < models_.size(); ++index) {
		models_[index] = Model::CreateFromOBJ(modelNames[index], false);
		assert(models_[index]);
		textureHandles_[index] = TextureManager::Load(texturePaths[index]);
		worldTransforms_[index].Initialize();
		colors_[index].Initialize();
	}
	Reset();
}

void TitleControlGuide::Reset() {
	phase_ = Phase::Visible;
	animationTime_ = 0.0f;
	opacity_ = 1.0f;
	UpdateTransforms();
}

void TitleControlGuide::StartExit() {
	if (phase_ != Phase::Visible) {
		return;
	}
	phase_ = Phase::Fading;
	animationTime_ = 0.0f;
}

void TitleControlGuide::Update() {
	animationTime_ += kDeltaTime;
	if (phase_ == Phase::Fading) {
		const float progress = std::clamp(
		    animationTime_ / kFadeDuration, 0.0f, 1.0f);
		opacity_ = 1.0f - progress * progress;
		if (progress >= 1.0f) {
			opacity_ = 0.0f;
			phase_ = Phase::Hidden;
		}
	}
	UpdateTransforms();
}

void TitleControlGuide::Draw(const Camera& camera) const {
	if (phase_ == Phase::Hidden || opacity_ <= 0.0f) {
		return;
	}
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

void TitleControlGuide::UpdateTransforms() {
	const float breath = 1.0f + kSpaceBreathAmplitude * std::sin(
	    animationTime_ * 2.0f * std::numbers::pi_v<float> /
	    kSpaceBreathCycle);

	WorldTransform& keyA = worldTransforms_[kKeyAIndex];
	keyA.scale_ = {-kKeyScale, kKeyScale, -kKeyScale};
	keyA.rotation_ = {
	    -kFacingRotationX, -kKeyFaceCameraYaw, 0.0f};
	keyA.translation_ = {-kKeyPositionX, kKeyPositionY, 0.0f};

	WorldTransform& keyD = worldTransforms_[kKeyDIndex];
	keyD.scale_ = {-kKeyScale, kKeyScale, -kKeyScale};
	keyD.rotation_ = {
	    -kFacingRotationX, kKeyFaceCameraYaw, 0.0f};
	keyD.translation_ = {kKeyPositionX, kKeyPositionY, 0.0f};

	const float animatedSpaceScale = kSpaceScale * breath;
	WorldTransform& space = worldTransforms_[kSpaceIndex];
	space.scale_ = {
	    -animatedSpaceScale, animatedSpaceScale, animatedSpaceScale};
	space.rotation_ = {
	    kFacingRotationX + kSpaceBackwardTilt, 0.0f, 0.0f};
	space.translation_ = {0.0f, kKeyPositionY, 0.0f};

	WorldTransform& arrowLeft = worldTransforms_[kArrowLeftIndex];
	arrowLeft.scale_ = {kArrowScale, kArrowScale, kArrowScale};
	arrowLeft.rotation_ = {kFacingRotationX, 0.0f, 0.0f};
	arrowLeft.translation_ = {
	    kKeyPositionX + kArrowHorizontalOffset -
	        kArrowLeftLocalCenterX * kArrowScale,
	    kArrowPositionY - kArrowLocalCenterZ * kArrowScale,
	    0.0f,
	};

	WorldTransform& arrowRight = worldTransforms_[kArrowRightIndex];
	arrowRight.scale_ = {kArrowScale, kArrowScale, kArrowScale};
	arrowRight.rotation_ = {kFacingRotationX, 0.0f, 0.0f};
	arrowRight.translation_ = {
	    -kKeyPositionX + kArrowHorizontalOffset -
	        kArrowRightLocalCenterX * kArrowScale,
	    kArrowPositionY - kArrowLocalCenterZ * kArrowScale,
	    0.0f,
	};

	for (std::size_t index = 0; index < worldTransforms_.size(); ++index) {
		WorldTransformUpdate(worldTransforms_[index]);
		colors_[index].SetColor({1.0f, 1.0f, 1.0f, opacity_});
	}
}
