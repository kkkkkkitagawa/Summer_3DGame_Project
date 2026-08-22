#pragma once

#include "GameData.h"
#include "LevelGenerator.h"
#include "Obstacle.h"
#include "Player.h"
#include "Skydome.h"

#include <array>
#include <cstddef>
#include <memory>
#include <numbers>
#include <vector>

class GameScene {
public:
	~GameScene();

	void Initialize(bool obstacleGenerationEnabled = true);
	void Update(bool allowMapRotationInput = true);
	void Draw();
	bool IsDebugMode() const { return isDebugMode_; }

private:
	struct MapBlock {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::WorldTransform modelWorldTransform;
		float positionX = 0.0f;
		float positionY = 0.0f;
		float verticalVelocity = 0.0f;
		float shakeTime = 0.0f;
		float rotationX = 0.0f;
		float targetRotationX = 0.0f;
		float collisionRotationX = 0.0f;
		bool isFalling = false;
		std::vector<std::unique_ptr<Obstacle>> obstacles;
	};

	void InitializeMapBlocks();
	void UpdateMapRotationInput();
	bool IsMapRotationDestinationBlocked(
	    int turnDirection, int quarterTurnCount) const;
	void StartBlockedRotationFeedback(int turnDirection);
	float CalculateBlockedRotationOffset() const;
	void UpdateMapBlocks();
	void UpdateDetachedObstacles();
	void ResolvePlayerObstacleCollisions();
	void SpawnMapBlock(
	    float positionX, const MapBlockSpawnPlan& spawnPlan);
	void AttachObstacle(
	    MapBlock& block, KamataEngine::Model* model, BlockFace attachedFace,
	    const KamataEngine::Vector3& size,
	    ObstacleInteractionRules interactionRules);
	KamataEngine::Matrix4x4 CreateMapBlockLogicalTransform(
	    const MapBlock& block, float rotationX) const;
	AABB GetObstacleLogicalAABB(
	    const MapBlock& block, const Obstacle& obstacle,
	    float rotationX) const;
	float CalculateShakeStrength(float positionX) const;
	void DrawMapBlocks();
	void UpdateDebugCommand();
	void DrawDebugInfo();
	void UpdateCamera();
	void UpdateAxisIndicatorCamera();
	void DrawMapGrid();
	void DrawDebugCoordinateLabels();
	void DrawPlayer();
	void DrawPlayerCollisionBox();
	void DrawObstacleCollisionBoxes();
	void DrawCollisionBox(
	    const AABB& aabb, const KamataEngine::Vector4& color);
	void DrawAxisIndicator();
	void DrawMouseCircle();
	const KamataEngine::Camera& GetActiveCamera() const;

	KamataEngine::Model* modelAxis_ = nullptr;
	KamataEngine::Model* modelBlock_ = nullptr;
	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::Model* modelObstacle_ = nullptr;
	Skydome* skydome_ = nullptr;
	Player* player_ = nullptr;
	KamataEngine::Sprite* mouseCircleSprite_ = nullptr;
	KamataEngine::PrimitiveDrawer* primitiveDrawer_ = nullptr;
	std::vector<std::unique_ptr<MapBlock>> mapBlocks_;
	std::vector<std::unique_ptr<Obstacle>> detachedObstacles_;
	// 現在は試験用に難しい難度を生成する。採用済みの簡単難度シードは保持する。
	LevelGenerator levelGenerator_{LevelDifficulty::Hard};

	SceneMapData sceneMap_;
	KamataEngine::WorldTransform worldTransformAxis_;
	KamataEngine::Camera playerCamera_;
	KamataEngine::Camera debugCamera_;
	KamataEngine::Camera axisIndicatorCamera_;

	KamataEngine::Vector3 debugCameraTarget_ = {0.0f, 0.0f, 0.0f};
	float debugCameraYaw_ = -0.55f;
	float debugCameraPitch_ = 0.48f;
	float debugCameraDistance_ = 24.0f;

