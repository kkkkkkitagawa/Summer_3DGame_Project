#include "GameScene.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numbers>

using namespace KamataEngine;

namespace {
bool isDebugTextInitialized = false;

bool IsModifierKey(BYTE key) {
	return key == DIK_LSHIFT || key == DIK_RSHIFT || key == DIK_LCONTROL ||
	       key == DIK_RCONTROL || key == DIK_LALT || key == DIK_RALT;
}

bool ProjectWorldToScreen(
    const Vector3& worldPosition, const Camera& camera, Vector2& screenPosition) {
	const Vector4 world = {
	    worldPosition.x, worldPosition.y, worldPosition.z, 1.0f};
	Vector4 view = {};
	Vector4 clip = {};

	for (int column = 0; column < 4; ++column) {
		const float value =
		    world.x * camera.matView.m[0][column] +
		    world.y * camera.matView.m[1][column] +
		    world.z * camera.matView.m[2][column] +
		    world.w * camera.matView.m[3][column];
		switch (column) {
		case 0:
			view.x = value;
			break;
		case 1:
			view.y = value;
			break;
		case 2:
			view.z = value;
			break;
		default:
			view.w = value;
			break;
		}
	}

	for (int column = 0; column < 4; ++column) {
		const float value =
		    view.x * camera.matProjection.m[0][column] +
		    view.y * camera.matProjection.m[1][column] +
		    view.z * camera.matProjection.m[2][column] +
		    view.w * camera.matProjection.m[3][column];
		switch (column) {
		case 0:
			clip.x = value;
			break;
		case 1:
			clip.y = value;
			break;
		case 2:
			clip.z = value;
			break;
		default:
			clip.w = value;
			break;
		}
	}

	if (clip.w <= 0.0f) {
		return false;
	}

	const float ndcX = clip.x / clip.w;
	const float ndcY = clip.y / clip.w;
	const float ndcZ = clip.z / clip.w;
	if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f ||
	    ndcZ < 0.0f || ndcZ > 1.0f) {
		return false;
	}

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	const float width = static_cast<float>(dxCommon->GetBackBufferWidth());
	const float height = static_cast<float>(dxCommon->GetBackBufferHeight());
	screenPosition = {
	    (ndcX + 1.0f) * 0.5f * width,
	    (1.0f - ndcY) * 0.5f * height,
	};
	return true;
}
} // namespace

GameScene::~GameScene() {
	mapBlocks_.clear();
	detachedObstacles_.clear();
	delete player_;
	delete modelPlayer_;
	delete modelObstacle_;
	delete skydome_;
	delete modelSkydome_;
	delete modelBlock_;
	delete modelAxis_;
	delete mouseCircleSprite_;
}

void GameScene::Initialize() {
	mapMoveSpeed_ = kInitialMapMoveSpeed;

	modelSkydome_ = Model::CreateFromOBJ("SkyDome", true);
	assert(modelSkydome_);
	skydome_ = new Skydome();
	skydome_->Initialize(modelSkydome_);

	modelBlock_ = Model::CreateFromOBJ("block", true);
	assert(modelBlock_);
	InitializeMapBlocks();

	modelPlayer_ = Model::CreateFromOBJ("playerTest", true);
	assert(modelPlayer_);
	player_ = new Player();
	const Vector3 playerPosition = {
	    sceneMap_.origin.x,
	    sceneMap_.groundHeight + kBlockSize * 0.5f +
	        Player::kCollisionHalfSize.y + kPlayerGroundClearance,
	    sceneMap_.origin.z,
	};
	player_->Initialize(modelPlayer_, playerPosition);

	modelObstacle_ = Model::CreateFromOBJ("cube", true);
	assert(modelObstacle_);

	modelAxis_ = Model::CreateFromOBJ("axis", true);
	assert(modelAxis_);

	worldTransformAxis_.Initialize();
	worldTransformAxis_.scale_ = {0.60f, 0.60f, 0.60f};
	worldTransformAxis_.translation_ = {0.0f, 0.0f, 0.0f};
	WorldTransformUpdate(worldTransformAxis_);

	playerCamera_.farZ = 1000.0f;
	playerCamera_.Initialize();
	playerCamera_.translation_ = {-3.395f, 5.308f, -6.101f};
	playerCamera_.rotation_ = {0.625002f, 0.419926f, 0.0f};
	playerCamera_.UpdateMatrix();

	debugCamera_.farZ = 1000.0f;
	debugCamera_.Initialize();
	debugCameraTarget_ = {
	    sceneMap_.origin.x,
	    sceneMap_.groundHeight,
	    sceneMap_.origin.z,
	};
	UpdateCamera();

	axisIndicatorCamera_.aspectRatio = 1.0f;
	axisIndicatorCamera_.farZ = 100.0f;
	axisIndicatorCamera_.Initialize();
	UpdateAxisIndicatorCamera();

	primitiveDrawer_ = PrimitiveDrawer::GetInstance();
	primitiveDrawer_->SetCamera(&GetActiveCamera());

	DebugText* debugText = DebugText::GetInstance();
	assert(debugText);
	if (!isDebugTextInitialized) {
		debugText->Initialize();
		isDebugTextInitialized = true;
	}

	uint32_t whiteTextureHandle = TextureManager::Load("white1x1.png");
	mouseCircleSprite_ = Sprite::Create(
	    whiteTextureHandle, {0.0f, 0.0f}, {0.85f, 0.76f, 1.0f, 0.90f});
	assert(mouseCircleSprite_);

	AxisIndicator::SetVisible(false);
}

