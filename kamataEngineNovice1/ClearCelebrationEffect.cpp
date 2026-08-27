#include "ClearCelebrationEffect.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>
#include <utility>

using namespace KamataEngine;

ClearCelebrationEffect::ClearCelebrationEffect()
    : randomEngine_(std::random_device{}()) {}

ClearCelebrationEffect::~ClearCelebrationEffect() = default;

void ClearCelebrationEffect::Initialize(uint32_t fireworksSfxSoundHandle) {
	fireworksSfxSoundHandle_ = fireworksSfxSoundHandle;
	textureHandle_ = TextureManager::Load("white1x1.png");
	particles_.reserve(kMaximumParticleCount);
}

void ClearCelebrationEffect::StartFireworks(
    float obstacleRetractionDuration) {
	areFireworksActive_ = true;
	fireworkElapsedTime_ = 0.0f;
	firstFireworkTime_ = obstacleRetractionDuration + kPostRetractionDelay;
	fireworkInterval_ = kFireworkInterval;
	firedFireworkCount_ = 0;
	areFireworksLooping_ = false;
}

void ClearCelebrationEffect::StartConfetti() {
	if (isConfettiActive_) {
		return;
	}
	isConfettiActive_ = true;
	isContinuousConfetti_ = false;
	confettiElapsedTime_ = 0.0f;
	confettiSpawnTimer_ = kConfettiSpawnInterval;
	confettiOpacity_ = 1.0f;
}

void ClearCelebrationEffect::StartDifficultySelectCelebration() {
	particles_.clear();
	areFireworksActive_ = true;
	areFireworksLooping_ = true;
	fireworkElapsedTime_ = 0.0f;
	firstFireworkTime_ = 0.0f;
	fireworkInterval_ = kSelectionFireworkInterval;
	firedFireworkCount_ = 0;

	isConfettiActive_ = true;
	isContinuousConfetti_ = true;
	confettiElapsedTime_ = 0.0f;
	confettiSpawnTimer_ = kSelectionConfettiSpawnInterval;
	confettiOpacity_ = 1.0f;
}

void ClearCelebrationEffect::StopDifficultySelectCelebration() {
	areFireworksActive_ = false;
	areFireworksLooping_ = false;
	isConfettiActive_ = false;
	isContinuousConfetti_ = false;
	confettiOpacity_ = 0.0f;
	particles_.clear();
}

void ClearCelebrationEffect::Update(float deltaTime) {
	if (areFireworksActive_) {
		fireworkElapsedTime_ += deltaTime;
		const float nextFireworkTime =
		    firstFireworkTime_ +
		    static_cast<float>(firedFireworkCount_) * fireworkInterval_;
		if (fireworkElapsedTime_ >= nextFireworkTime) {
			SpawnFirework();
			++firedFireworkCount_;
			if (
			    !areFireworksLooping_ &&
			    firedFireworkCount_ >= kFireworkBurstCount) {
				areFireworksActive_ = false;
			}
		}
	}

	if (isConfettiActive_) {
		confettiElapsedTime_ += deltaTime;
		if (isContinuousConfetti_) {
			confettiSpawnTimer_ += deltaTime;
			while (
			    confettiSpawnTimer_ >=
			    kSelectionConfettiSpawnInterval) {
				confettiSpawnTimer_ -=
				    kSelectionConfettiSpawnInterval;
				SpawnConfetti();
			}
		} else if (confettiElapsedTime_ <= kConfettiEmissionDuration) {
			confettiSpawnTimer_ += deltaTime;
			while (confettiSpawnTimer_ >= kConfettiSpawnInterval) {
				confettiSpawnTimer_ -= kConfettiSpawnInterval;
				SpawnConfetti();
				SpawnConfetti();
			}
		} else if (!isContinuousConfetti_) {
			const float fadeProgress = std::clamp(
			    (confettiElapsedTime_ - kConfettiEmissionDuration) /
			        kConfettiFadeDuration,
			    0.0f, 1.0f);
			confettiOpacity_ = 1.0f - fadeProgress;
		}
		if (
		    !isContinuousConfetti_ &&
		    confettiElapsedTime_ >=
		    kConfettiEmissionDuration + kConfettiFadeDuration) {
			isConfettiActive_ = false;
			confettiOpacity_ = 0.0f;
			std::erase_if(particles_, [](const Particle& particle) {
				return particle.type == ParticleType::Confetti;
			});
		}
	}

	for (Particle& particle : particles_) {
		particle.age += deltaTime;
		if (particle.type == ParticleType::Firework) {
			particle.velocity.y += 105.0f * deltaTime;
			const float damping = std::pow(0.985f, deltaTime * 60.0f);
			particle.velocity.x *= damping;
			particle.velocity.y *= damping;
		} else {
			particle.velocity.y += 22.0f * deltaTime;
			particle.position.x +=
			    std::sin(
			        particle.age * particle.flutterSpeed +
			        particle.flutterPhase) *
			    particle.flutterAmount * deltaTime;
		}
		particle.position.x += particle.velocity.x * deltaTime;
		particle.position.y += particle.velocity.y * deltaTime;
		particle.rotation += particle.rotationSpeed * deltaTime;
	}

	std::erase_if(particles_, [](const Particle& particle) {
		return particle.age >= particle.lifetime;
	});
}

