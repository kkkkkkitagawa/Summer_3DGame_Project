#include "GoalStair.h"

#include <cassert>

using namespace KamataEngine;

void GoalStair::Initialize(
    Model* model, BlockFace attachedFace, WorldTransform* parent,
    float blockHalfSize, float outlineThickness) {
	assert(model);
	assert(parent);
	assert(blockHalfSize > 0.0f);
	assert(outlineThickness >= 0.0f);
	model_ = model;
	attachedFace_ = attachedFace;
	parent_ = parent;
	outlineThickness_ = outlineThickness;
	surfaceOffset_ = blockHalfSize - kModelLowestY * modelScale_;
	worldTransform_.Initialize();
	outlineWorldTransform_.Initialize();
	Update();
}

void GoalStair::Update() {
	worldTransform_.matWorld_ = MathUtility::operator*(
	    CreateLocalTransform(modelScale_), parent_->matWorld_);
	worldTransform_.TransferMatrix();
	outlineWorldTransform_.matWorld_ = MathUtility::operator*(
	    CreateLocalTransform(modelScale_ + outlineThickness_),
	    parent_->matWorld_);
	outlineWorldTransform_.TransferMatrix();
}

void GoalStair::Draw(const Camera& camera) const {
	model_->Draw(worldTransform_, camera);
}

void GoalStair::DrawOutline(
    const Camera& camera, const ObjectColor& outlineColor) const {
	model_->Draw(outlineWorldTransform_, camera, &outlineColor);
}

Matrix4x4 GoalStair::CreateLocalTransform(float scale) const {
	const Vector3 surfaceNormal = Obstacle::GetFaceNormal(attachedFace_);
	// The model rises toward local +Z. Keep that direction on world +X so the
	// low end always faces the approaching player, regardless of map face.
	const Vector3 forward = {1.0f, 0.0f, 0.0f};
	const Vector3 width = Cross(surfaceNormal, forward);
	Matrix4x4 orientation = {};
	orientation.m[0][0] = width.x;
	orientation.m[0][1] = width.y;
	orientation.m[0][2] = width.z;
	orientation.m[1][0] = surfaceNormal.x;
	orientation.m[1][1] = surfaceNormal.y;
	orientation.m[1][2] = surfaceNormal.z;
	orientation.m[2][0] = forward.x;
	orientation.m[2][1] = forward.y;
	orientation.m[2][2] = forward.z;
	orientation.m[3][3] = 1.0f;

	Matrix4x4 localTransform = MathUtility::MakeScaleMatrix(
	    {scale, scale, scale});
	localTransform = MathUtility::operator*(localTransform, orientation);
	localTransform = MathUtility::operator*(
	    localTransform,
	    MathUtility::MakeTranslateMatrix({
	        surfaceNormal.x * surfaceOffset_,
	        surfaceNormal.y * surfaceOffset_,
	        surfaceNormal.z * surfaceOffset_,
	    }));
	return localTransform;
}

Vector3 GoalStair::Cross(const Vector3& first, const Vector3& second) {
	return {
	    first.y * second.z - first.z * second.y,
	    first.z * second.x - first.x * second.z,
	    first.x * second.y - first.y * second.x,
	};
}
