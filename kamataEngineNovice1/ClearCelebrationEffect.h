#pragma once

#include "KamataEngine.h"

#include <memory>
#include <random>
#include <vector>

// Simple screen-space fireworks and confetti used during the clear sequence.
class ClearCelebrationEffect {
public:
	ClearCelebrationEffect();
	~ClearCelebrationEffect();

	void Initialize(uint32_t fireworksSfxSoundHandle);
	void StartFireworks(float obstacleRetractionDuration);
	void StartConfetti();
	void StartDifficultySelectCelebration();
	void StopDifficultySelectCelebration();
	void Update(float deltaTime);
	void Draw() const;

private:
	enum class ParticleType {
		Firework,
		Confetti,
	};

	struct Particle {
		std::unique_ptr<KamataEngine::Sprite> sprite;
		ParticleType type = ParticleType::Firework;
		KamataEngine::Vector2 position = {};
		KamataEngine::Vector2 velocity = {};
		KamataEngine::Vector2 size = {};
		KamataEngine::Vector4 color = {};
		float rotation = 0.0f;
		float rotationSpeed = 0.0f;
		float age = 0.0f;
		float lifetime = 1.0f;
		float flutterPhase = 0.0f;
		float flutterSpeed = 0.0f;
		float flutterAmount = 0.0f;
	};

	void SpawnFirework();
	void SpawnConfetti();
	KamataEngine::Vector4 SelectRandomColor();
	float RandomFloat(float minimum, float maximum);
	int RandomInt(int minimum, int maximum);

	static inline constexpr int kFireworkBurstCount = 3;
	static inline constexpr float kPostRetractionDelay = 2.0f;
	static inline constexpr float kFireworkInterval = 1.0f;
	static inline constexpr float kConfettiEmissionDuration = 5.0f;
	static inline constexpr float kConfettiFadeDuration = 1.5f;
	static inline constexpr float kConfettiSpawnInterval = 0.10f;
	static inline constexpr float kSelectionFireworkInterval = 2.5f;
	static inline constexpr float kSelectionConfettiSpawnInterval = 0.35f;
	static inline constexpr std::size_t kMaximumParticleCount = 220;

	uint32_t textureHandle_ = 0;
	uint32_t fireworksSfxSoundHandle_ = 0;
	std::vector<Particle> particles_;
	std::mt19937 randomEngine_;
	float fireworkElapsedTime_ = 0.0f;
	float firstFireworkTime_ = 0.0f;
	float confettiElapsedTime_ = 0.0f;
	float confettiSpawnTimer_ = 0.0f;
	float confettiOpacity_ = 0.0f;
	float fireworkInterval_ = kFireworkInterval;
	int firedFireworkCount_ = 0;
	bool areFireworksActive_ = false;
	bool areFireworksLooping_ = false;
	bool isConfettiActive_ = false;
	bool isContinuousConfetti_ = false;
};
