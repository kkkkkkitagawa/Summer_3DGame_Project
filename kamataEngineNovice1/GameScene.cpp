#include "GameScene.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numbers>
#include <utility>

using namespace KamataEngine;

namespace {
bool isDebugTextInitialized = false;

struct DifficultyCommandDefinition {
	const char* text;
	LevelDifficulty difficulty;
};

constexpr std::array<DifficultyCommandDefinition, 3> kDifficultyCommands = {{
	{"EASY", LevelDifficulty::Easy},
	{"NORMAL", LevelDifficulty::Normal},
	{"HARD", LevelDifficulty::Hard},
}};

char GetTriggeredAlphabeticKey(Input* input) {
	constexpr std::array<std::pair<int, char>, 26> keyMap = {{
	    {DIK_A, 'A'}, {DIK_B, 'B'}, {DIK_C, 'C'}, {DIK_D, 'D'},
	    {DIK_E, 'E'}, {DIK_F, 'F'}, {DIK_G, 'G'}, {DIK_H, 'H'},
	    {DIK_I, 'I'}, {DIK_J, 'J'}, {DIK_K, 'K'}, {DIK_L, 'L'},
	    {DIK_M, 'M'}, {DIK_N, 'N'}, {DIK_O, 'O'}, {DIK_P, 'P'},
	    {DIK_Q, 'Q'}, {DIK_R, 'R'}, {DIK_S, 'S'}, {DIK_T, 'T'},
	    {DIK_U, 'U'}, {DIK_V, 'V'}, {DIK_W, 'W'}, {DIK_X, 'X'},
	    {DIK_Y, 'Y'}, {DIK_Z, 'Z'},
	}};
	for (const auto& [key, character] : keyMap) {
		if (input->TriggerKey(static_cast<BYTE>(key))) {
			return character;
		}
	}
	return '\0';
}

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

GameScene::GameScene(LevelDifficulty difficulty)
    : levelGenerator_(difficulty) {}

GameScene::~GameScene() {
	mapBlocks_.clear();
	detachedObstacles_.clear();
	detachedSlimeObstacles_.clear();
	delete player_;
	delete modelPlayer_;
	delete modelObstacle_;
	delete modelSlimeInner_;
	delete modelSlimeOuter_;
	delete modelGoalStair_;
	delete skydome_;
	delete modelSkydome_;
	delete modelBlock_;
	delete modelAxis_;
	delete mouseCircleSprite_;
}

void GameScene::Initialize(bool obstacleGenerationEnabled) {
	fallenMapBlockCount_ = 0;
	victoryTarget_ = SelectVictoryTarget();
	clearSequenceState_ = ClearSequenceState::Playing;
	deathSequenceState_ = DeathSequenceState::Inactive;
	deathSequenceTime_ = 0.0f;
	goalBlock_ = nullptr;
	switch (levelGenerator_.GetDifficulty()) {
	case LevelDifficulty::Easy:
		mapMoveSpeed_ =
		    kInitialMapMoveSpeed * kEasyMapMoveSpeedMultiplier;
		break;
	case LevelDifficulty::Normal:
		mapMoveSpeed_ =
		    kInitialMapMoveSpeed * kNormalMapMoveSpeedMultiplier;
		break;
	case LevelDifficulty::Hard:
		mapMoveSpeed_ =
		    kInitialMapMoveSpeed * kHardMapMoveSpeedMultiplier;
		break;
	}
	obstacleGenerationEnabled_ = obstacleGenerationEnabled;

	modelSkydome_ = Model::CreateFromOBJ("SkyDome", true);
	assert(modelSkydome_);
	skydome_ = new Skydome();
	skydome_->Initialize(modelSkydome_);

	modelBlock_ = Model::CreateFromOBJ("block", true);
	assert(modelBlock_);
	modelObstacle_ = Model::CreateFromOBJ("cube", true);
	assert(modelObstacle_);
	modelSlimeInner_ = Model::CreateFromOBJ("SlimeInner", false);
	assert(modelSlimeInner_);
	modelSlimeOuter_ = Model::CreateFromOBJ("SlimeOuter", false);
	assert(modelSlimeOuter_);
	modelGoalStair_ = Model::CreateFromOBJ("stairs", true);
	assert(modelGoalStair_);
	slimeInnerTextureHandle_ =
	    TextureManager::Load("SlimeCube/Inner.png");
	slimeOuterTextureHandle_ =
	    TextureManager::Load("SlimeCube/Outer.png");
	outlineColor_.Initialize();
	outlineColor_.SetColor({0.0f, 0.0f, 0.0f, 1.0f});
	InitializeMapBlocks();

	modelPlayer_ = Model::CreateFromOBJ("player", true);
	assert(modelPlayer_);
	player_ = new Player();
	const Vector3 playerPosition = {
	    sceneMap_.origin.x,
	    sceneMap_.groundHeight + kBlockSize * 0.5f +
	        Player::kCollisionHalfSize.y + kPlayerGroundClearance,
	    sceneMap_.origin.z,
	};
	player_->Initialize(
	    modelPlayer_, playerPosition, kPlayerOutlineThickness);
	deathHazardLine_.Initialize({
	    -2.5f,
	    sceneMap_.groundHeight + kBlockSize * 0.5f +
	        3.0f / kPixelsPerWorldUnit,
	    sceneMap_.origin.z,
	});

	modelAxis_ = Model::CreateFromOBJ("axis", true);
	assert(modelAxis_);

	worldTransformAxis_.Initialize();
	worldTransformAxis_.scale_ = {0.60f, 0.60f, 0.60f};
	worldTransformAxis_.translation_ = {0.0f, 0.0f, 0.0f};
	WorldTransformUpdate(worldTransformAxis_);

	playerCamera_.farZ = 1000.0f;
	playerCamera_.Initialize();
	playerCamera_.translation_ = {-3.269f, 7.610f, -10.224f};
	playerCamera_.rotation_ = {0.519934f, 0.404916f, 0.0f};
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

void GameScene::Update(bool allowMapRotationInput) {
	if (deathSequenceState_ != DeathSequenceState::Inactive) {
		UpdateDeathSequence();
		return;
	}

	skydome_->Update();
	switch (clearSequenceState_) {
	case ClearSequenceState::Playing:
	case ClearSequenceState::RunwayApproach:
		player_->Update(
		    kInitialMapMoveSpeed * kEasyMapMoveSpeedMultiplier *
		        kPlayerForwardSpeedMultiplier,
		    sceneMap_.origin.x, kDeltaTime);
		break;
	case ClearSequenceState::PlayerRun:
		player_->UpdateGoalRun(kGoalPlayerRunSpeed, kDeltaTime);
		break;
	case ClearSequenceState::PlayerLaunch:
		player_->UpdateClearLaunch(kDeltaTime);
		break;
	case ClearSequenceState::Ready:
		break;
	}
	if (allowMapRotationInput &&
	    (clearSequenceState_ == ClearSequenceState::Playing ||
	     clearSequenceState_ == ClearSequenceState::RunwayApproach)) {
		UpdateMapRotationInput();
	}
	UpdateMapBlocks();
	if (clearSequenceState_ == ClearSequenceState::Playing &&
	    obstacleGenerationEnabled_ &&
	    fallenMapBlockCount_ >= victoryTarget_) {
		BeginClearSequence();
	}
	UpdateClearSequence();
	if (obstacleGenerationEnabled_ &&
	    (clearSequenceState_ == ClearSequenceState::Playing ||
	     clearSequenceState_ == ClearSequenceState::RunwayApproach)) {
		ResolvePlayerObstacleCollisions();
	}
	deathHazardLine_.Update(player_->GetWorldPosition().x);
	UpdateDebugCommand();
	UpdateDifficultyCommand();
	if (!isDebugMode_ &&
	    clearSequenceState_ == ClearSequenceState::Playing &&
	    player_->GetWorldPosition().x <= kDeathTriggerX) {
		BeginDeathSequence();
	}
	UpdateCamera();
}

std::optional<LevelDifficulty>
GameScene::ConsumeDifficultyChangeRequest() {
	const std::optional<LevelDifficulty> request = difficultyChangeRequest_;
	difficultyChangeRequest_.reset();
	return request;
}

void GameScene::InitializeMapBlocks() {
	mapBlocks_.clear();
	mapBlocks_.reserve(kMapBlockCount + kClearRunwayBlockCount);
	levelGenerator_.Reset();
	// Keep cosmetic obstacle variations reproducible for the same level seed,
	// without changing the level generator's gameplay random sequence.
	obstacleVisualRandomEngine_.seed(levelGenerator_.GetSeed() ^ 0xA511E9B3u);
	for (std::size_t index = 0; index < kMapBlockCount; ++index) {
		const MapBlockSpawnPlan spawnPlan =
		    !obstacleGenerationEnabled_
		        ? MapBlockSpawnPlan{}
		        : index < kInitialSafeBlockCount
		        ? levelGenerator_.CreateInitialBlockPlan()
		        : levelGenerator_.CreateReplacementBlockPlan();
		SpawnMapBlock(
		    sceneMap_.origin.x + static_cast<float>(index) * kBlockSize,
		    spawnPlan);
	}
}

void GameScene::SpawnMapBlock(
    float positionX, const MapBlockSpawnPlan& spawnPlan) {
	auto block = std::make_unique<MapBlock>();
	block->positionX = positionX;
	block->positionY = sceneMap_.groundHeight;
	block->worldTransform.Initialize();
	block->worldTransform.scale_ = {kBlockSize, kBlockSize, kBlockSize};
	block->modelWorldTransform.Initialize();
	block->modelWorldTransform.scale_ = {
	    kBlockModelScale, kBlockModelScale, kBlockModelScale};
	block->outlineWorldTransform.Initialize();
	block->outlineWorldTransform.scale_ = {
	    kBlockModelScale + kMapOutlineThickness,
	    kBlockModelScale + kMapOutlineThickness,
	    kBlockModelScale + kMapOutlineThickness,
	};
	block->rotationX =
	    static_cast<float>(mapRotationQuarterTurns_) *
	    std::numbers::pi_v<float> * 0.5f;
	block->targetRotationX = block->rotationX;
	block->collisionRotationX = block->rotationX;
	block->worldTransform.rotation_.x = block->rotationX;
	block->worldTransform.translation_ = {
	    block->positionX, block->positionY, sceneMap_.origin.z};
	WorldTransformUpdate(block->worldTransform);
	block->modelWorldTransform.rotation_ = block->worldTransform.rotation_;
	block->modelWorldTransform.translation_ = block->worldTransform.translation_;
	WorldTransformUpdate(block->modelWorldTransform);
	block->outlineWorldTransform.rotation_ = block->worldTransform.rotation_;
	block->outlineWorldTransform.translation_ = block->worldTransform.translation_;
	WorldTransformUpdate(block->outlineWorldTransform);
	for (const ObstacleSpawnPlan& obstaclePlan : spawnPlan.obstacles) {
		switch (obstaclePlan.type) {
		case ObstacleType::Normal:
			AttachObstacle(
			    *block, modelObstacle_, obstaclePlan.attachedFace,
			    obstaclePlan.size, obstaclePlan.interactionRules);
			break;
		case ObstacleType::Slime:
			AttachSlimeObstacle(*block, obstaclePlan.attachedFace);
			break;
		}
	}
	mapBlocks_.push_back(std::move(block));
}

void GameScene::AttachObstacle(
    MapBlock& block, Model* model, BlockFace attachedFace,
    const Vector3& size, ObstacleInteractionRules interactionRules) {
	std::uniform_int_distribution<int> heightDistribution(
	    kMinObstacleVisualHeightHundredths,
	    kMaxObstacleVisualHeightHundredths);
	const float visualHeightScale =
	    static_cast<float>(heightDistribution(obstacleVisualRandomEngine_)) /
	    100.0f;

	const float safeMapMoveSpeed = (std::max)(mapMoveSpeed_, 0.001f);
	const float timeUntilPlayerOrigin = (std::max)(
	    0.0f,
	    (block.positionX - sceneMap_.origin.x) / safeMapMoveSpeed);
	const float availableGrowthTime = (std::max)(
	    kDeltaTime, timeUntilPlayerOrigin - kObstacleGrowthFinishMargin);
	const float maximumGrowthDuration = (std::min)(
	    kMaxObstacleVisualGrowthDuration, availableGrowthTime);
	const float minimumGrowthDuration = (std::min)(
	    kMinObstacleVisualGrowthDuration, maximumGrowthDuration);
	std::uniform_real_distribution<float> durationDistribution(
	    minimumGrowthDuration, maximumGrowthDuration);
	const float visualGrowthDuration =
	    durationDistribution(obstacleVisualRandomEngine_);

	auto obstacle = std::make_unique<Obstacle>();
	obstacle->Initialize(
	    model, attachedFace, &block.worldTransform, kBlockSize * 0.5f, size,
	    interactionRules, visualHeightScale, visualGrowthDuration,
	    kObstacleOutlineThickness);
	block.obstacles.push_back(std::move(obstacle));
}

void GameScene::AttachSlimeObstacle(
    MapBlock& block, BlockFace attachedFace) {
	auto obstacle = std::make_unique<SlimeObstacle>();
	obstacle->Initialize(
	    modelSlimeInner_, modelSlimeOuter_, slimeInnerTextureHandle_,
	    slimeOuterTextureHandle_, obstacleVisualRandomEngine_(), attachedFace,
	    &block.worldTransform,
	    kBlockSize * 0.5f, {kBlockSize, kBlockSize, kBlockSize});
	block.slimeObstacles.push_back(std::move(obstacle));
}

void GameScene::AttachGoalStairs(MapBlock& block) {
	constexpr std::array<BlockFace, 4> faces = {
	    BlockFace::Top,
	    BlockFace::Bottom,
	    BlockFace::Front,
	    BlockFace::Back,
	};
	block.goalStairs.reserve(faces.size());
	for (BlockFace face : faces) {
		auto stair = std::make_unique<GoalStair>();
		stair->Initialize(
		    modelGoalStair_, face, &block.worldTransform,
		    kBlockSize * 0.5f, kGoalStairOutlineThickness);
		block.goalStairs.push_back(std::move(stair));
	}
}

std::size_t GameScene::SelectVictoryTarget() const {
	int minimumTarget = 90;
	int maximumTarget = 110;
	switch (levelGenerator_.GetDifficulty()) {
	case LevelDifficulty::Easy:
		break;
	case LevelDifficulty::Normal:
		minimumTarget = 200;
		maximumTarget = 210;
		break;
	case LevelDifficulty::Hard:
		minimumTarget = 295;
		maximumTarget = 320;
		break;
	}
	std::random_device randomDevice;
	std::mt19937 randomEngine(randomDevice());
	std::uniform_int_distribution<int> targetDistribution(
	    minimumTarget, maximumTarget);
	return static_cast<std::size_t>(targetDistribution(randomEngine));
}

void GameScene::BeginClearSequence() {
	if (clearSequenceState_ != ClearSequenceState::Playing) {
		return;
	}
	fallenMapBlockCount_ = victoryTarget_;
	clearSequenceState_ = ClearSequenceState::RunwayApproach;
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		for (const std::unique_ptr<Obstacle>& obstacle : block->obstacles) {
			obstacle->StartClearRetraction(
			    kClearObstacleRetractionDuration);
		}
		for (const std::unique_ptr<SlimeObstacle>& slime :
		     block->slimeObstacles) {
			slime->StartClearRetraction(kClearObstacleRetractionDuration);
		}
	}

	float frontX = sceneMap_.origin.x - kBlockSize;
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		frontX = (std::max)(frontX, block->positionX);
	}
	for (std::size_t index = 0; index < kClearRunwayBlockCount; ++index) {
		frontX += kBlockSize;
		SpawnMapBlock(frontX, {});
	}
	goalBlock_ = mapBlocks_.back().get();
	goalBlock_->isGoalBlock = true;
	AttachGoalStairs(*goalBlock_);
}

