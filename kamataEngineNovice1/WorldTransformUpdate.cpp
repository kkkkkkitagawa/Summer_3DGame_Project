#include "WorldTransformUpdate.h"

using namespace KamataEngine;

namespace {
Matrix4x4 Multiply(const Matrix4x4& left, const Matrix4x4& right) {
	Matrix4x4 result = {};
	for (int row = 0; row < 4; ++row) {
		for (int column = 0; column < 4; ++column) {
			for (int index = 0; index < 4; ++index) {
				result.m[row][column] += left.m[row][index] * right.m[index][column];
			}
		}
	}
	return result;
}
} // namespace

void WorldTransformUpdate(WorldTransform& worldTransform) {
	Matrix4x4 scaleMatrix = MathUtility::MakeScaleMatrix(worldTransform.scale_);
	Matrix4x4 rotateXMatrix = MathUtility::MakeRotateXMatrix(worldTransform.rotation_.x);
	Matrix4x4 rotateYMatrix = MathUtility::MakeRotateYMatrix(worldTransform.rotation_.y);
	Matrix4x4 rotateZMatrix = MathUtility::MakeRotateZMatrix(worldTransform.rotation_.z);
	Matrix4x4 translateMatrix = MathUtility::MakeTranslateMatrix(worldTransform.translation_);

	worldTransform.matWorld_ = Multiply(scaleMatrix, rotateXMatrix);
	worldTransform.matWorld_ = Multiply(worldTransform.matWorld_, rotateYMatrix);
	worldTransform.matWorld_ = Multiply(worldTransform.matWorld_, rotateZMatrix);
	worldTransform.matWorld_ = Multiply(worldTransform.matWorld_, translateMatrix);
	if (worldTransform.parent_) {
		worldTransform.matWorld_ = Multiply(worldTransform.matWorld_, worldTransform.parent_->matWorld_);
	}

	worldTransform.TransferMatrix();
}