void ClearCelebrationEffect::Draw() const {
	if (particles_.empty()) {
		return;
	}

	auto drawType = [this](ParticleType type, Sprite::BlendMode blendMode) {
		Sprite::PreDraw(nullptr, blendMode);
		for (const Particle& particle : particles_) {
			if (particle.type != type) {
				continue;
			}
			assert(particle.sprite);
			const float lifeProgress = std::clamp(
			    particle.age / particle.lifetime, 0.0f, 1.0f);
			float alpha = 1.0f - lifeProgress;
			if (type == ParticleType::Confetti) {
				const float fadeStart = 0.72f;
				alpha = 1.0f - std::clamp(
				    (lifeProgress - fadeStart) / (1.0f - fadeStart),
				    0.0f, 1.0f);
				alpha *= confettiOpacity_;
			}
			Vector4 drawColor = particle.color;
			drawColor.w = alpha;
			particle.sprite->SetPosition(particle.position);
			particle.sprite->SetSize(particle.size);
			particle.sprite->SetRotation(particle.rotation);
			particle.sprite->SetColor(drawColor);
			particle.sprite->Draw();
		}
		Sprite::PostDraw();
	};

	drawType(ParticleType::Firework, Sprite::BlendMode::kAdd);
	drawType(ParticleType::Confetti, Sprite::BlendMode::kNormal);
}

void ClearCelebrationEffect::SpawnFirework() {
	// Difficulty-select fireworks are decorative and intentionally silent.
	if (!areFireworksLooping_) {
		Audio::GetInstance()->PlayWave(
		    fireworksSfxSoundHandle_, false, 1.0f);
	}
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	const float screenWidth =
	    static_cast<float>(dxCommon->GetBackBufferWidth());
	const float screenHeight =
	    static_cast<float>(dxCommon->GetBackBufferHeight());
	const Vector2 center = {
	    RandomFloat(screenWidth * 0.18f, screenWidth * 0.82f),
	    RandomFloat(screenHeight * 0.16f, screenHeight * 0.46f),
	};
	const int particleCount = RandomInt(36, 48);
	const float angleOffset = RandomFloat(
	    0.0f, 2.0f * std::numbers::pi_v<float>);

	for (int index = 0; index < particleCount; ++index) {
		if (particles_.size() >= kMaximumParticleCount) {
			return;
		}
		Particle particle;
		particle.type = ParticleType::Firework;
		particle.position = center;
		const float angle =
		    angleOffset +
		    2.0f * std::numbers::pi_v<float> *
		        static_cast<float>(index) / static_cast<float>(particleCount) +
		    RandomFloat(-0.06f, 0.06f);
		const float speed = RandomFloat(140.0f, 240.0f);
		particle.velocity = {
		    std::cos(angle) * speed,
		    std::sin(angle) * speed,
		};
		const float size = RandomFloat(6.0f, 12.0f);
		particle.size = {size, size};
		particle.color = SelectRandomColor();
		particle.rotation = RandomFloat(
		    0.0f, 2.0f * std::numbers::pi_v<float>);
		particle.rotationSpeed = RandomFloat(-6.0f, 6.0f);
		particle.lifetime = RandomFloat(1.25f, 1.85f);
		particle.sprite.reset(Sprite::Create(
		    textureHandle_, particle.position, particle.color,
		    {0.5f, 0.5f}));
		assert(particle.sprite);
		particles_.push_back(std::move(particle));
	}
}

void ClearCelebrationEffect::SpawnConfetti() {
	if (particles_.size() >= kMaximumParticleCount) {
		return;
	}
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	const float screenWidth =
	    static_cast<float>(dxCommon->GetBackBufferWidth());

	Particle particle;
	particle.type = ParticleType::Confetti;
	particle.position = {RandomFloat(0.0f, screenWidth), -24.0f};
	particle.velocity = {
	    RandomFloat(-42.0f, 42.0f), RandomFloat(85.0f, 155.0f)};
	particle.size = {
	    RandomFloat(7.0f, 12.0f), RandomFloat(20.0f, 36.0f)};
	particle.color = SelectRandomColor();
	particle.rotation = RandomFloat(
	    0.0f, 2.0f * std::numbers::pi_v<float>);
	particle.rotationSpeed = RandomFloat(-5.0f, 5.0f);
	particle.lifetime = RandomFloat(5.0f, 7.0f);
	particle.flutterPhase = RandomFloat(
	    0.0f, 2.0f * std::numbers::pi_v<float>);
	particle.flutterSpeed = RandomFloat(5.0f, 10.0f);
	particle.flutterAmount = RandomFloat(16.0f, 42.0f);
	particle.sprite.reset(Sprite::Create(
	    textureHandle_, particle.position, particle.color,
	    {0.5f, 0.5f}));
	assert(particle.sprite);
	particles_.push_back(std::move(particle));
}

Vector4 ClearCelebrationEffect::SelectRandomColor() {
	static constexpr std::array<Vector4, 7> colors = {{
	    {1.00f, 0.22f, 0.18f, 1.0f},
	    {1.00f, 0.78f, 0.12f, 1.0f},
	    {0.18f, 0.72f, 1.00f, 1.0f},
	    {0.25f, 1.00f, 0.42f, 1.0f},
	    {0.82f, 0.30f, 1.00f, 1.0f},
	    {1.00f, 0.38f, 0.72f, 1.0f},
	    {1.00f, 1.00f, 1.00f, 1.0f},
	}};
	return colors[static_cast<std::size_t>(
	    RandomInt(0, static_cast<int>(colors.size()) - 1))];
}

float ClearCelebrationEffect::RandomFloat(float minimum, float maximum) {
	return std::uniform_real_distribution<float>(minimum, maximum)(
	    randomEngine_);
}

int ClearCelebrationEffect::RandomInt(int minimum, int maximum) {
	return std::uniform_int_distribution<int>(minimum, maximum)(randomEngine_);
}
