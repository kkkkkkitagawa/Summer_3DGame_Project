#pragma once

#include "KamataEngine.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

// Green paper-like fragments emitted below the player while slime slowdown is
// active.
class SlowdownTrailEffect {
public:
	SlowdownTrailEffect();
	~SlowdownTrailEffect();

	void Initialize();
	void Update(
	    bool shouldEmit, const KamataEngine::Vector3& playerPosition,
	    float deltaTime);
	void Draw(const KamataEngine::Camera& camera) const;

private:
	struct Fragment {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor color;
		KamataEngine::Vector3 velocity = {};
		KamataEngine::Vector3 angularVelocity = {};
		float age = 0.0f;
		float lifetime = 0.0f;
		bool isActive = false;
	};

	void SpawnFragment(const KamataEngine::Vector3& playerPosition);
	float RandomFloat(float minimum, float maximum);

	static inline constexpr std::size_t kMaximumFragmentCount = 28;
	static inline constexpr float kSpawnInterval = 0.055f;
	static inline constexpr float kGravity = 0.75f;

	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	std::array<Fragment, kMaximumFragmentCount> fragments_ = {};
	std::mt19937 randomEngine_;
	float spawnTimer_ = 0.0f;
	bool wasEmitting_ = false;
};
