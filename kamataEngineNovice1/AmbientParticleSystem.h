#pragma once

#include "KamataEngine.h"

#include <array>
#include <cstddef>
#include <random>

// Camera-frustum ambient particles shared by every scene.
class AmbientParticleSystem {
public:
	AmbientParticleSystem();
	~AmbientParticleSystem();

	void Initialize();
	void Update(const KamataEngine::Camera& camera);
	void Draw(const KamataEngine::Camera& camera) const;

private:
	struct Particle {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor color;
		KamataEngine::Vector3 velocity = {};
		KamataEngine::Vector3 angularVelocity = {};
		KamataEngine::Vector3 driftAxis = {};
		float driftPhase = 0.0f;
		float driftSpeed = 0.0f;
		float driftAmount = 0.0f;
		float age = 0.0f;
		float visibleLifetime = 0.0f;
		bool isActive = false;
	};

	struct CameraBasis {
		KamataEngine::Vector3 right;
		KamataEngine::Vector3 up;
		KamataEngine::Vector3 forward;
	};

	void RespawnParticle(
	    Particle& particle, const KamataEngine::Camera& camera,
	    bool initializeTransform);
	bool IsInsideSpawnVolume(
	    const Particle& particle, const KamataEngine::Camera& camera) const;
	CameraBasis CalculateCameraBasis(
	    const KamataEngine::Camera& camera) const;
	float RandomFloat(float minimum, float maximum);

	static inline constexpr std::size_t kMaximumParticleCount = 14;
	static inline constexpr float kMinimumScale = 0.07f;
	static inline constexpr float kMaximumScale = 0.12f;
	static inline constexpr float kMinimumSpawnInterval = 0.45f;
	static inline constexpr float kMaximumSpawnInterval = 0.90f;
	static inline constexpr float kMinimumLifetime = 5.0f;
	static inline constexpr float kMaximumLifetime = 10.0f;
	static inline constexpr float kFadeDuration = 1.0f;
	static inline constexpr float kMinimumDepth = 4.0f;
	static inline constexpr float kMaximumDepth = 20.0f;
	static inline constexpr float kSpawnMargin = 0.86f;
	static inline constexpr float kDespawnMargin = 1.12f;
	static inline constexpr float kDeltaTime = 1.0f / 60.0f;

	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	std::array<Particle, kMaximumParticleCount> particles_ = {};
	std::mt19937 randomEngine_;
	float spawnTimer_ = 0.0f;
	float nextSpawnInterval_ = 0.0f;
};