void GameScene::UpdateClearSequence() {
	if (!goalBlock_) {
		return;
	}
	if (clearSequenceState_ == ClearSequenceState::RunwayApproach) {
		const float distanceToGoal =
		    goalBlock_->positionX - player_->GetWorldPosition().x;
		if (distanceToGoal <= kGoalStopDistance) {
			clearSequenceState_ = ClearSequenceState::PlayerRun;
		}
		return;
	}
	if (clearSequenceState_ == ClearSequenceState::PlayerRun) {
		const float contactX =
		    goalBlock_->positionX - kBlockSize * 0.5f -
		    Player::kCollisionHalfSize.x;
		if (player_->GetWorldPosition().x >= contactX) {
			player_->SetPositionX(contactX);
			player_->StartClearLaunch();
			clearSequenceState_ = ClearSequenceState::PlayerLaunch;
		}
		return;
	}
	if (clearSequenceState_ == ClearSequenceState::PlayerLaunch &&
	    player_->IsClearLaunchFinished()) {
		clearSequenceState_ = ClearSequenceState::Ready;
	}
}

void GameScene::BeginDeathSequence() {
	if (deathSequenceState_ != DeathSequenceState::Inactive ||
	    isDebugMode_ ||
	    clearSequenceState_ != ClearSequenceState::Playing) {
		return;
	}

	deathCameraStartPosition_ = playerCamera_.translation_;
	deathCameraStartRotation_ = playerCamera_.rotation_;
	deathSequenceTime_ = 0.0f;
	deathSequenceState_ = DeathSequenceState::CameraMove;
}

