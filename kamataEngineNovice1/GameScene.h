#pragma once

#include "GameData.h"
#include "Player.h"

#include <array>
#include <cstddef>

class GameScene {
public:
	~GameScene();

	void Initialize();
	void Update();
	void Draw();

private:
	void UpdateDebugCommand();
	void UpdateCamera();
	void UpdateAxisIndicatorCamera();
	void DrawMapGrid();
	void DrawAxisIndicator();
	void DrawMouseCircle();
	const KamataEngine::Camera& GetActiveCamera() const;

	KamataEngine::Model* modelAxis_ = nullptr;
	KamataEngine::Sprite* mouseCircleSprite_ = nullptr;
	KamataEngine::PrimitiveDrawer* primitiveDrawer_ = nullptr;

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

	static inline const std::array<BYTE, 5> kDebugCommand = {
	    DIK_L,
	    DIK_V,
	    DIK_9,
	    DIK_9,
	    DIK_9,
	};
	static inline const float kMouseSensitivity = 0.005f;
	static inline const float kMouseWheelZoomSpeed = 0.01f;
	static inline const float kMinDebugCameraDistance = 4.0f;
	static inline const float kMaxDebugCameraDistance = 120.0f;
	static inline const float kAxisIndicatorSize = 120.0f;
	static inline const float kAxisIndicatorMargin = 16.0f;
	static inline const int kMouseCircleRadius = 16;
};
