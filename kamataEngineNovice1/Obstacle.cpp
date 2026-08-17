#include "Obstacle.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>

using namespace KamataEngine;

void Obstacle::Initialize(
    Model* model, BlockFace attachedFace, WorldTransform* parent,
    float blockHalfSize, const Vector3& size) {
	assert(model);
	assert(parent);
	assert(size.x > 0.0f && size.y > 0.0f && size.z > 0.0f);

	model_ = model;
	attachedFace_ = attachedFace;
	worldTransform_.Initialize();
	worldTransform_.parent_ = parent;
	worldTransform_.scale_ = size;
	worldTransform_.translation_ =
	    CalculateLocalPosition(attachedFace, blockHalfSize, size);
	WorldTransformUpdate(worldTransform_);
}

void Obstacle::Update() {
	WorldTransformUpdate(worldTransform_);
}

void Obstacle::Draw(const Camera& camera) const {
	model_->Draw(worldTransform_, camera);
}

AABB Obstacle::GetAABB() const {
	constexpr float halfUnit = 0.5f;
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
	    MathUtility::TransformCoord(localCorners[0], worldTransform_.matWorld_);
	AABB result = {transformedCorner, transformedCorner};
	for (std::size_t index = 1; index < 8; ++index) {
		transformedCorner = MathUtility::TransformCoord(
		    localCorners[index], worldTransform_.matWorld_);
		result.min.x = (std::min)(result.min.x, transformedCorner.x);
		result.min.y = (std::min)(result.min.y, transformedCorner.y);
		result.min.z = (std::min)(result.min.z, transformedCorner.z);
		result.max.x = (std::max)(result.max.x, transformedCorner.x);
		result.max.y = (std::max)(result.max.y, transformedCorner.y);
		result.max.z = (std::max)(result.max.z, transformedCorner.z);
	}
	return result;
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