void GameScene::UpdateDeathSequence() {
	if (deathSequenceState_ == DeathSequenceState::CameraMove) {
		deathSequenceTime_ = (std::min)(
		    deathSequenceTime_ + kDeltaTime, kDeathCameraMoveDuration);
		const float progress = std::clamp(
		    deathSequenceTime_ / kDeathCameraMoveDuration, 0.0f, 1.0f);
		const float smoothProgress =
		    progress * progress * (3.0f - 2.0f * progress);
		auto lerp = [smoothProgress](float start, float end) {
			return start + (end - start) * smoothProgress;
		};
		playerCamera_.translation_ = {
		    lerp(deathCameraStartPosition_.x, kDeathCameraPosition.x),
		    lerp(deathCameraStartPosition_.y, kDeathCameraPosition.y),
		    lerp(deathCameraStartPosition_.z, kDeathCameraPosition.z),
		};
		playerCamera_.rotation_ = {
		    lerp(deathCameraStartRotation_.x, kDeathCameraRotation.x),
		    lerp(deathCameraStartRotation_.y, kDeathCameraRotation.y),
		    lerp(deathCameraStartRotation_.z, kDeathCameraRotation.z),
		};
		playerCamera_.UpdateMatrix();

		if (progress >= 1.0f) {
			player_->StartDeathFall();
			deathSequenceState_ = DeathSequenceState::PlayerFall;
		}
		return;
	}

	if (deathSequenceState_ == DeathSequenceState::PlayerFall) {
		player_->UpdateDeathFall(kDeltaTime);
		if (player_->IsDeathFallFinished()) {
			deathSequenceState_ = DeathSequenceState::Ready;
		}
	}
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

AABB GameScene::GetSlimeObstacleLogicalAABB(
    const MapBlock& block, const SlimeObstacle& obstacle,
    float rotationX) const {
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
	player_->StartTurnJump();

	const int turnDirection = rotateLeft ? 1 : -1;
	if (IsMapRotationDestinationBlocked(turnDirection, 1)) {
		StartBlockedRotationFeedback(turnDirection);
		return;
	}

	blockedRotationFeedbackTime_ = 0.0f;
	blockedRotationFeedbackDirection_ = 0;
	// 同じブロックの多面障害物を通過するときも、1入力につき90度だけ進める。
	// 移動先にも障害物があれば引き続き押され、次の入力で別の面へ進める。
	mapRotationQuarterTurns_ += turnDirection;
	const float rotationAmount =
	    static_cast<float>(turnDirection) *
	    std::numbers::pi_v<float> * 0.5f;

	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		if (!block->isFalling) {
			block->targetRotationX += rotationAmount;
			// ゲーム判定は入力時に目的面へ切り替え、表示だけを補間する。
			block->collisionRotationX = block->targetRotationX;
		}
	}
}

