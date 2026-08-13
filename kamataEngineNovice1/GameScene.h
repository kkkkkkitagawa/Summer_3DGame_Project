#pragma once

#include "GameData.h"
#include "Player.h"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

class GameScene {
public:
	~GameScene();

	void Initialize();
	void Update();
	void Draw();

private:
	struct MapBlock {
		KamataEngine::WorldTransform worldTransform;
		float positionX = 0.0f;
		float positionY = 0.0f;
		float verticalVelocity = 0.0f;
		float shakeTime = 0.0f;
		bool isFalling = false;
	};

	void InitializeMapBlocks();
	void UpdateMapBlocks();
	void SpawnMapBlock(float positionX);
	float CalculateShakeStrength(float positionX) const;
	void DrawMapBlocks();
	void UpdateDebugCommand();
	void DrawDebugInfo();
	void UpdateCamera();
	void UpdateAxisIndicatorCamera();
	void DrawMapGrid();
	void DrawAxisIndicator();
	void DrawMouseCircle();
	const KamataEngine::Camera& GetActiveCamera() const;

	KamataEngine::Model* modelAxis_ = nullptr;
	KamataEngine::Model* modelBlock_ = nullptr;
	KamataEngine::Sprite* mouseCircleSprite_ = nullptr;
	KamataEngine::PrimitiveDrawer* primitiveDrawer_ = nullptr;
	std::vector<std::unique_ptr<MapBlock>> mapBlocks_;

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

	// The block OBJ is a 1-unit cube. One world unit represents 32 design pixels.
	static inline const std::size_t kMapBlockCount = 20;
	static inline const float kPixelsPerWorldUnit = 32.0f;
	static inline const float kBlockSizePixels = 32.0f;
	static inline const float kBlockSize = kBlockSizePixels / kPixelsPerWorldUnit;
	static inline const float kBlockMoveSpeed = 2.0f;
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