void GameScene::Update() {
	skydome_->Update();
	player_->Update(
	    kInitialMapMoveSpeed, sceneMap_.origin.x, kDeltaTime);
	UpdateMapRotationInput();
	UpdateMapBlocks();
	ResolvePlayerObstacleCollisions();
	UpdateDebugCommand();
	UpdateCamera();
}

void GameScene::InitializeMapBlocks() {
	mapBlocks_.clear();
	mapBlocks_.reserve(kMapBlockCount);
	for (std::size_t index = 0; index < kMapBlockCount; ++index) {
		SpawnMapBlock(
		    sceneMap_.origin.x + static_cast<float>(index) * kBlockSize, false);
	}
}

void GameScene::SpawnMapBlock(float positionX, bool canSpawnObstacle) {
	auto block = std::make_unique<MapBlock>();
	block->positionX = positionX;
	block->positionY = sceneMap_.groundHeight;
	block->worldTransform.Initialize();
	block->worldTransform.scale_ = {kBlockSize, kBlockSize, kBlockSize};
	block->rotationX =
	    static_cast<float>(mapRotationQuarterTurns_) *
	    std::numbers::pi_v<float> * 0.5f;
	block->targetRotationX = block->rotationX;
	block->collisionRotationX = block->rotationX;
	block->worldTransform.rotation_.x = block->rotationX;
	block->worldTransform.translation_ = {
	    block->positionX, block->positionY, sceneMap_.origin.z};
	WorldTransformUpdate(block->worldTransform);
	if (canSpawnObstacle && !hasSpawnedFirstObstacle_) {
		AttachObstacle(
		    *block, modelObstacle_, FindPlayerFacingBlockFace(*block),
		    kObstacleSize);
		hasSpawnedFirstObstacle_ = true;
	}
	mapBlocks_.push_back(std::move(block));
}

void GameScene::AttachObstacle(
    MapBlock& block, Model* model, BlockFace attachedFace,
    const Vector3& size) {
	auto obstacle = std::make_unique<Obstacle>();
	obstacle->Initialize(
	    model, attachedFace, &block.worldTransform, kBlockSize * 0.5f, size);
	block.obstacles.push_back(std::move(obstacle));
}

BlockFace GameScene::FindPlayerFacingBlockFace(const MapBlock& block) const {
	constexpr BlockFace faces[] = {
	    BlockFace::Top,
	    BlockFace::Bottom,
	    BlockFace::Front,
	    BlockFace::Back,
	};
	constexpr Vector3 localNormals[] = {
	    {0.0f, 1.0f, 0.0f},
	    {0.0f, -1.0f, 0.0f},
	    {0.0f, 0.0f, 1.0f},
	    {0.0f, 0.0f, -1.0f},
	};

	BlockFace playerFacingFace = faces[0];
	float highestWorldY = -1.0f;
	for (std::size_t index = 0; index < std::size(faces); ++index) {
		Vector3 worldNormal = MathUtility::TransformNormal(
		    localNormals[index], block.worldTransform.matWorld_);
		MathUtility::Normalize(worldNormal);
		if (worldNormal.y > highestWorldY) {
			highestWorldY = worldNormal.y;
			playerFacingFace = faces[index];
		}
	}
	return playerFacingFace;
}

