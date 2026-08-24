#include "FallenBlockCounter.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <numbers>
#include <string>

using namespace KamataEngine;

FallenBlockCounter::~FallenBlockCounter() {
	for (Model* model : digitModels_) {
		delete model;
	}
}

void FallenBlockCounter::Initialize() {
	for (std::size_t digit = 0; digit < digitModels_.size(); ++digit) {
		digitModels_[digit] =
		    Model::CreateFromOBJ("num" + std::to_string(digit), false);
		assert(digitModels_[digit]);
	}
	textureHandle_ = TextureManager::Load("number/num/num.png");

	for (WorldTransform& transform : digitTransforms_) {
		transform.Initialize();
		// Blender文字のX-Z平面を画面へ正対させ、表示倍率を0.5倍にする。
		transform.scale_ = {-kModelScale, kModelScale, kModelScale};
		transform.rotation_.x = std::numbers::pi_v<float> * 0.5f;
	}
	SetValue(0);
}

void FallenBlockCounter::SetValue(std::size_t value) {
	const std::string text = std::to_string(value);
	visibleDigitCount_ = (std::min)(text.size(), digitTransforms_.size());
	const float totalWidth =
	    static_cast<float>(visibleDigitCount_ - 1) * kDigitSpacing;

	for (std::size_t index = 0; index < visibleDigitCount_; ++index) {
		displayedDigits_[index] =
		    static_cast<std::size_t>(text[index] - '0');
		WorldTransform& transform = digitTransforms_[index];
		transform.translation_ = {
		    -totalWidth * 0.5f + static_cast<float>(index) * kDigitSpacing,
		    kTopPositionY,
		    0.0f,
		};
		WorldTransformUpdate(transform);
	}
}

void FallenBlockCounter::Draw(const Camera& camera) const {
	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kOff);
	for (std::size_t index = 0; index < visibleDigitCount_; ++index) {
		digitModels_[displayedDigits_[index]]->Draw(
		    digitTransforms_[index], camera, textureHandle_);
	}
	Model::PostDraw();
}
