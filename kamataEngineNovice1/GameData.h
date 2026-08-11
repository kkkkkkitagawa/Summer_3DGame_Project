#pragma once

#include "KamataEngine.h"

#include <cstdint>

// シーンマップの基礎データ
struct SceneMapData {
	KamataEngine::Vector3 origin = {0.0f, 0.0f, 0.0f};
	KamataEngine::Vector3 size = {500.0f, 0.0f, 500.0f};
	uint32_t divisionX = 50;
	uint32_t divisionZ = 50;
	float groundHeight = 0.0f;
};