Matrix4x4 GameScene::CreateMapBlockLogicalTransform(
    const MapBlock& block, float rotationX) const {
	Matrix4x4 logicalTransform =
	    MathUtility::MakeScaleMatrix(block.worldTransform.scale_);
	logicalTransform = MathUtility::operator*(
	    logicalTransform, MathUtility::MakeRotateXMatrix(rotationX));
	logicalTransform = MathUtility::operator*(
	    logicalTransform,
	    MathUtility::MakeRotateYMatrix(block.worldTransform.rotation_.y));
	logicalTransform = MathUtility::operator*(
	    logicalTransform,
	    MathUtility::MakeRotateZMatrix(block.worldTransform.rotation_.z));
	return MathUtility::operator*(
	    logicalTransform,
	    MathUtility::MakeTranslateMatrix(block.worldTransform.translation_));
}

AABB GameScene::GetObstacleLogicalAABB(
    const MapBlock& block, const Obstacle& obstacle, float rotationX) const {
	return obstacle.GetAABBForParentTransform(
	    CreateMapBlockLogicalTransform(block, rotationX));
}

void GameScene::UpdateMapRotationInput() {
	Input* input = Input::GetInstance();
	const bool rotateLeft = input->TriggerKey(DIK_A);
	const bool rotateRight = input->TriggerKey(DIK_D);
	if (rotateLeft == rotateRight) {
		return;
	}

	const int turnDirection = rotateLeft ? 1 : -1;
	if (IsMapRotationBlocked(turnDirection)) {
		StartBlockedRotationFeedback(turnDirection);
		return;
	}

	blockedRotationFeedbackTime_ = 0.0f;
	blockedRotationFeedbackDirection_ = 0;
	mapRotationQuarterTurns_ += turnDirection;
	const float rotationAmount =
	    static_cast<float>(turnDirection) *
	    std::numbers::pi_v<float> * 0.5f;

	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		if (!block->isFalling) {
			block->targetRotationX += rotationAmount;
		}
	}
}

bool GameScene::IsMapRotationBlocked(int turnDirection) const {
	const AABB playerAABB = player_->GetAABB();
	const float rotationAmount =
	    static_cast<float>(turnDirection) *
	    std::numbers::pi_v<float> * 0.5f;

	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		if (block->isFalling) {
			continue;
		}

		for (const std::unique_ptr<Obstacle>& obstacle : block->obstacles) {
			const ObstacleInteractionRules& rules =
			    obstacle->GetInteractionRules();
			if (!obstacle->IsCollisionEnabled() ||
			    !rules.blocksRotationFromSide) {
				continue;
			}

			const ObstacleSurfaceRelation currentRelation =
			    obstacle->GetSurfaceRelation(
			        block->targetRotationX, kPlayerSurfaceNormal);
			if (currentRelation != ObstacleSurfaceRelation::PositiveSide &&
			    currentRelation != ObstacleSurfaceRelation::NegativeSide) {
				continue;
			}

			const AABB obstacleAABB = GetObstacleLogicalAABB(
			    *block, *obstacle, block->targetRotationX);
			const bool overlapsPlayerX =
			    obstacleAABB.min.x <= playerAABB.max.x &&
			    obstacleAABB.max.x >= playerAABB.min.x;
			if (!overlapsPlayerX) {
				continue;
			}

			const ObstacleSurfaceRelation proposedRelation =
			    obstacle->GetSurfaceRelation(
			        block->targetRotationX + rotationAmount,
			        kPlayerSurfaceNormal);
			if (proposedRelation == ObstacleSurfaceRelation::PlayerFace) {
				return true;
			}
		}
	}
	return false;
}

void GameScene::StartBlockedRotationFeedback(int turnDirection) {
	blockedRotationFeedbackTime_ = kBlockedRotationFeedbackDuration;
	blockedRotationFeedbackDirection_ = turnDirection;
}

float GameScene::CalculateBlockedRotationOffset() const {
	if (blockedRotationFeedbackTime_ <= 0.0f) {
		return 0.0f;
	}

	const float progress = 1.0f -
	                       blockedRotationFeedbackTime_ /
	                           kBlockedRotationFeedbackDuration;
	return static_cast<float>(blockedRotationFeedbackDirection_) *
	       kBlockedRotationFeedbackAngle *
	       std::sin(std::numbers::pi_v<float> * progress);
}

