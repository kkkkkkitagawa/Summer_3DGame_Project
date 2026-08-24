#include "Obstacle.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>

using namespace KamataEngine;

void Obstacle::Initialize(
    Model* model, BlockFace attachedFace, WorldTransform* parent,
    float blockHalfSize, const Vector3& size,
    ObstacleInteractionRules interactionRules, float visualHeightScale,
    float visualGrowthDuration, float outlineThickness) {
	assert(model);
	assert(parent);
	assert(size.x > 0.0f && size.y > 0.0f && size.z > 0.0f);
	assert(visualHeightScale > 0.0f);
	assert(visualGrowthDuration > 0.0f);
	assert(outlineThickness >= 0.0f);

	model_ = model;
	attachedFace_ = attachedFace;
	interactionRules_ = interactionRules;
	logicalSize_ = size;
	blockHalfSize_ = blockHalfSize;
	visualHeightScale_ = visualHeightScale;
	visualGrowthDuration_ = visualGrowthDuration;
	visualGrowthElapsed_ = 0.0f;
	outlineThickness_ = outlineThickness;
	worldTransform_.Initialize();
	worldTransform_.parent_ = parent;
	// Resources/cube has local bounds from -1 to +1 on every axis.
	worldTransform_.scale_ = {size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
	worldTransform_.translation_ =
	    CalculateLocalPosition(attachedFace, blockHalfSize, size);
	WorldTransformUpdate(worldTransform_);

	visualWorldTransform_.Initialize();
	visualWorldTransform_.parent_ = parent;
	outlineWorldTransform_.Initialize();
	outlineWorldTransform_.parent_ = parent;
	UpdateVisualTransform();
}

void Obstacle::Update(
    float deltaTime, float gravity, float timeUntilGrowthDeadline,
    bool canStartVisualGrowth) {
	if (isFalling_) {
		velocity_.y -= gravity * deltaTime;
		const Vector3 movement = {
		    velocity_.x * deltaTime,
		    velocity_.y * deltaTime,
		    velocity_.z * deltaTime,
		};
		worldTransform_.translation_.x += movement.x;
		worldTransform_.translation_.y += movement.y;
		worldTransform_.translation_.z += movement.z;
		visualWorldTransform_.translation_.x += movement.x;
		visualWorldTransform_.translation_.y += movement.y;
		visualWorldTransform_.translation_.z += movement.z;
		outlineWorldTransform_.translation_.x += movement.x;
		outlineWorldTransform_.translation_.y += movement.y;
		outlineWorldTransform_.translation_.z += movement.z;
		WorldTransformUpdate(worldTransform_);
		WorldTransformUpdate(visualWorldTransform_);
		WorldTransformUpdate(outlineWorldTransform_);
		return;
	}

	if (canStartVisualGrowth) {
		isVisualGrowthStarted_ = true;
	}
	if (isVisualGrowthStarted_) {
		visualGrowthElapsed_ = (std::min)(
		    visualGrowthDuration_, visualGrowthElapsed_ + deltaTime);
	}
	// If the map speed is increased after this obstacle spawns, advance the
	// presentation as needed so it still finishes before reaching x = 0.
	const float minimumElapsedForDeadline =
	    visualGrowthDuration_ - (std::max)(0.0f, timeUntilGrowthDeadline);
	visualGrowthElapsed_ = (std::max)(
	    visualGrowthElapsed_,
	    std::clamp(
	        minimumElapsedForDeadline, 0.0f, visualGrowthDuration_));
	WorldTransformUpdate(worldTransform_);
	UpdateVisualTransform();
}

void Obstacle::DetachAndFall(
    const Vector3& inheritedVelocity, float repulsionSpeed) {
	if (!IsAttached()) {
		return;
	}

	const WorldTransform* parent = worldTransform_.parent_;
	Vector3 outwardNormal = MathUtility::TransformNormal(
	    GetFaceNormal(attachedFace_), parent->matWorld_);
	MathUtility::Normalize(outwardNormal);
	const Vector3 worldPosition = GetWorldPosition();
	const Vector3 visualWorldPosition = {
	    visualWorldTransform_.matWorld_.m[3][0],
	    visualWorldTransform_.matWorld_.m[3][1],
	    visualWorldTransform_.matWorld_.m[3][2],
	};
	const Vector3 outlineWorldPosition = {
	    outlineWorldTransform_.matWorld_.m[3][0],
	    outlineWorldTransform_.matWorld_.m[3][1],
	    outlineWorldTransform_.matWorld_.m[3][2],
	};

	worldTransform_.parent_ = nullptr;
	worldTransform_.translation_ = worldPosition;
	worldTransform_.rotation_ = parent->rotation_;
	visualWorldTransform_.parent_ = nullptr;
	visualWorldTransform_.translation_ = visualWorldPosition;
	visualWorldTransform_.rotation_ = parent->rotation_;
	outlineWorldTransform_.parent_ = nullptr;
	outlineWorldTransform_.translation_ = outlineWorldPosition;
	outlineWorldTransform_.rotation_ = parent->rotation_;
	velocity_ = {
	    inheritedVelocity.x + outwardNormal.x * repulsionSpeed,
	    inheritedVelocity.y + outwardNormal.y * repulsionSpeed,
	    inheritedVelocity.z + outwardNormal.z * repulsionSpeed,
	};
	isCollisionEnabled_ = false;
	isFalling_ = true;
	WorldTransformUpdate(worldTransform_);
	WorldTransformUpdate(visualWorldTransform_);
	WorldTransformUpdate(outlineWorldTransform_);
}

void Obstacle::Draw(const Camera& camera) const {
	model_->Draw(visualWorldTransform_, camera);
}

void Obstacle::DrawOutline(
    const Camera& camera, const ObjectColor& outlineColor) const {
	model_->Draw(outlineWorldTransform_, camera, &outlineColor);
}

void Obstacle::UpdateVisualTransform() {
	const float progress = std::clamp(
	    visualGrowthElapsed_ / visualGrowthDuration_, 0.0f, 1.0f);
	const float smoothProgress = progress * progress * (3.0f - 2.0f * progress);
	const float currentHeightScale =
	    kMinimumVisualHeightScale +
	    (visualHeightScale_ - kMinimumVisualHeightScale) * smoothProgress;

	Vector3 visualSize = logicalSize_;
	switch (attachedFace_) {
	case BlockFace::Top:
	case BlockFace::Bottom:
		visualSize.y *= currentHeightScale;
		break;
	case BlockFace::Front:
	case BlockFace::Back:
		visualSize.z *= currentHeightScale;
		break;
	}

	visualWorldTransform_.scale_ = {
	    visualSize.x * 0.5f,
	    visualSize.y * 0.5f,
	    visualSize.z * 0.5f,
	};
	visualWorldTransform_.translation_ =
	    CalculateLocalPosition(attachedFace_, blockHalfSize_, visualSize);
	WorldTransformUpdate(visualWorldTransform_);

	// Grow the outline thickness with the presentation so a black slab does not
	// appear while the obstacle itself is still visually flat.
	const float outlineExpansion = outlineThickness_ * smoothProgress;
	outlineWorldTransform_.scale_ = {
	    visualWorldTransform_.scale_.x + outlineExpansion,
	    visualWorldTransform_.scale_.y + outlineExpansion,
	    visualWorldTransform_.scale_.z + outlineExpansion,
	};
	outlineWorldTransform_.translation_ = visualWorldTransform_.translation_;
	outlineWorldTransform_.rotation_ = visualWorldTransform_.rotation_;
	WorldTransformUpdate(outlineWorldTransform_);
}

AABB Obstacle::GetAABB() const {
	return CalculateAABB(worldTransform_.matWorld_);
}

AABB Obstacle::GetAABBForParentTransform(
    const Matrix4x4& parentTransform) const {
	Matrix4x4 localTransform =
	    MathUtility::MakeScaleMatrix(worldTransform_.scale_);
	localTransform = MathUtility::operator*(
	    localTransform,
	    MathUtility::MakeRotateXMatrix(worldTransform_.rotation_.x));
	localTransform = MathUtility::operator*(
	    localTransform,
	    MathUtility::MakeRotateYMatrix(worldTransform_.rotation_.y));
	localTransform = MathUtility::operator*(
	    localTransform,
	    MathUtility::MakeRotateZMatrix(worldTransform_.rotation_.z));
	localTransform = MathUtility::operator*(
	    localTransform,
	    MathUtility::MakeTranslateMatrix(worldTransform_.translation_));
	return CalculateAABB(
	    MathUtility::operator*(localTransform, parentTransform));
}

AABB Obstacle::CalculateAABB(const Matrix4x4& worldTransform) const {
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
		transformedCorner = MathUtility::TransformCoord(
		    localCorners[index], worldTransform);
		result.min.x = (std::min)(result.min.x, transformedCorner.x);
		result.min.y = (std::min)(result.min.y, transformedCorner.y);
		result.min.z = (std::min)(result.min.z, transformedCorner.z);
		result.max.x = (std::max)(result.max.x, transformedCorner.x);
		result.max.y = (std::max)(result.max.y, transformedCorner.y);
		result.max.z = (std::max)(result.max.z, transformedCorner.z);
	}
	return result;
}

ObstacleSurfaceRelation Obstacle::GetSurfaceRelation(
    float parentRotationX, const Vector3& playerSurfaceNormal) const {
	const Matrix4x4 logicalRotation =
	    MathUtility::MakeRotateXMatrix(parentRotationX);
	Vector3 obstacleNormal = MathUtility::TransformNormal(
	    GetFaceNormal(attachedFace_), logicalRotation);
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

Vector3 Obstacle::GetWorldPosition() const {
	return {
	    worldTransform_.matWorld_.m[3][0],
	    worldTransform_.matWorld_.m[3][1],
	    worldTransform_.matWorld_.m[3][2],
	};
}

Vector3 Obstacle::CalculateLocalPosition(
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

Vector3 Obstacle::GetFaceNormal(BlockFace attachedFace) {
	switch (attachedFace) {
	case BlockFace::Top:
		return {0.0f, 1.0f, 0.0f};
	case BlockFace::Bottom:
		return {0.0f, -1.0f, 0.0f};
	case BlockFace::Front:
		return {0.0f, 0.0f, 1.0f};
	case BlockFace::Back:
		return {0.0f, 0.0f, -1.0f};
	}
	return {};
}
