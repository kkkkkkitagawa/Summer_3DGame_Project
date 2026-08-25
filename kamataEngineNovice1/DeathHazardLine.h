#pragma once

#include "KamataEngine.h"

// 失败边界的警戒线。当前仅负责根据玩家位置渐显，不参与碰撞或死亡判定。
class DeathHazardLine {
public:
	~DeathHazardLine();

	void Initialize(const KamataEngine::Vector3& position);
	void Update(float playerPositionX);
	void Draw(const KamataEngine::Camera& camera) const;

private:
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::ObjectColor color_;
	float opacity_ = 0.0f;

	static inline constexpr float kVisibleStartX = -1.5f;
	static inline constexpr float kMaximumOpacityX = -2.5f;
	static inline constexpr float kMinimumOpacity = 0.3f;
};