void GameScene::UpdateMapBlocks() {
	UpdateDetachedObstacles();
	const float blockedRotationOffset = CalculateBlockedRotationOffset();

	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		block->positionX -= mapMoveSpeed_ * kDeltaTime;

		bool startedFalling = false;
		if (!block->isFalling && block->positionX < kFallStartX) {
			block->isFalling = true;
			startedFalling = true;
			block->verticalVelocity = 0.0f;
			block->targetRotationX = block->rotationX;
		}

		if (!block->isFalling) {
			const float rotationStep =
			    (std::numbers::pi_v<float> * 0.5f / kMapRotationDuration) *
			    kDeltaTime;
			const float rotationDifference =
			    block->targetRotationX - block->rotationX;
			block->rotationX += std::clamp(
			    rotationDifference, -rotationStep, rotationStep);

			const float quarterTurn = std::numbers::pi_v<float> * 0.5f;
			while (
			    block->collisionRotationX + quarterTurn <=
			        block->targetRotationX + 0.0001f &&
			    block->collisionRotationX + quarterTurn <=
			        block->rotationX + 0.0001f) {
				block->collisionRotationX += quarterTurn;
			}
			while (
			    block->collisionRotationX - quarterTurn >=
			        block->targetRotationX - 0.0001f &&
			    block->collisionRotationX - quarterTurn >=
			        block->rotationX - 0.0001f) {
				block->collisionRotationX -= quarterTurn;
			}
		}

		if (block->isFalling) {
			block->verticalVelocity -= kGravity * kDeltaTime;
			block->positionY += block->verticalVelocity * kDeltaTime;
			block->worldTransform.translation_ = {
			    block->positionX, block->positionY, sceneMap_.origin.z};
			block->worldTransform.rotation_.x = block->rotationX;
			block->worldTransform.rotation_.z = 0.0f;
		} else if (block->positionX < kShakeStartX) {
			block->shakeTime += kDeltaTime;
			const float shakeStrength = CalculateShakeStrength(block->positionX);
			const float shakeAmount = kShakeAmount * shakeStrength;
			const float shakeX =
			    std::sin(block->shakeTime * kShakeFrequency) * shakeAmount;
			const float shakeY =
			    std::cos(block->shakeTime * kShakeFrequency * 1.37f) * shakeAmount;
			block->worldTransform.translation_ = {
			    block->positionX + shakeX,
			    block->positionY + shakeY,
			    sceneMap_.origin.z};
			block->worldTransform.rotation_.x =
			    block->rotationX + blockedRotationOffset;
			block->worldTransform.rotation_.z = shakeX * 0.35f;
		} else {
			block->worldTransform.translation_ = {
			    block->positionX, block->positionY, sceneMap_.origin.z};
			block->worldTransform.rotation_.x =
			    block->rotationX + blockedRotationOffset;
			block->worldTransform.rotation_.z = 0.0f;
		}

		WorldTransformUpdate(block->worldTransform);
		for (std::unique_ptr<Obstacle>& obstacle : block->obstacles) {
			obstacle->Update(kDeltaTime, kGravity);
			if (startedFalling) {
				obstacle->DetachAndFall(
				    {-mapMoveSpeed_, block->verticalVelocity, 0.0f},
				    kObstacleDetachRepulsionSpeed);
				detachedObstacles_.push_back(std::move(obstacle));
			}
		}
		if (startedFalling) {
			block->obstacles.clear();
		}
	}
	blockedRotationFeedbackTime_ = (std::max)(
	    0.0f, blockedRotationFeedbackTime_ - kDeltaTime);

	const float deleteY = sceneMap_.groundHeight - kDeleteDistance;
	const std::size_t oldCount = mapBlocks_.size();
	mapBlocks_.erase(
	    std::remove_if(
	        mapBlocks_.begin(), mapBlocks_.end(),
	        [deleteY](const std::unique_ptr<MapBlock>& block) {
		        return block->isFalling && block->positionY < deleteY;
	        }),
	    mapBlocks_.end());

	const std::size_t removedCount = oldCount - mapBlocks_.size();
	for (std::size_t index = 0; index < removedCount; ++index) {
		float frontX = sceneMap_.origin.x - kBlockSize;
		for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
			frontX = (std::max)(frontX, block->positionX);
		}
		SpawnMapBlock(frontX + kBlockSize, true);
	}
}

void GameScene::UpdateDetachedObstacles() {
	for (const std::unique_ptr<Obstacle>& obstacle : detachedObstacles_) {
		obstacle->Update(kDeltaTime, kGravity);
	}

	const float deleteY = sceneMap_.groundHeight - kDeleteDistance;
	detachedObstacles_.erase(
	    std::remove_if(
	        detachedObstacles_.begin(), detachedObstacles_.end(),
	        [deleteY](const std::unique_ptr<Obstacle>& obstacle) {
		        return obstacle->GetWorldPosition().y < deleteY;
	        }),
	    detachedObstacles_.end());
}