	bool isDebugMode_ = false;
	std::size_t debugCommandIndex_ = 0;
	int mapRotationQuarterTurns_ = 0;
	float blockedRotationFeedbackTime_ = 0.0f;
	int blockedRotationFeedbackDirection_ = 0;
	float mapMoveSpeed_ = 0.0f;
	bool obstacleGenerationEnabled_ = true;

	static inline const std::array<BYTE, 5> kEnterDebugCommand = {
	    DIK_L,
	    DIK_V,
	    DIK_9,
	    DIK_9,
	    DIK_9,
	};
	static inline const std::array<BYTE, 5> kExitDebugCommand = {
	    DIK_H,
	    DIK_E,
	    DIK_R,
	    DIK_T,
	    DIK_A,
	};
	static inline const float kMouseSensitivity = 0.005f;
	static inline const float kMouseWheelZoomSpeed = 0.01f;
	static inline const float kMinDebugCameraDistance = 4.0f;
	static inline const float kMaxDebugCameraDistance = 120.0f;
	static inline const float kAxisIndicatorSize = 120.0f;
	static inline const float kAxisIndicatorMargin = 16.0f;
	static inline const int kMouseCircleRadius = 16;

	// The latest block OBJ is a 2-unit cube. Normalize it to one map block.
	static inline const std::size_t kMapBlockCount = 23;
	static inline const std::size_t kInitialSafeBlockCount = 15;
	static inline const float kPixelsPerWorldUnit = 32.0f;
	// 隣り合うブロックの境界を浮動小数点誤差で抜けないための判定余白。
	static inline const float kRotationBlockerSkin =
	    1.0f / kPixelsPerWorldUnit;
	static inline const float kBlockSizePixels = 32.0f;
	static inline const float kBlockSize = kBlockSizePixels / kPixelsPerWorldUnit;
	static inline const float kBlockModelSourceSize = 2.0f;
	static inline const float kBlockModelScale =
	    kBlockSize / kBlockModelSourceSize;
	static inline const float kPlayerGroundClearance =
	    1.0f / kPixelsPerWorldUnit;
	static inline const int kNegativeCoordinateCount = 5;
	static inline const float kRulerHeightPixels = 18.0f;
	static inline const float kRulerHeight =
	    kRulerHeightPixels / kPixelsPerWorldUnit;
	static inline const float kCoordinateLabelHeight =
	    kRulerHeight + 4.0f / kPixelsPerWorldUnit;
	static inline const float kOriginLabelHeight =
	    kRulerHeight + 24.0f / kPixelsPerWorldUnit;
	static inline const float kCoordinateTickHalfLength =
	    6.0f / kPixelsPerWorldUnit;
	static inline const float kInitialMapMoveSpeed = 2.0f;
	static inline const float kMapRotationDuration = 0.18f;
	static inline const float kBlockedRotationFeedbackDuration = 0.18f;
	static inline const float kBlockedRotationFeedbackAngle =
	    15.0f * std::numbers::pi_v<float> / 180.0f;
	static inline const KamataEngine::Vector3 kPlayerSurfaceNormal = {
	    0.0f,
	    1.0f,
	    0.0f,
	};
	static inline const float kObstacleDetachRepulsionSpeed = 1.5f;
	static inline const float kShakeStartX = -1.0f;
	static inline const float kFallStartX = -3.0f;
	static inline const float kDeleteDistance = 200.0f / kPixelsPerWorldUnit;
	static inline const float kGravity = 9.8f;
	static inline const float kShakeFrequency = 38.0f;
	static inline const float kShakeAmount = 2.0f / kPixelsPerWorldUnit;
	static inline const float kMinShakeStrength = 0.5f;
	static inline const float kMaxShakeStrength = 1.0f;
	static inline const float kDeltaTime = 1.0f / 60.0f;
};
