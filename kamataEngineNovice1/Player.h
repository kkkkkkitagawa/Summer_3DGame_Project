#pragma once

#include "Collision.h"

class Player {
public:
	void Initialize(
	    KamataEngine::Model* model, const KamataEngine::Vector3& position);
	void Update();
	void Draw(const KamataEngine::Camera& camera) const;

	KamataEngine::Vector3 GetWorldPosition() const;
	AABB GetAABB() const;

	static inline const KamataEngine::Vector3 kCollisionHalfSize = {
	    0.5f,
	    0.5f,
	    0.5f,
	};
	static inline const KamataEngine::Vector3 kModelScale = {
	    2.0f,
	    2.0f,
	    2.0f,
	};

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
};