void GameScene::ResolvePlayerObstacleCollisions() {
	AABB playerAABB = player_->GetAABB();
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		for (const std::unique_ptr<Obstacle>& obstacle : block->obstacles) {
			const ObstacleInteractionRules& rules =
			    obstacle->GetInteractionRules();
			if (!obstacle->IsCollisionEnabled() ||
			    !rules.pushesPlayerOnPlayerFace) {
				continue;
			}
			if (obstacle->GetSurfaceRelation(
			        block->collisionRotationX, kPlayerSurfaceNormal) !=
			    ObstacleSurfaceRelation::PlayerFace) {
				continue;
			}

			const AABB obstacleAABB = GetObstacleLogicalAABB(
			    *block, *obstacle, block->collisionRotationX);
			const float obstacleCenterX =
			    (obstacleAABB.min.x + obstacleAABB.max.x) * 0.5f;
			if (!IsCollision(playerAABB, obstacleAABB) ||
			    obstacleCenterX < player_->GetWorldPosition().x) {
				continue;
			}

			player_->SetPositionX(
			    obstacleAABB.min.x - Player::kCollisionHalfSize.x);
			playerAABB = player_->GetAABB();
		}
	}
}

float GameScene::CalculateShakeStrength(float positionX) const {
	const float shakeRange = kShakeStartX - kFallStartX;
	const float progress = std::clamp(
	    (kShakeStartX - positionX) / shakeRange, 0.0f, 1.0f);
	return kMinShakeStrength +
	       (kMaxShakeStrength - kMinShakeStrength) * progress;
}

void GameScene::UpdateDebugCommand() {
	Input* input = Input::GetInstance();
	const std::array<BYTE, 5>& command =
	    isDebugMode_ ? kExitDebugCommand : kEnterDebugCommand;
	const BYTE expectedKey = command[debugCommandIndex_];
	if (input->TriggerKey(expectedKey)) {
		++debugCommandIndex_;
		if (debugCommandIndex_ == command.size()) {
			isDebugMode_ = !isDebugMode_;
			debugCommandIndex_ = 0;
		}
		return;
	}

	for (uint16_t keyIndex = 0; keyIndex < 256; ++keyIndex) {
		const BYTE key = static_cast<BYTE>(keyIndex);
		if (IsModifierKey(key) || !input->TriggerKey(key)) {
			continue;
		}

		debugCommandIndex_ = key == command.front() ? 1 : 0;
		break;
	}
}

void GameScene::DrawDebugInfo() {
	if (!isDebugMode_) {
		return;
	}
	DebugText* debugText = DebugText::GetInstance();
	if (!debugText) {
		return;
	}
	DrawDebugCoordinateLabels();

	char playerPositionText[64] = {};
	char cameraPositionText[128] = {};
	char cameraRotationText[128] = {};
	std::snprintf(
	    playerPositionText, sizeof(playerPositionText), "Player X: %.1f",
	    player_->GetWorldPosition().x);
	std::snprintf(
	    cameraPositionText, sizeof(cameraPositionText),
	    "Debug Camera Position  X: %.3f  Y: %.3f  Z: %.3f",
	    debugCamera_.translation_.x, debugCamera_.translation_.y,
	    debugCamera_.translation_.z);
	constexpr float radiansToDegrees = 180.0f / std::numbers::pi_v<float>;
	std::snprintf(
	    cameraRotationText, sizeof(cameraRotationText),
	    "Debug Camera Rotation  X: %.2f deg  Y: %.2f deg  Z: %.2f deg",
	    debugCamera_.rotation_.x * radiansToDegrees,
	    debugCamera_.rotation_.y * radiansToDegrees,
	    debugCamera_.rotation_.z * radiansToDegrees);

	debugText->Print(playerPositionText, 10.0f, 10.0f, 1.0f);
	debugText->Print(cameraPositionText, 10.0f, 30.0f, 1.0f);
	debugText->Print(cameraRotationText, 10.0f, 50.0f, 1.0f);
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	constexpr float hertaTextWidth = 5.0f * DebugText::kFontWidth;
	const float hertaX =
	    static_cast<float>(dxCommon->GetBackBufferWidth()) - hertaTextWidth - 10.0f;
	const float hertaY =
	    static_cast<float>(dxCommon->GetBackBufferHeight()) -
	    static_cast<float>(DebugText::kFontHeight) - 10.0f;
	debugText->Print("Herta", hertaX, hertaY, 1.0f);
	Sprite::PreDraw();
	debugText->DrawAll();
	Sprite::PostDraw();
}

