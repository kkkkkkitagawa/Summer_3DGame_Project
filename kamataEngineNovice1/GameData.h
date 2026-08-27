#pragma once

#include "KamataEngine.h"

#include <cstdint>

enum class LevelDifficulty : uint8_t {
	Easy,
	Normal,
	Hard,
};

inline const char* GetLevelDifficultyName(LevelDifficulty difficulty) {
	switch (difficulty) {
	case LevelDifficulty::Easy:
		return "EASY";
	case LevelDifficulty::Normal:
		return "NORMAL";
	case LevelDifficulty::Hard:
		return "HARD";
	}
	return "UNKNOWN";
}

// シーンマップの基礎データ
struct SceneMapData {
	KamataEngine::Vector3 origin = {0.0f, 0.0f, 0.0f};
	KamataEngine::Vector3 size = {500.0f, 0.0f, 500.0f};
	uint32_t divisionX = 50;
	uint32_t divisionZ = 50;
	float groundHeight = 0.0f;
};
