#include "FloatingBlockSystem.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
Vector3 Add(const Vector3& left, const Vector3& right) {
	return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vector3 Subtract(const Vector3& left, const Vector3& right) {
	return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vector3 Scale(const Vector3& value, float scale) {
	return {value.x * scale, value.y * scale, value.z * scale};
}

float LengthSquared(const Vector3& value) {
	return value.x * value.x + value.y * value.y + value.z * value.z;
}
} // namespace

FloatingBlockSystem::FloatingBlockSystem()
    : randomEngine_(std::random_device{}()) {}

FloatingBlockSystem::~FloatingBlockSystem() { delete model_; }

void FloatingBlockSystem::Initialize(
    const Camera& playerCamera, float mapGroundHeight) {
	delete model_;
	model_ = Model::CreateFromOBJ("Floating_Block", true);
	assert(model_);
	textureHandle_ =
	    TextureManager::Load("Floating_Block/Floating_Block.png");

	blocks_.clear();
	blocks_.reserve(kBlockCount);
	const CameraBasis basis = CalculateCameraBasis(playerCamera);
	constexpr float horizontalStart = -1.08f;
	constexpr float horizontalEnd = 1.08f;
	constexpr float upperBandY = -0.82f;
	constexpr float lowerBandY = -1.05f;

	for (std::size_t index = 0; index < kBlockCount; ++index) {
		bool wasPlaced = false;

		for (int attempt = 0; attempt < 180 && !wasPlaced; ++attempt) {
			auto candidate = std::make_unique<FloatingBlock>();
			candidate->worldTransform.Initialize();

			const float horizontalProgress =
			    static_cast<float>(index) /
			    static_cast<float>(kBlockCount - 1);
			const float cellX =
			    horizontalStart +
			    (horizontalEnd - horizontalStart) * horizontalProgress;
			const float cellY =
			    index % 2 == 0 ? upperBandY : lowerBandY;
			const float normalizedX =
			    cellX + RandomFloat(-0.025f, 0.025f);
			const float normalizedY =
			    cellY + RandomFloat(-0.025f, 0.025f);
			const float depth = RandomFloat(kMinimumDepth, kMaximumDepth);
			const float halfHeight =
			    std::tan(playerCamera.fovAngleY * 0.5f) * depth;
			const float halfWidth = halfHeight * playerCamera.aspectRatio;
			const float reducedMaximumScale =
			    kMaximumScale - static_cast<float>(attempt) * 0.0065f;
			const float maximumScale =
			    reducedMaximumScale > kMinimumScale
			        ? reducedMaximumScale
			        : kMinimumScale;
			const float blockScale =
			    RandomFloat(kMinimumScale, maximumScale);

			candidate->basePosition = Add(
			    playerCamera.translation_,
			    Add(
			        Scale(basis.right, normalizedX * halfWidth),
			        Add(
			            Scale(basis.up, normalizedY * halfHeight),
			            Scale(basis.forward, depth))));
			candidate->bobAmplitude = RandomFloat(0.06f, 0.16f);
			candidate->collisionRadius =
			    std::sqrt(3.0f) * blockScale + candidate->bobAmplitude;
			candidate->screenX = normalizedX;
			candidate->screenY = normalizedY;
			candidate->screenRadius =
			    candidate->collisionRadius / halfHeight;

			if (candidate->basePosition.y + candidate->collisionRadius >=
			        mapGroundHeight - kMapClearance ||
			    !CanPlaceBlock(*candidate)) {
				continue;
			}

			candidate->worldTransform.scale_ = {
			    blockScale, blockScale, blockScale};
			candidate->worldTransform.rotation_ = {
			    RandomFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
			    RandomFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
			    RandomFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
			};
			candidate->worldTransform.translation_ = candidate->basePosition;
			candidate->angularVelocity = {
			    RandomFloat(-0.18f, 0.18f),
			    RandomFloat(-0.18f, 0.18f),
			    RandomFloat(-0.18f, 0.18f),
			};
			candidate->bobPhase = RandomFloat(
			    0.0f, 2.0f * std::numbers::pi_v<float>);
			candidate->bobSpeed = RandomFloat(0.25f, 0.55f);
			WorldTransformUpdate(candidate->worldTransform);
			blocks_.push_back(std::move(candidate));
			wasPlaced = true;
		}
	}
}

void FloatingBlockSystem::Update() {
	for (const std::unique_ptr<FloatingBlock>& blockPointer : blocks_) {
		FloatingBlock& block = *blockPointer;
		block.bobPhase += block.bobSpeed * kDeltaTime;
		block.worldTransform.translation_ = block.basePosition;
		block.worldTransform.translation_.y +=
		    std::sin(block.bobPhase) * block.bobAmplitude;
		block.worldTransform.rotation_.x +=
		    block.angularVelocity.x * kDeltaTime;
		block.worldTransform.rotation_.y +=
		    block.angularVelocity.y * kDeltaTime;
		block.worldTransform.rotation_.z +=
		    block.angularVelocity.z * kDeltaTime;
		WorldTransformUpdate(block.worldTransform);
	}
}

void FloatingBlockSystem::Draw(const Camera& camera) const {
	if (!model_) {
		return;
	}

	Model::PreDraw();
	for (const std::unique_ptr<FloatingBlock>& blockPointer : blocks_) {
		const FloatingBlock& block = *blockPointer;
		model_->Draw(block.worldTransform, camera, textureHandle_);
	}
	Model::PostDraw();
}

FloatingBlockSystem::CameraBasis
FloatingBlockSystem::CalculateCameraBasis(const Camera& camera) const {
	const float sinPitch = std::sin(camera.rotation_.x);
	const float cosPitch = std::cos(camera.rotation_.x);
	const float sinYaw = std::sin(camera.rotation_.y);
	const float cosYaw = std::cos(camera.rotation_.y);
	return {
	    {cosYaw, 0.0f, -sinYaw},
	    {sinPitch * sinYaw, cosPitch, sinPitch * cosYaw},
	    {cosPitch * sinYaw, -sinPitch, cosPitch * cosYaw},
	};
}

bool FloatingBlockSystem::CanPlaceBlock(
    const FloatingBlock& candidate) const {
	for (const std::unique_ptr<FloatingBlock>& blockPointer : blocks_) {
		const FloatingBlock& block = *blockPointer;
		const Vector3 separation =
		    Subtract(candidate.basePosition, block.basePosition);
		const float requiredWorldDistance =
		    candidate.collisionRadius + block.collisionRadius +
		    kMinimumWorldGap;
		if (LengthSquared(separation) <
		    requiredWorldDistance * requiredWorldDistance) {
			return false;
		}

		const float screenX = candidate.screenX - block.screenX;
		const float screenY = candidate.screenY - block.screenY;
		const float requiredScreenDistance =
		    candidate.screenRadius + block.screenRadius +
		    kMinimumScreenGap;
		if (screenX * screenX + screenY * screenY <
		    requiredScreenDistance * requiredScreenDistance) {
			return false;
		}
	}
	return true;
}

float FloatingBlockSystem::RandomFloat(float minimum, float maximum) {
	return std::uniform_real_distribution<float>(minimum, maximum)(
	    randomEngine_);
}