void GameScene::DrawDebugCoordinateLabels() {
	DebugText* debugText = DebugText::GetInstance();
	const Camera& camera = GetActiveCamera();

	Vector2 originScreenPosition = {};
	const Vector3 originLabelPosition = {
	    sceneMap_.origin.x,
	    sceneMap_.groundHeight + kOriginLabelHeight,
	    sceneMap_.origin.z,
	};
	if (ProjectWorldToScreen(originLabelPosition, camera, originScreenPosition)) {
		char originText[48] = {};
		std::snprintf(
		    originText, sizeof(originText), "ORIGIN (%.0f,%.0f,%.0f)",
		    sceneMap_.origin.x, sceneMap_.groundHeight, sceneMap_.origin.z);
		const float textWidth =
		    static_cast<float>(std::strlen(originText)) * DebugText::kFontWidth;
		debugText->Print(
		    originText, originScreenPosition.x - textWidth * 0.5f,
		    originScreenPosition.y);
	}

	for (
	    int index = -kNegativeCoordinateCount;
	    index < static_cast<int>(kMapBlockCount); ++index) {
		const float coordinateX =
		    sceneMap_.origin.x + static_cast<float>(index) * kBlockSize;
		const Vector3 labelPosition = {
		    coordinateX,
		    sceneMap_.groundHeight + kCoordinateLabelHeight,
		    sceneMap_.origin.z,
		};
		Vector2 screenPosition = {};
		if (!ProjectWorldToScreen(labelPosition, camera, screenPosition)) {
			continue;
		}

		char coordinateText[16] = {};
		std::snprintf(
		    coordinateText, sizeof(coordinateText), "X=%.0f", coordinateX);
		const float textWidth =
		    static_cast<float>(std::strlen(coordinateText)) * DebugText::kFontWidth;
		debugText->Print(
		    coordinateText, screenPosition.x - textWidth * 0.5f,
		    screenPosition.y);
	}
}

void GameScene::UpdateCamera() {
	if (!isDebugMode_) {
		return;
	}

	Input* input = Input::GetInstance();
	const bool isRotating = input->IsPressMouse(0);
	const bool isPanning = input->IsPressMouse(2);
	if (isRotating || isPanning) {
		const Input::MouseMove mouseMove = input->GetMouseMove();
		if (isRotating) {
			debugCameraYaw_ += static_cast<float>(mouseMove.lX) * kMouseSensitivity;
			debugCameraPitch_ += static_cast<float>(mouseMove.lY) * kMouseSensitivity;
			debugCameraPitch_ = std::clamp(debugCameraPitch_, -1.35f, 1.35f);
		}

		if (isPanning) {
			const float panSpeed = debugCameraDistance_ * 0.0015f;
			const Vector3 right = {
			    std::cos(debugCameraYaw_),
			    0.0f,
			    -std::sin(debugCameraYaw_),
			};
			const Vector3 up = {
			    std::sin(debugCameraPitch_) * std::sin(debugCameraYaw_),
			    std::cos(debugCameraPitch_),
			    std::sin(debugCameraPitch_) * std::cos(debugCameraYaw_),
			};
			const float mouseX = static_cast<float>(mouseMove.lX) * panSpeed;
			const float mouseY = static_cast<float>(mouseMove.lY) * panSpeed;
			debugCameraTarget_.x += -right.x * mouseX + up.x * mouseY;
			debugCameraTarget_.y += -right.y * mouseX + up.y * mouseY;
			debugCameraTarget_.z += -right.z * mouseX + up.z * mouseY;
		}
	}

	debugCameraDistance_ -=
	    static_cast<float>(input->GetWheel()) * kMouseWheelZoomSpeed;
	debugCameraDistance_ = std::clamp(
	    debugCameraDistance_, kMinDebugCameraDistance, kMaxDebugCameraDistance);

	const float horizontalDistance =
	    std::cos(debugCameraPitch_) * debugCameraDistance_;
	debugCamera_.translation_.x =
	    debugCameraTarget_.x - std::sin(debugCameraYaw_) * horizontalDistance;
	debugCamera_.translation_.y =
	    debugCameraTarget_.y + std::sin(debugCameraPitch_) * debugCameraDistance_;
	debugCamera_.translation_.z =
	    debugCameraTarget_.z - std::cos(debugCameraYaw_) * horizontalDistance;
	debugCamera_.rotation_ = {debugCameraPitch_, debugCameraYaw_, 0.0f};
	debugCamera_.UpdateMatrix();
	UpdateAxisIndicatorCamera();
}