bool GameScene::IsMapRotationDestinationBlocked(
    int turnDirection, int quarterTurnCount) const {
	const AABB playerAABB = player_->GetAABB();
	const float rotationAmount =
	    static_cast<float>(turnDirection * quarterTurnCount) *
	    std::numbers::pi_v<float> * 0.5f;

	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		if (block->isFalling) {
			continue;
		}

		bool hasCurrentPlayerFaceContact = false;
		bool hasDestinationBlocker = false;
		for (const std::unique_ptr<Obstacle>& obstacle : block->obstacles) {
			const ObstacleInteractionRules& rules =
			    obstacle->GetInteractionRules();
			if (!obstacle->IsCollisionEnabled()) {
				continue;
			}

			const AABB obstacleAABB = GetObstacleLogicalAABB(
			    *block, *obstacle, block->collisionRotationX);
			const bool overlapsPlayerX =
			    obstacleAABB.min.x <=
			        playerAABB.max.x + kRotationBlockerSkin &&
			    obstacleAABB.max.x >=
			        playerAABB.min.x - kRotationBlockerSkin;
			if (!overlapsPlayerX) {
				continue;
			}

			const ObstacleSurfaceRelation currentRelation =
			    obstacle->GetSurfaceRelation(
			        block->collisionRotationX, kPlayerSurfaceNormal);
			if (rules.pushesPlayerOnPlayerFace &&
			    currentRelation == ObstacleSurfaceRelation::PlayerFace) {
				const float obstacleCenterX =
				    (obstacleAABB.min.x + obstacleAABB.max.x) * 0.5f;
				hasCurrentPlayerFaceContact =
				    hasCurrentPlayerFaceContact ||
				    (IsCollision(playerAABB, obstacleAABB) &&
				     obstacleCenterX >= player_->GetWorldPosition().x);
			}

			const ObstacleSurfaceRelation proposedRelation =
			    obstacle->GetSurfaceRelation(
			        block->collisionRotationX + rotationAmount,
			        kPlayerSurfaceNormal);
			if (rules.blocksRotationFromSide &&
			    proposedRelation == ObstacleSurfaceRelation::PlayerFace) {
				hasDestinationBlocker = true;
			}
		}

		// 連続壁の例外は同じマップブロック内だけで成立する。
		// 別ブロック由来の進行先障害物は、他の例外で解除しない。
		if (hasDestinationBlocker && !hasCurrentPlayerFaceContact) {
			return true;
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
	const bool isMapStoppedForClear =
	    clearSequenceState_ == ClearSequenceState::PlayerRun ||
	    clearSequenceState_ == ClearSequenceState::PlayerLaunch ||
	    clearSequenceState_ == ClearSequenceState::Ready;
	const float activeMapMoveSpeed =
	    isMapStoppedForClear ? 0.0f : mapMoveSpeed_;
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	const float screenWidth =
	    static_cast<float>(dxCommon->GetBackBufferWidth());
	const float screenHeight =
	    static_cast<float>(dxCommon->GetBackBufferHeight());

	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		block->positionX -= activeMapMoveSpeed * kDeltaTime;

		bool startedFalling = false;
		if (!block->isFalling && block->positionX < kFallStartX) {
			block->isFalling = true;
			startedFalling = true;
			block->verticalVelocity = 0.0f;
			block->targetRotationX = block->rotationX;
			block->collisionRotationX = block->rotationX;
		}

		if (!block->isFalling) {
			const float rotationStep =
			    (std::numbers::pi_v<float> * 0.5f / kMapRotationDuration) *
			    kDeltaTime;
			const float rotationDifference =
			    block->targetRotationX - block->rotationX;
			block->rotationX += std::clamp(
			    rotationDifference, -rotationStep, rotationStep);

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
		block->modelWorldTransform.rotation_ = block->worldTransform.rotation_;
		block->modelWorldTransform.translation_ =
		    block->worldTransform.translation_;
		WorldTransformUpdate(block->modelWorldTransform);
		block->outlineWorldTransform.rotation_ = block->worldTransform.rotation_;
		block->outlineWorldTransform.translation_ =
		    block->worldTransform.translation_;
		WorldTransformUpdate(block->outlineWorldTransform);
		for (const std::unique_ptr<GoalStair>& stair : block->goalStairs) {
			stair->Update();
		}
		const float safeMapMoveSpeed = (std::max)(mapMoveSpeed_, 0.001f);
		const float timeUntilPlayerOrigin = (std::max)(
		    0.0f,
		    (block->positionX - sceneMap_.origin.x) / safeMapMoveSpeed);
		const float timeUntilGrowthDeadline = (std::max)(
		    0.0f,
		    timeUntilPlayerOrigin - kObstacleGrowthFinishMargin);
		Vector2 blockScreenPosition = {};
		const bool isBlockProjected = ProjectWorldToScreen(
		    block->worldTransform.translation_, playerCamera_,
		    blockScreenPosition);
		const bool canStartObstacleGrowth =
		    isBlockProjected &&
		    blockScreenPosition.x >= kObstacleGrowthScreenMarginPixels &&
		    blockScreenPosition.x <=
		        screenWidth - kObstacleGrowthScreenMarginPixels &&
		    blockScreenPosition.y >= kObstacleGrowthScreenMarginPixels &&
		    blockScreenPosition.y <=
		        screenHeight - kObstacleGrowthScreenMarginPixels;
		for (auto iterator = block->obstacles.begin();
		     iterator != block->obstacles.end();) {
			Obstacle& obstacle = **iterator;
			obstacle.Update(
			    kDeltaTime, kGravity, timeUntilGrowthDeadline,
			    canStartObstacleGrowth);
			if (obstacle.IsDead()) {
				iterator = block->obstacles.erase(iterator);
				continue;
			}
			if (startedFalling) {
				obstacle.DetachAndFall(
				    {-activeMapMoveSpeed, block->verticalVelocity, 0.0f},
				    kObstacleDetachRepulsionSpeed);
				detachedObstacles_.push_back(std::move(*iterator));
				iterator = block->obstacles.erase(iterator);
				continue;
			}
			++iterator;
		}
		for (auto iterator = block->slimeObstacles.begin();
		     iterator != block->slimeObstacles.end();) {
			SlimeObstacle& slime = **iterator;
			slime.Update(kDeltaTime, kGravity);
			if (slime.IsDead()) {
				iterator = block->slimeObstacles.erase(iterator);
				continue;
			}
			if (startedFalling) {
				slime.DetachAndFall(
				    {-activeMapMoveSpeed, block->verticalVelocity, 0.0f},
				    kObstacleDetachRepulsionSpeed);
				detachedSlimeObstacles_.push_back(std::move(*iterator));
				iterator = block->slimeObstacles.erase(iterator);
				continue;
			}
			++iterator;
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
	if (obstacleGenerationEnabled_ &&
	    clearSequenceState_ == ClearSequenceState::Playing) {
		fallenMapBlockCount_ = (std::min)(
		    victoryTarget_, fallenMapBlockCount_ + removedCount);
	}
	for (std::size_t index = 0;
	     index < removedCount &&
	     clearSequenceState_ == ClearSequenceState::Playing;
	     ++index) {
		float frontX = sceneMap_.origin.x - kBlockSize;
		for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
			frontX = (std::max)(frontX, block->positionX);
		}
		SpawnMapBlock(
		    frontX + kBlockSize,
		    obstacleGenerationEnabled_
		        ? levelGenerator_.CreateReplacementBlockPlan()
		        : MapBlockSpawnPlan{});
	}
}

void GameScene::UpdateDetachedObstacles() {
	for (const std::unique_ptr<Obstacle>& obstacle : detachedObstacles_) {
		obstacle->Update(kDeltaTime, kGravity, 0.0f, true);
	}

	const float deleteY = sceneMap_.groundHeight - kDeleteDistance;
	detachedObstacles_.erase(
	    std::remove_if(
	        detachedObstacles_.begin(), detachedObstacles_.end(),
	        [deleteY](const std::unique_ptr<Obstacle>& obstacle) {
		        return obstacle->GetWorldPosition().y < deleteY;
	        }),
	    detachedObstacles_.end());

	for (const std::unique_ptr<SlimeObstacle>& obstacle :
	     detachedSlimeObstacles_) {
		obstacle->Update(kDeltaTime, kGravity);
	}
	detachedSlimeObstacles_.erase(
	    std::remove_if(
	        detachedSlimeObstacles_.begin(),
	        detachedSlimeObstacles_.end(),
	        [deleteY](const std::unique_ptr<SlimeObstacle>& obstacle) {
		        return obstacle->IsDead() ||
		               obstacle->GetWorldPosition().y < deleteY;
	        }),
	    detachedSlimeObstacles_.end());
}

void GameScene::ResolvePlayerObstacleCollisions() {
	AABB playerAABB = player_->GetAABB();
	bool slimeTriggered = false;
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		if (block->isFalling) {
			continue;
		}
		for (const std::unique_ptr<SlimeObstacle>& slime :
		     block->slimeObstacles) {
			if (!slime->IsCollisionEnabled() ||
			    slime->GetSurfaceRelation(
			        block->collisionRotationX, kPlayerSurfaceNormal) !=
			        ObstacleSurfaceRelation::PlayerFace) {
				continue;
			}

			const AABB slimeAABB = GetSlimeObstacleLogicalAABB(
			    *block, *slime, block->collisionRotationX);
			const float slimeCenterX =
			    (slimeAABB.min.x + slimeAABB.max.x) * 0.5f;
			if (!IsCollision(playerAABB, slimeAABB) ||
			    slimeCenterX < player_->GetWorldPosition().x) {
				continue;
			}

			if (slime->TriggerHit()) {
				player_->StartKnockback(
				    kSlimeKnockbackDistance, kSlimeKnockbackDuration);
				slimeTriggered = true;
				break;
			}
		}
		if (slimeTriggered) {
			break;
		}
	}

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
			if (!IsCollision(playerAABB, obstacleAABB)) {
				continue;
			}

			const float correctedPlayerX =
			    obstacleCenterX >= player_->GetWorldPosition().x
			        ? obstacleAABB.min.x - Player::kCollisionHalfSize.x
			        : obstacleAABB.max.x + Player::kCollisionHalfSize.x;
			player_->SetPositionX(
			    (std::min)(sceneMap_.origin.x, correctedPlayerX));
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
			difficultyCommandInput_.clear();
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

void GameScene::UpdateDifficultyCommand() {
	if (!isDebugMode_ || difficultyChangeRequest_) {
		return;
	}

	Input* input = Input::GetInstance();
	if (input->TriggerKey(DIK_BACK)) {
		if (!difficultyCommandInput_.empty()) {
			difficultyCommandInput_.pop_back();
		}
		return;
	}

	const char character = GetTriggeredAlphabeticKey(input);
	if (character == '\0') {
		return;
	}

	difficultyCommandInput_.push_back(character);
	for (const DifficultyCommandDefinition& command : kDifficultyCommands) {
		if (difficultyCommandInput_ == command.text) {
			difficultyChangeRequest_ = command.difficulty;
			return;
		}
	}

	const auto isCommandPrefix = [this](const char* commandText) {
		return std::strncmp(
		           commandText, difficultyCommandInput_.c_str(),
		           difficultyCommandInput_.size()) == 0;
	};
	for (const DifficultyCommandDefinition& command : kDifficultyCommands) {
		if (isCommandPrefix(command.text)) {
			return;
		}
	}

	// Treat the latest key as a possible beginning of a new command.
	difficultyCommandInput_.assign(1, character);
	for (const DifficultyCommandDefinition& command : kDifficultyCommands) {
		if (isCommandPrefix(command.text)) {
			return;
		}
	}
	difficultyCommandInput_.clear();
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
	char solverFaceText[96] = {};
	char levelSeedText[64] = {};
	char levelDifficultyText[64] = {};
	char mapMoveSpeedText[64] = {};
	char fallenMapBlockCountText[64] = {};
	char victoryTargetText[64] = {};
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
	int solverFaceIndex = (-mapRotationQuarterTurns_) % 4;
	if (solverFaceIndex < 0) {
		solverFaceIndex += 4;
	}
	std::snprintf(
	    solverFaceText, sizeof(solverFaceText),
	    "Solver Face: %d deg  (D: +90 / A: -90)",
	    solverFaceIndex * 90);
	std::snprintf(
	    levelSeedText, sizeof(levelSeedText), "Level Seed: %u",
	    levelGenerator_.GetSeed());
	std::snprintf(
	    levelDifficultyText, sizeof(levelDifficultyText),
	    "Level Difficulty: %s", levelGenerator_.GetDifficultyName());
	std::snprintf(
	    mapMoveSpeedText, sizeof(mapMoveSpeedText),
	    "Map Move Speed: %.2f", mapMoveSpeed_);
	std::snprintf(
	    fallenMapBlockCountText, sizeof(fallenMapBlockCountText),
	    "Fallen Map Blocks: %zu", fallenMapBlockCount_);
	std::snprintf(
	    victoryTargetText, sizeof(victoryTargetText),
	    "CLEAR TARGET: %zu", victoryTarget_);
	char currentDifficultyText[64] = {};
	std::snprintf(
	    currentDifficultyText, sizeof(currentDifficultyText),
	    "CURRENT DIFFICULTY: %s", levelGenerator_.GetDifficultyName());
	debugText->Print(playerPositionText, 10.0f, 10.0f, 1.0f);
	debugText->Print(cameraPositionText, 10.0f, 30.0f, 1.0f);
	debugText->Print(cameraRotationText, 10.0f, 50.0f, 1.0f);
	debugText->Print(solverFaceText, 10.0f, 70.0f, 1.0f);
	debugText->Print(levelSeedText, 10.0f, 90.0f, 1.0f);
	debugText->Print(levelDifficultyText, 10.0f, 110.0f, 1.0f);
	debugText->Print(mapMoveSpeedText, 10.0f, 130.0f, 1.0f);
	debugText->Print(fallenMapBlockCountText, 10.0f, 150.0f, 1.0f);
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	constexpr float difficultyTextScale = 1.0f;
	const float difficultyTextWidth =
	    static_cast<float>(std::strlen(currentDifficultyText)) *
	    DebugText::kFontWidth * difficultyTextScale;
	const float difficultyTextX =
	    (static_cast<float>(dxCommon->GetBackBufferWidth()) -
	     difficultyTextWidth) *
	    0.5f;
	debugText->Print(
	    currentDifficultyText, difficultyTextX, 24.0f,
	    difficultyTextScale);
	if (obstacleGenerationEnabled_) {
		const float victoryTargetTextWidth =
		    static_cast<float>(std::strlen(victoryTargetText)) *
		    DebugText::kFontWidth;
		const float victoryTargetTextX =
		    (static_cast<float>(dxCommon->GetBackBufferWidth()) -
		     victoryTargetTextWidth) *
		    0.5f;
		debugText->Print(victoryTargetText, victoryTargetTextX, 44.0f, 1.0f);
	}
	constexpr float hertaTextWidth = 5.0f * DebugText::kFontWidth;
	const float hertaX =
	    static_cast<float>(dxCommon->GetBackBufferWidth()) - hertaTextWidth - 10.0f;
	const float hertaY =
	    static_cast<float>(dxCommon->GetBackBufferHeight()) -
	    static_cast<float>(DebugText::kFontHeight) - 10.0f;
	debugText->Print("Herta", hertaX, hertaY, 1.0f);
	constexpr const char* difficultyCodesText = "Easy   Normal   Hard";
	debugText->Print(difficultyCodesText, 10.0f, hertaY, 1.0f);
	char difficultyInputText[64] = {};
	std::snprintf(
	    difficultyInputText, sizeof(difficultyInputText), "Input: %s",
	    difficultyCommandInput_.c_str());
	debugText->Print(
	    difficultyInputText, 10.0f,
	    hertaY - static_cast<float>(DebugText::kFontHeight) - 4.0f,
	    1.0f);
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
	deathHazardLine_.Draw(GetActiveCamera());
	DrawPlayer();

	if (isDebugMode_) {
		DrawPlayerCollisionBox();
		DrawObstacleCollisionBoxes();
		DrawAxisIndicator();
	}

	DrawMouseCircle();
	DrawDebugInfo();
}

void GameScene::DrawMapBlocks() {
	const Camera& camera = GetActiveCamera();

	// One-pixel inverted hull for each map block.
	Model::PreDraw(
	    Model::CullingMode::kFront, Model::BlendMode::kNone,
	    Model::DepthTestMode::kOn);
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		modelBlock_->Draw(
		    block->outlineWorldTransform, camera, &outlineColor_);
	}
	Model::PostDraw();

	// Draw the map first so its depth hides the part of the expanded outline
	// shell that is inside the block surface.
	Model::PreDraw();
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		modelBlock_->Draw(block->modelWorldTransform, camera);
	}
	Model::PostDraw();

	// Inverted-hull outline: render only the back faces of a slightly expanded
	// black copy, then place the regular obstacle over it.
	Model::PreDraw(
	    Model::CullingMode::kFront, Model::BlendMode::kNone,
	    Model::DepthTestMode::kOn);
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		for (const std::unique_ptr<Obstacle>& obstacle : block->obstacles) {
			obstacle->DrawOutline(camera, outlineColor_);
		}
		for (const std::unique_ptr<GoalStair>& stair : block->goalStairs) {
			stair->DrawOutline(camera, outlineColor_);
		}
	}
	for (const std::unique_ptr<Obstacle>& obstacle : detachedObstacles_) {
		obstacle->DrawOutline(camera, outlineColor_);
	}
	Model::PostDraw();

	Model::PreDraw();
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		for (const std::unique_ptr<Obstacle>& obstacle : block->obstacles) {
			obstacle->Draw(camera);
		}
		for (const std::unique_ptr<SlimeObstacle>& slime :
		     block->slimeObstacles) {
			slime->DrawInner(camera);
		}
		for (const std::unique_ptr<GoalStair>& stair : block->goalStairs) {
			stair->Draw(camera);
		}
	}
	for (const std::unique_ptr<Obstacle>& obstacle : detachedObstacles_) {
		obstacle->Draw(camera);
	}
	for (const std::unique_ptr<SlimeObstacle>& slime :
	     detachedSlimeObstacles_) {
		slime->DrawInner(camera);
	}
	Model::PostDraw();

	// The translucent shell is drawn after the opaque inner cube and does not
	// write depth, so the inner layer remains visible through it.
	Model::PreDraw(
	    Model::CullingMode::kBack, Model::BlendMode::kNormal,
	    Model::DepthTestMode::kReadOnly);
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		for (const std::unique_ptr<SlimeObstacle>& slime :
		     block->slimeObstacles) {
			slime->DrawOuter(camera);
		}
	}
	for (const std::unique_ptr<SlimeObstacle>& slime :
	     detachedSlimeObstacles_) {
		slime->DrawOuter(camera);
	}
	Model::PostDraw();
}

