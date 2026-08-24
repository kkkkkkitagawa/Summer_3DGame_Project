#include "SlimeObstacle.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
float SmoothStep(float value) {
	return value * value * (3.0f - 2.0f * value);
}

float Lerp(float start, float end, float progress) {
	return start + (end - start) * progress;
}
} // namespace

void SlimeObstacle::Initialize(
    Model* innerModel, Model* outerModel, uint32_t innerTextureHandle,
    uint32_t outerTextureHandle, uint32_t effectSeed,
    BlockFace attachedFace,
    WorldTransform* parent, float blockHalfSize, const Vector3& size) {
	assert(innerModel);
	assert(outerModel);
	assert(parent);
	assert(size.x > 0.0f && size.y > 0.0f && size.z > 0.0f);

	innerModel_ = innerModel;
	outerModel_ = outerModel;
	innerTextureHandle_ = innerTextureHandle;
	outerTextureHandle_ = outerTextureHandle;
	effectRandomEngine_.seed(effectSeed);
	attachedFace_ = attachedFace;
	logicalSize_ = size;
	blockHalfSize_ = blockHalfSize;
	hitPoints_ = 1;
	isCollisionEnabled_ = true;
	isFalling_ = false;
	animationPhase_ = AnimationPhase::Alive;
	animationTime_ = 0.0f;
	visualScale_ = 1.0f;
	opacity_ = 1.0f;
	burstFragments_.clear();

	logicalWorldTransform_.Initialize();
	logicalWorldTransform_.parent_ = parent;
	logicalWorldTransform_.scale_ = {
	    size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
	logicalWorldTransform_.translation_ =
	    CalculateLocalPosition(attachedFace, blockHalfSize, size);
	WorldTransformUpdate(logicalWorldTransform_);

	innerWorldTransform_.Initialize();
	innerWorldTransform_.parent_ = parent;
	outerWorldTransform_.Initialize();
	outerWorldTransform_.parent_ = parent;
	innerColor_.Initialize();
	outerColor_.Initialize();
	UpdateVisualTransforms();
}

void SlimeObstacle::Update(float deltaTime, float gravity) {
	if (isFalling_) {
		velocity_.y -= gravity * deltaTime;
		const Vector3 movement = {
		    velocity_.x * deltaTime,
		    velocity_.y * deltaTime,
		    velocity_.z * deltaTime,
		};
		logicalWorldTransform_.translation_.x += movement.x;
		logicalWorldTransform_.translation_.y += movement.y;
		logicalWorldTransform_.translation_.z += movement.z;
	}

	WorldTransformUpdate(logicalWorldTransform_);
	UpdateAnimation(deltaTime);
	UpdateBurstFragments(deltaTime, gravity);
	UpdateVisualTransforms();
}

bool SlimeObstacle::TriggerHit() {
	if (!isCollisionEnabled_ || hitPoints_ <= 0 ||
	    animationPhase_ != AnimationPhase::Alive) {
		return false;
	}

	--hitPoints_;
	// Disable collision on the contact frame so the same slime cannot retrigger.
	isCollisionEnabled_ = false;
	animationPhase_ = AnimationPhase::Squashing;
	animationTime_ = 0.0f;
	return true;
}

void SlimeObstacle::DetachAndFall(
    const Vector3& inheritedVelocity, float repulsionSpeed) {
	if (!IsAttached()) {
		return;
	}

	const WorldTransform* parent = logicalWorldTransform_.parent_;
	Vector3 outwardNormal = MathUtility::TransformNormal(
	    Obstacle::GetFaceNormal(attachedFace_), parent->matWorld_);
	MathUtility::Normalize(outwardNormal);
	const Vector3 worldPosition = GetWorldPosition();

	logicalWorldTransform_.parent_ = nullptr;
	logicalWorldTransform_.translation_ = worldPosition;
	logicalWorldTransform_.rotation_ = parent->rotation_;
	innerWorldTransform_.parent_ = nullptr;
	outerWorldTransform_.parent_ = nullptr;
	velocity_ = {
	    inheritedVelocity.x + outwardNormal.x * repulsionSpeed,
	    inheritedVelocity.y + outwardNormal.y * repulsionSpeed,
	    inheritedVelocity.z + outwardNormal.z * repulsionSpeed,
	};
	isCollisionEnabled_ = false;
	isFalling_ = true;
	WorldTransformUpdate(logicalWorldTransform_);
	UpdateVisualTransforms();
}

void SlimeObstacle::DrawInner(const Camera& camera) const {
	if (animationPhase_ == AnimationPhase::Bursting ||
	    animationPhase_ == AnimationPhase::Dead) {
		return;
	}
	innerModel_->Draw(
	    innerWorldTransform_, camera, innerTextureHandle_, &innerColor_);
}

void SlimeObstacle::DrawOuter(const Camera& camera) const {
	if (animationPhase_ != AnimationPhase::Bursting &&
	    animationPhase_ != AnimationPhase::Dead) {
		outerModel_->Draw(
		    outerWorldTransform_, camera, outerTextureHandle_, &outerColor_);
	}
	for (const std::unique_ptr<BurstFragment>& fragment : burstFragments_) {
		outerModel_->Draw(
		    fragment->worldTransform, camera, outerTextureHandle_, &outerColor_);
	}
}

AABB SlimeObstacle::GetAABBForParentTransform(
    const Matrix4x4& parentTransform) const {
	Matrix4x4 localTransform =
	    MathUtility::MakeScaleMatrix(logicalWorldTransform_.scale_);
	localTransform = MathUtility::operator*(
	    localTransform,
	    MathUtility::MakeRotateXMatrix(logicalWorldTransform_.rotation_.x));
	localTransform = MathUtility::operator*(
	    localTransform,
	    MathUtility::MakeRotateYMatrix(logicalWorldTransform_.rotation_.y));
	localTransform = MathUtility::operator*(
	    localTransform,
	    MathUtility::MakeRotateZMatrix(logicalWorldTransform_.rotation_.z));
	localTransform = MathUtility::operator*(
	    localTransform,
	    MathUtility::MakeTranslateMatrix(logicalWorldTransform_.translation_));
	return CalculateAABB(
	    MathUtility::operator*(localTransform, parentTransform));
}

Vector3 SlimeObstacle::GetWorldPosition() const {
	return {
	    logicalWorldTransform_.matWorld_.m[3][0],
	    logicalWorldTransform_.matWorld_.m[3][1],
	    logicalWorldTransform_.matWorld_.m[3][2],
	};
}

ObstacleSurfaceRelation SlimeObstacle::GetSurfaceRelation(
    float parentRotationX, const Vector3& playerSurfaceNormal) const {
	const Matrix4x4 logicalRotation =
	    MathUtility::MakeRotateXMatrix(parentRotationX);
	Vector3 obstacleNormal = MathUtility::TransformNormal(
	    Obstacle::GetFaceNormal(attachedFace_), logicalRotation);
	Vector3 normalizedPlayerNormal = playerSurfaceNormal;
	MathUtility::Normalize(obstacleNormal);
	MathUtility::Normalize(normalizedPlayerNormal);
	const float alignment =
	    MathUtility::Dot(obstacleNormal, normalizedPlayerNormal);
	if (alignment > 0.9f) {
		return ObstacleSurfaceRelation::PlayerFace;
	}
	if (alignment < -0.9f) {
		return ObstacleSurfaceRelation::OppositeFace;
	}
	return obstacleNormal.z >= 0.0f
	           ? ObstacleSurfaceRelation::PositiveSide
	           : ObstacleSurfaceRelation::NegativeSide;
}

Vector3 SlimeObstacle::CalculateLocalPosition(
    BlockFace attachedFace, float blockHalfSize, const Vector3& size) {
	switch (attachedFace) {
	case BlockFace::Top:
		return {0.0f, blockHalfSize + size.y * 0.5f, 0.0f};
	case BlockFace::Bottom:
		return {0.0f, -(blockHalfSize + size.y * 0.5f), 0.0f};
	case BlockFace::Front:
		return {0.0f, 0.0f, blockHalfSize + size.z * 0.5f};
	case BlockFace::Back:
		return {0.0f, 0.0f, -(blockHalfSize + size.z * 0.5f)};
	}
	return {};
}

AABB SlimeObstacle::CalculateAABB(const Matrix4x4& worldTransform) const {
	constexpr float halfUnit = 1.0f;
	const Vector3 localCorners[8] = {
	    {-halfUnit, -halfUnit, -halfUnit},
	    {halfUnit, -halfUnit, -halfUnit},
	    {halfUnit, halfUnit, -halfUnit},
	    {-halfUnit, halfUnit, -halfUnit},
	    {-halfUnit, -halfUnit, halfUnit},
	    {halfUnit, -halfUnit, halfUnit},
	    {halfUnit, halfUnit, halfUnit},
	    {-halfUnit, halfUnit, halfUnit},
	};

	Vector3 transformedCorner =
	    MathUtility::TransformCoord(localCorners[0], worldTransform);
	AABB result = {transformedCorner, transformedCorner};
	for (std::size_t index = 1; index < 8; ++index) {
		transformedCorner =
		    MathUtility::TransformCoord(localCorners[index], worldTransform);
		result.min.x = (std::min)(result.min.x, transformedCorner.x);
		result.min.y = (std::min)(result.min.y, transformedCorner.y);
		result.min.z = (std::min)(result.min.z, transformedCorner.z);
		result.max.x = (std::max)(result.max.x, transformedCorner.x);
		result.max.y = (std::max)(result.max.y, transformedCorner.y);
		result.max.z = (std::max)(result.max.z, transformedCorner.z);
	}
	return result;
}

void SlimeObstacle::UpdateAnimation(float deltaTime) {
	switch (animationPhase_) {
	case AnimationPhase::Alive:
	case AnimationPhase::Bursting:
	case AnimationPhase::Dead:
		return;
	case AnimationPhase::Squashing: {
		animationTime_ =
		    (std::min)(animationTime_ + deltaTime, kSquashDuration);
		const float progress = SmoothStep(animationTime_ / kSquashDuration);
		visualScale_ = Lerp(1.0f, kSquashedScale, progress);
		if (animationTime_ >= kSquashDuration) {
			animationPhase_ = AnimationPhase::Expanding;
			animationTime_ = 0.0f;
		}
		return;
	}
	case AnimationPhase::Expanding: {
		animationTime_ =
		    (std::min)(animationTime_ + deltaTime, kExpandDuration);
		const float progress = SmoothStep(animationTime_ / kExpandDuration);
		visualScale_ =
		    Lerp(kSquashedScale, kExpandedScale, progress);
		if (animationTime_ >= kExpandDuration) {
			StartBurst();
		}
		return;
	}
	}
}

void SlimeObstacle::StartBurst() {
	animationPhase_ = AnimationPhase::Bursting;
	animationTime_ = 0.0f;
	opacity_ = 1.0f;
	burstFragments_.clear();

	std::uniform_int_distribution<int> fragmentCountDistribution(
	    kMinimumBurstFragmentCount, kMaximumBurstFragmentCount);
	std::uniform_real_distribution<float> angleDistribution(
	    0.0f, 2.0f * std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> horizontalSpeedDistribution(
	    kMinimumHorizontalBurstSpeed, kMaximumHorizontalBurstSpeed);
	std::uniform_real_distribution<float> upwardSpeedDistribution(
	    kMinimumUpwardBurstSpeed, kMaximumUpwardBurstSpeed);
	std::uniform_real_distribution<float> scaleDistribution(
	    kMinimumFragmentScale, kMaximumFragmentScale);
	std::uniform_real_distribution<float> rotationDistribution(
	    0.0f, 2.0f * std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> rotationSpeedDistribution(
	    -8.0f, 8.0f);

	const Vector3 burstOrigin = GetWorldPosition();
	const int fragmentCount = fragmentCountDistribution(effectRandomEngine_);
	burstFragments_.reserve(static_cast<std::size_t>(fragmentCount));
	for (int index = 0; index < fragmentCount; ++index) {
		auto fragment = std::make_unique<BurstFragment>();
		fragment->worldTransform.Initialize();
		const float fragmentScale = scaleDistribution(effectRandomEngine_);
		fragment->worldTransform.scale_ = {
		    fragmentScale, fragmentScale, fragmentScale};
		fragment->worldTransform.rotation_ = {
		    rotationDistribution(effectRandomEngine_),
		    rotationDistribution(effectRandomEngine_),
		    rotationDistribution(effectRandomEngine_),
		};
		fragment->worldTransform.translation_ = burstOrigin;
		const float angle = angleDistribution(effectRandomEngine_);
		const float horizontalSpeed =
		    horizontalSpeedDistribution(effectRandomEngine_);
		fragment->velocity = {
		    std::cos(angle) * horizontalSpeed,
		    upwardSpeedDistribution(effectRandomEngine_),
		    std::sin(angle) * horizontalSpeed,
		};
		fragment->rotationVelocity = {
		    rotationSpeedDistribution(effectRandomEngine_),
		    rotationSpeedDistribution(effectRandomEngine_),
		    rotationSpeedDistribution(effectRandomEngine_),
		};
		WorldTransformUpdate(fragment->worldTransform);
		burstFragments_.push_back(std::move(fragment));
	}
}

void SlimeObstacle::UpdateBurstFragments(
    float deltaTime, float gravity) {
	if (animationPhase_ != AnimationPhase::Bursting) {
		return;
	}

	for (const std::unique_ptr<BurstFragment>& fragment : burstFragments_) {
		fragment->elapsedTime += deltaTime;
		fragment->velocity.y -= gravity * deltaTime;
		fragment->worldTransform.translation_.x +=
		    fragment->velocity.x * deltaTime;
		fragment->worldTransform.translation_.y +=
		    fragment->velocity.y * deltaTime;
		fragment->worldTransform.translation_.z +=
		    fragment->velocity.z * deltaTime;
		fragment->worldTransform.rotation_.x +=
		    fragment->rotationVelocity.x * deltaTime;
		fragment->worldTransform.rotation_.y +=
		    fragment->rotationVelocity.y * deltaTime;
		fragment->worldTransform.rotation_.z +=
		    fragment->rotationVelocity.z * deltaTime;
		WorldTransformUpdate(fragment->worldTransform);
	}

	burstFragments_.erase(
	    std::remove_if(
	        burstFragments_.begin(), burstFragments_.end(),
	        [](const std::unique_ptr<BurstFragment>& fragment) {
		        return fragment->elapsedTime >= kFragmentLifetime;
	        }),
	    burstFragments_.end());
	if (burstFragments_.empty()) {
		animationPhase_ = AnimationPhase::Dead;
		opacity_ = 0.0f;
	}
}

void SlimeObstacle::UpdateVisualTransforms() {
	const Vector3 visualScale = {
	    logicalSize_.x * 0.5f * visualScale_,
	    logicalSize_.y * 0.5f * visualScale_,
	    logicalSize_.z * 0.5f * visualScale_,
	};
	innerWorldTransform_.scale_ = visualScale;
	outerWorldTransform_.scale_ = visualScale;
	innerWorldTransform_.translation_ = logicalWorldTransform_.translation_;
	outerWorldTransform_.translation_ = logicalWorldTransform_.translation_;
	innerWorldTransform_.rotation_ = logicalWorldTransform_.rotation_;
	outerWorldTransform_.rotation_ = logicalWorldTransform_.rotation_;
	WorldTransformUpdate(innerWorldTransform_);
	WorldTransformUpdate(outerWorldTransform_);
	innerColor_.SetColor({1.0f, 1.0f, 1.0f, opacity_});
	outerColor_.SetColor({1.0f, 1.0f, 1.0f, opacity_});
}
