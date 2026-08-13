#include "GameScene.h"

#include "WorldTransformUpdate.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
bool isDebugTextInitialized = false;

bool IsModifierKey(BYTE key) {
	return key == DIK_LSHIFT || key == DIK_RSHIFT || key == DIK_LCONTROL ||
	       key == DIK_RCONTROL || key == DIK_LALT || key == DIK_RALT;
}
} // namespace

GameScene::~GameScene() {
	mapBlocks_.clear();
	delete modelBlock_;
	delete modelAxis_;
	delete mouseCircleSprite_;
}

void GameScene::Initialize() {
	modelBlock_ = Model::CreateFromOBJ("block", true);
	assert(modelBlock_);
	InitializeMapBlocks();

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
	UpdateMapBlocks();
	UpdateDebugCommand();
	UpdateCamera();
}

void GameScene::InitializeMapBlocks() {
	mapBlocks_.clear();
	mapBlocks_.reserve(kMapBlockCount);
	for (std::size_t index = 0; index < kMapBlockCount; ++index) {
		SpawnMapBlock(sceneMap_.origin.x + static_cast<float>(index) * kBlockSize);
	}
}

void GameScene::SpawnMapBlock(float positionX) {
	auto block = std::make_unique<MapBlock>();
	block->positionX = positionX;
	block->positionY = sceneMap_.groundHeight;
	block->worldTransform.Initialize();
	block->worldTransform.scale_ = {kBlockSize, kBlockSize, kBlockSize};
	block->worldTransform.translation_ = {
	    block->positionX, block->positionY, sceneMap_.origin.z};
	WorldTransformUpdate(block->worldTransform);
	mapBlocks_.push_back(std::move(block));
}

void GameScene::UpdateMapBlocks() {
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		block->positionX -= kBlockMoveSpeed * kDeltaTime;

		if (!block->isFalling && block->positionX < kFallStartX) {
			block->isFalling = true;
			block->verticalVelocity = 0.0f;
		}

		if (block->isFalling) {
			block->verticalVelocity -= kGravity * kDeltaTime;
			block->positionY += block->verticalVelocity * kDeltaTime;
			block->worldTransform.translation_ = {
			    block->positionX, block->positionY, sceneMap_.origin.z};
			block->worldTransform.rotation_ = {0.0f, 0.0f, 0.0f};
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
			block->worldTransform.rotation_.z = shakeX * 0.35f;
		} else {
			block->worldTransform.translation_ = {
			    block->positionX, block->positionY, sceneMap_.origin.z};
		}

		WorldTransformUpdate(block->worldTransform);
	}

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
		SpawnMapBlock(frontX + kBlockSize);
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

	char cameraPositionText[128] = {};
	char cameraRotationText[128] = {};
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

	debugText->Print(cameraPositionText, 10.0f, 10.0f, 1.0f);
	debugText->Print(cameraRotationText, 10.0f, 30.0f, 1.0f);
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
	DrawMapGrid();
	DrawMapBlocks();

	if (isDebugMode_) {
		DrawAxisIndicator();
	}

	DrawMouseCircle();
	DrawDebugInfo();
}

void GameScene::DrawMapBlocks() {
	Model::PreDraw();
	for (const std::unique_ptr<MapBlock>& block : mapBlocks_) {
		modelBlock_->Draw(block->worldTransform, GetActiveCamera());
	}
	Model::PostDraw();
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