void GameScene::UpdateAxisIndicatorCamera() {
	const float indicatorDistance = 7.0f;
	const float horizontalDistance =
	    std::cos(debugCameraPitch_) * indicatorDistance;
	axisIndicatorCamera_.translation_.x =
	    -std::sin(debugCameraYaw_) * horizontalDistance;
	axisIndicatorCamera_.translation_.y =
	    std::sin(debugCameraPitch_) * indicatorDistance;
	axisIndicatorCamera_.translation_.z =
	    -std::cos(debugCameraYaw_) * horizontalDistance;
	axisIndicatorCamera_.rotation_ = {
	    debugCameraPitch_, debugCameraYaw_, 0.0f};
	axisIndicatorCamera_.UpdateMatrix();
}

void GameScene::Draw() {
	Model::PreDraw();
	skydome_->Draw(GetActiveCamera());
	Model::PostDraw();

	DrawMapGrid();
	DrawMapBlocks();
	DrawPlayer();

	if (isDebugMode_) {
		DrawPlayerCollisionBox();
		DrawAxisIndicator();
	}

	DrawMouseCircle();
	DrawDebugInfo();
}

void GameScene::DrawMapBlocks() {
	Model::PreDraw();
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		modelBlock_->Draw(block->worldTransform, GetActiveCamera());
		for (const std::unique_ptr<Obstacle>& obstacle : block->obstacles) {
			obstacle->Draw(GetActiveCamera());
		}
	}
	for (const std::unique_ptr<Obstacle>& obstacle : detachedObstacles_) {
		obstacle->Draw(GetActiveCamera());
	}
	Model::PostDraw();
}

void GameScene::DrawPlayer() {
	Model::PreDraw();
	player_->Draw(GetActiveCamera());
	Model::PostDraw();
}

void GameScene::DrawPlayerCollisionBox() {
	primitiveDrawer_->SetCamera(&GetActiveCamera());
	const AABB aabb = player_->GetAABB();
	const Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};

	const Vector3 corners[8] = {
	    {aabb.min.x, aabb.min.y, aabb.min.z},
	    {aabb.max.x, aabb.min.y, aabb.min.z},
	    {aabb.max.x, aabb.max.y, aabb.min.z},
	    {aabb.min.x, aabb.max.y, aabb.min.z},
	    {aabb.min.x, aabb.min.y, aabb.max.z},
	    {aabb.max.x, aabb.min.y, aabb.max.z},
	    {aabb.max.x, aabb.max.y, aabb.max.z},
	    {aabb.min.x, aabb.max.y, aabb.max.z},
	};
	constexpr int edgeIndices[12][2] = {
	    {0, 1}, {1, 2}, {2, 3}, {3, 0},
	    {4, 5}, {5, 6}, {6, 7}, {7, 4},
	    {0, 4}, {1, 5}, {2, 6}, {3, 7},
	};

	for (const auto& edge : edgeIndices) {
		primitiveDrawer_->DrawLine3d(
		    corners[edge[0]], corners[edge[1]], color);
	}
}