void GameScene::DrawPlayer() {
	const Camera& camera = GetActiveCamera();
	Model::PreDraw(
	    Model::CullingMode::kFront, Model::BlendMode::kNone,
	    Model::DepthTestMode::kOn);
	player_->DrawOutline(camera, outlineColor_);
	Model::PostDraw();

	Model::PreDraw();
	player_->Draw(camera);
	Model::PostDraw();
}

void GameScene::DrawPlayerCollisionBox() {
	primitiveDrawer_->SetCamera(&GetActiveCamera());
	DrawCollisionBox(
	    player_->GetAABB(), {1.0f, 1.0f, 1.0f, 1.0f});
}

void GameScene::DrawObstacleCollisionBoxes() {
	primitiveDrawer_->SetCamera(&GetActiveCamera());
	const Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		for (const std::unique_ptr<Obstacle>& obstacle : block->obstacles) {
			if (!obstacle->IsCollisionEnabled()) {
				continue;
			}

			// 表示補間ではなく、実際のゲーム判定に使うAABBを描画する。
			DrawCollisionBox(
			    GetObstacleLogicalAABB(
			        *block, *obstacle, block->collisionRotationX),
			    color);
		}
		for (const std::unique_ptr<SlimeObstacle>& slime :
		     block->slimeObstacles) {
			if (!slime->IsCollisionEnabled()) {
				continue;
			}
			DrawCollisionBox(
			    GetSlimeObstacleLogicalAABB(
			        *block, *slime, block->collisionRotationX),
			    color);
		}
	}
}

void GameScene::DrawCollisionBox(
    const AABB& aabb, const Vector4& color) {
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
	if (deathSequenceState_ != DeathSequenceState::Inactive) {
		return playerCamera_;
	}
	return isDebugMode_ ? debugCamera_ : playerCamera_;
}
