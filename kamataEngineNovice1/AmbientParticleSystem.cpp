#include "AmbientParticleSystem.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
float Dot(const Vector3& left, const Vector3& right) {
	return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vector3 Add(const Vector3& left, const Vector3& right) {
	return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vector3 Subtract(const Vector3& left, const Vector3& right) {
	return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vector3 Scale(const Vector3& value, float scale) {
	return {value.x * scale, value.y * scale, value.z * scale};
}
} // namespace

AmbientParticleSystem::AmbientParticleSystem()
    : randomEngine_(std::random_device{}()) {}

AmbientParticleSystem::~AmbientParticleSystem() { delete model_; }

void AmbientParticleSystem::Initialize() {
	model_ = Model::CreateFromOBJ("particle", false);
	assert(model_);
	textureHandle_ = TextureManager::Load("particle/particle.png");
	// Keep the bright particle texture independent from scene directional
	// lighting. The exported MTL used a very dark diffuse coefficient, which
	// made randomly rotated faces appear almost black.
	for (const std::unique_ptr<Mesh>& mesh : model_->GetMeshes()) {
		Material* material = mesh->GetMaterial();
		if (!material) {
			continue;
		}
		material->ambient_ = {1.0f, 1.0f, 1.0f};
		material->diffuse_ = {0.0f, 0.0f, 0.0f};
		material->specular_ = {0.0f, 0.0f, 0.0f};
		material->Update();
	}

	for (Particle& particle : particles_) {
		particle.worldTransform.Initialize();
		particle.color.Initialize();
		particle.color.SetColor({1.0f, 1.0f, 1.0f, 0.0f});
		particle.isActive = false;
	}
	spawnTimer_ = 0.0f;
	nextSpawnInterval_ = 0.0f;
}

void AmbientParticleSystem::Update(const Camera& camera) {
	spawnTimer_ += kDeltaTime;
	if (spawnTimer_ >= nextSpawnInterval_) {
		for (Particle& particle : particles_) {
			if (!particle.isActive) {
				RespawnParticle(particle, camera, false);
				break;
			}
		}
		spawnTimer_ = 0.0f;
		nextSpawnInterval_ = RandomFloat(
		    kMinimumSpawnInterval, kMaximumSpawnInterval);
	}

	for (Particle& particle : particles_) {
		if (!particle.isActive) {
			continue;
		}

		particle.age += kDeltaTime;
		const float fadeProgress = std::clamp(
		    (particle.age - particle.visibleLifetime) / kFadeDuration,
		    0.0f, 1.0f);
		particle.color.SetColor(
		    {1.0f, 1.0f, 1.0f, 1.0f - fadeProgress});
		if (fadeProgress >= 1.0f) {
			particle.isActive = false;
			continue;
		}

		particle.driftPhase += particle.driftSpeed * kDeltaTime;
		const float driftVelocity =
		    std::sin(particle.driftPhase) * particle.driftAmount;
		const Vector3 frameVelocity = Add(
		    particle.velocity, Scale(particle.driftAxis, driftVelocity));
		particle.worldTransform.translation_ = Add(
		    particle.worldTransform.translation_,
		    Scale(frameVelocity, kDeltaTime));
		particle.worldTransform.rotation_.x +=
		    particle.angularVelocity.x * kDeltaTime;
		particle.worldTransform.rotation_.y +=
		    particle.angularVelocity.y * kDeltaTime;
		particle.worldTransform.rotation_.z +=
		    particle.angularVelocity.z * kDeltaTime;

		if (!IsInsideSpawnVolume(particle, camera)) {
			particle.isActive = false;
		} else {
			WorldTransformUpdate(particle.worldTransform);
		}
	}
}

void AmbientParticleSystem::Draw(const Camera& camera) const {
	if (!model_) {
		return;
	}
	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kReadOnly);
	for (const Particle& particle : particles_) {
		if (!particle.isActive) {
			continue;
		}
		model_->Draw(
		    particle.worldTransform, camera, textureHandle_, &particle.color);
	}
	Model::PostDraw();
}

void AmbientParticleSystem::RespawnParticle(
    Particle& particle, const Camera& camera, bool initializeTransform) {
	const CameraBasis basis = CalculateCameraBasis(camera);
	const float depth = RandomFloat(kMinimumDepth, kMaximumDepth);
	const float halfHeight =
	    std::tan(camera.fovAngleY * 0.5f) * depth * kSpawnMargin;
	const float halfWidth = halfHeight * camera.aspectRatio;
	const float localX = RandomFloat(-halfWidth, halfWidth);
	const float localY = RandomFloat(-halfHeight, halfHeight);

	particle.worldTransform.translation_ = Add(
	    camera.translation_,
	    Add(
	        Scale(basis.right, localX),
	        Add(Scale(basis.up, localY), Scale(basis.forward, depth))));

	const float scale = RandomFloat(kMinimumScale, kMaximumScale);
	particle.worldTransform.scale_ = {scale, scale, scale};
	particle.worldTransform.rotation_ = {
	    RandomFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
	    RandomFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
	    RandomFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
	};

	const Vector3 localVelocity = {
	    RandomFloat(-0.32f, 0.32f),
	    RandomFloat(0.04f, 0.30f),
	    RandomFloat(-0.12f, 0.12f),
	};
	particle.velocity = Add(
	    Scale(basis.right, localVelocity.x),
	    Add(
	        Scale(basis.up, localVelocity.y),
	        Scale(basis.forward, localVelocity.z)));
	particle.angularVelocity = {
	    RandomFloat(-1.8f, 1.8f),
	    RandomFloat(-1.8f, 1.8f),
	    RandomFloat(-1.8f, 1.8f),
	};
	particle.driftAxis = basis.right;
	particle.driftPhase = RandomFloat(
	    0.0f, 2.0f * std::numbers::pi_v<float>);
	particle.driftSpeed = RandomFloat(0.55f, 1.65f);
	particle.driftAmount = RandomFloat(0.04f, 0.20f);
	particle.age = 0.0f;
	particle.visibleLifetime = RandomFloat(
	    kMinimumLifetime, kMaximumLifetime);
	particle.isActive = true;
	particle.color.SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	if (initializeTransform) {
		particle.worldTransform.parent_ = nullptr;
	}
	WorldTransformUpdate(particle.worldTransform);
}

bool AmbientParticleSystem::IsInsideSpawnVolume(
    const Particle& particle, const Camera& camera) const {
	const CameraBasis basis = CalculateCameraBasis(camera);
	const Vector3 relative = Subtract(
	    particle.worldTransform.translation_, camera.translation_);
	const float depth = Dot(relative, basis.forward);
	if (depth < kMinimumDepth * 0.75f || depth > kMaximumDepth * 1.15f) {
		return false;
	}

	const float halfHeight =
	    std::tan(camera.fovAngleY * 0.5f) * depth * kDespawnMargin;
	const float halfWidth = halfHeight * camera.aspectRatio;
	return std::abs(Dot(relative, basis.right)) <= halfWidth &&
	       std::abs(Dot(relative, basis.up)) <= halfHeight;
}

AmbientParticleSystem::CameraBasis
AmbientParticleSystem::CalculateCameraBasis(const Camera& camera) const {
	const float sinPitch = std::sin(camera.rotation_.x);
	const float cosPitch = std::cos(camera.rotation_.x);
	const float sinYaw = std::sin(camera.rotation_.y);
	const float cosYaw = std::cos(camera.rotation_.y);
	return {
	    {cosYaw, 0.0f, -sinYaw},
	    {sinPitch * sinYaw, cosPitch, sinPitch * cosYaw},
	    {cosPitch * sinYaw, -sinPitch, cosPitch * cosYaw},
	};
}

float AmbientParticleSystem::RandomFloat(float minimum, float maximum) {
	return std::uniform_real_distribution<float>(minimum, maximum)(
	    randomEngine_);
}