void GameScene::DrawMapGrid() {
	primitiveDrawer_->Reset();
	primitiveDrawer_->SetCamera(&GetActiveCamera());

	const float minX = sceneMap_.origin.x - sceneMap_.size.x * 0.5f;
	const float maxX = sceneMap_.origin.x + sceneMap_.size.x * 0.5f;
	const float minZ = sceneMap_.origin.z - sceneMap_.size.z * 0.5f;
	const float maxZ = sceneMap_.origin.z + sceneMap_.size.z * 0.5f;
	const float gridY = sceneMap_.groundHeight;
	const Vector4 gridColor = {0.30f, 0.33f, 0.40f, 0.75f};

	for (uint32_t index = 0; index <= sceneMap_.divisionX; ++index) {
		const float rate = static_cast<float>(index) /
		                   static_cast<float>(sceneMap_.divisionX);
		const float x = minX + (maxX - minX) * rate;
		primitiveDrawer_->DrawLine3d({x, gridY, minZ}, {x, gridY, maxZ}, gridColor);
	}

	for (uint32_t index = 0; index <= sceneMap_.divisionZ; ++index) {
		const float rate = static_cast<float>(index) /
		                   static_cast<float>(sceneMap_.divisionZ);
		const float z = minZ + (maxZ - minZ) * rate;
		primitiveDrawer_->DrawLine3d({minX, gridY, z}, {maxX, gridY, z}, gridColor);
	}

	if (isDebugMode_) {
		const float originX = sceneMap_.origin.x;
		const float groundY = sceneMap_.groundHeight;
		const float rulerY = groundY + kRulerHeight;
		const float originZ = sceneMap_.origin.z;
		const float startX =
		    originX - static_cast<float>(kNegativeCoordinateCount) * kBlockSize;
		const float endX =
		    originX + static_cast<float>(kMapBlockCount - 1) * kBlockSize;
		const Vector4 xAxisColor = {1.0f, 0.28f, 0.20f, 1.0f};
		const Vector4 tickColor = {1.0f, 0.82f, 0.18f, 1.0f};

		primitiveDrawer_->DrawLine3d(
		    {startX, rulerY, originZ}, {endX, rulerY, originZ}, xAxisColor);
		primitiveDrawer_->DrawLine3d(
		    {originX, groundY, originZ}, {originX, rulerY, originZ}, xAxisColor);

		for (
		    int index = -kNegativeCoordinateCount;
		    index < static_cast<int>(kMapBlockCount); ++index) {
			const float coordinateX =
			    originX + static_cast<float>(index) * kBlockSize;
			primitiveDrawer_->DrawLine3d(
			    {coordinateX, rulerY, originZ - kCoordinateTickHalfLength},
			    {coordinateX, rulerY, originZ + kCoordinateTickHalfLength},
			    tickColor);
		}
	}

}

void GameScene::DrawAxisIndicator() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();
	const float backBufferWidth = static_cast<float>(dxCommon->GetBackBufferWidth());
	const float backBufferHeight = static_cast<float>(dxCommon->GetBackBufferHeight());
	const float left = backBufferWidth - kAxisIndicatorSize - kAxisIndicatorMargin;
	const float top = kAxisIndicatorMargin;

	const D3D12_VIEWPORT indicatorViewport = {
	    left,
	    top,
	    kAxisIndicatorSize,
	    kAxisIndicatorSize,
	    0.0f,
	    1.0f,
	};
	const D3D12_RECT indicatorScissor = {
	    static_cast<LONG>(left),
	    static_cast<LONG>(top),
	    static_cast<LONG>(left + kAxisIndicatorSize),
	    static_cast<LONG>(top + kAxisIndicatorSize),
	};

	dxCommon->ClearDepthBuffer();
	commandList->RSSetViewports(1, &indicatorViewport);
	commandList->RSSetScissorRects(1, &indicatorScissor);
	Model::PreDraw();
	modelAxis_->Draw(worldTransformAxis_, axisIndicatorCamera_);
	Model::PostDraw();

	const D3D12_VIEWPORT fullViewport = {
	    0.0f,
	    0.0f,
	    backBufferWidth,
	    backBufferHeight,
	    0.0f,
	    1.0f,
	};
	const D3D12_RECT fullScissor = {
	    0,
	    0,
	    static_cast<LONG>(backBufferWidth),
	    static_cast<LONG>(backBufferHeight),
	};
	commandList->RSSetViewports(1, &fullViewport);
	commandList->RSSetScissorRects(1, &fullScissor);
}

void GameScene::DrawMouseCircle() {
	if (!isDebugMode_ || !Input::GetInstance()->IsPressMouse(0)) {
		return;
	}

	const Vector2& mousePosition = Input::GetInstance()->GetMousePosition();
	Sprite::PreDraw();
	for (int y = -kMouseCircleRadius; y < kMouseCircleRadius; ++y) {
		const float sampleY = static_cast<float>(y) + 0.5f;
		const float halfWidth = std::sqrt(
		    static_cast<float>(kMouseCircleRadius * kMouseCircleRadius) -
		    sampleY * sampleY);
		mouseCircleSprite_->SetPosition(
		    {mousePosition.x - halfWidth, mousePosition.y + static_cast<float>(y)});
		mouseCircleSprite_->SetSize({halfWidth * 2.0f, 1.0f});
		mouseCircleSprite_->Draw();
	}
	Sprite::PostDraw();
}

const Camera& GameScene::GetActiveCamera() const {
	return isDebugMode_ ? debugCamera_ : playerCamera_;
}
