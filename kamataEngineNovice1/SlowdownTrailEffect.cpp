#include "SlowdownTrailEffect.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

SlowdownTrailEffect::SlowdownTrailEffect()
    : randomEngine_(std::random_device{}()) {}

SlowdownTrailEffect::~SlowdownTrailEffect() { delete model_; }

void SlowdownTrailEffect::Initialize() {
	delete model_;
	model_ = Model::CreateFromOBJ("hit_effect", false);
	assert(model_);
	textureHandle_ = TextureManager::Load("white1x1.png");

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

	for (Fragment& fragment : fragments_) {
		fragment.worldTransform.Initialize();
		fragment.color.Initialize();
		fragment.color.SetColor({0.2f, 1.0f, 0.32f, 0.0f});
		fragment.isActive = false;
	}
	spawnTimer_ = 0.0f;
	wasEmitting_ = false;
}

void SlowdownTrailEffect::Update(
    bool shouldEmit, const Vector3& playerPosition, float deltaTime) {
	if (shouldEmit) {
		if (!wasEmitting_) {
			spawnTimer_ = kSpawnInterval;
		}
		spawnTimer_ += deltaTime;
		while (spawnTimer_ >= kSpawnInterval) {
			SpawnFragment(playerPosition);
			spawnTimer_ -= kSpawnInterval;
		}
	} else {
		spawnTimer_ = 0.0f;
	}
	wasEmitting_ = shouldEmit;

	for (Fragment& fragment : fragments_) {
		if (!fragment.isActive) {
			continue;
		}

		fragment.age += deltaTime;
		if (fragment.age >= fragment.lifetime) {
			fragment.isActive = false;
			fragment.color.SetColor({0.2f, 1.0f, 0.32f, 0.0f});
			continue;
		}

		fragment.velocity.y -= kGravity * deltaTime;
		fragment.worldTransform.translation_.x +=
		    fragment.velocity.x * deltaTime;
		fragment.worldTransform.translation_.y +=
		    fragment.velocity.y * deltaTime;
		fragment.worldTransform.translation_.z +=
		    fragment.velocity.z * deltaTime;
		fragment.worldTransform.rotation_.x +=
		    fragment.angularVelocity.x * deltaTime;
		fragment.worldTransform.rotation_.y +=
		    fragment.angularVelocity.y * deltaTime;
		fragment.worldTransform.rotation_.z +=
		    fragment.angularVelocity.z * deltaTime;

		const float fadeStart = fragment.lifetime * 0.45f;
		const float opacity = 1.0f - std::clamp(
		    (fragment.age - fadeStart) /
		        (fragment.lifetime - fadeStart),
		    0.0f, 1.0f);
		fragment.color.SetColor({0.2f, 1.0f, 0.32f, opacity});
		WorldTransformUpdate(fragment.worldTransform);
	}
}

void SlowdownTrailEffect::Draw(const Camera& camera) const {
	if (!model_) {
		return;
	}

	Model::PreDraw(
	    Model::CullingMode::kNone, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kReadOnly);
	for (const Fragment& fragment : fragments_) {
		if (!fragment.isActive) {
			continue;
		}
		model_->Draw(
		    fragment.worldTransform, camera, textureHandle_, &fragment.color);
	}
	Model::PostDraw();
}

void SlowdownTrailEffect::SpawnFragment(const Vector3& playerPosition) {
	Fragment* fragment = nullptr;
	for (Fragment& candidate : fragments_) {
		if (!candidate.isActive) {
			fragment = &candidate;
			break;
		}
	}
	if (!fragment) {
		return;
	}

	const float width = RandomFloat(0.035f, 0.070f);
	const float length = RandomFloat(0.090f, 0.180f);
	fragment->worldTransform.scale_ = {1.0f, width, length};
	fragment->worldTransform.rotation_ = {
	    RandomFloat(-0.55f, 0.55f),
	    RandomFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
	    RandomFloat(-0.55f, 0.55f),
	};
	fragment->worldTransform.translation_ = {
	    playerPosition.x + RandomFloat(-0.36f, -0.10f),
	    playerPosition.y - 0.43f + RandomFloat(-0.04f, 0.05f),
	    playerPosition.z + RandomFloat(-0.28f, 0.28f),
	};
	fragment->velocity = {
	    RandomFloat(-1.15f, -0.55f),
	    RandomFloat(0.08f, 0.32f),
	    RandomFloat(-0.24f, 0.24f),
	};
	fragment->angularVelocity = {
	    RandomFloat(-4.0f, 4.0f),
	    RandomFloat(-5.0f, 5.0f),
	    RandomFloat(-4.0f, 4.0f),
	};
	fragment->age = 0.0f;
	fragment->lifetime = RandomFloat(0.55f, 0.95f);
	fragment->isActive = true;
	fragment->color.SetColor({0.2f, 1.0f, 0.32f, 1.0f});
	WorldTransformUpdate(fragment->worldTransform);
}

float SlowdownTrailEffect::RandomFloat(float minimum, float maximum) {
	return std::uniform_real_distribution<float>(minimum, maximum)(
	    randomEngine_);
}
