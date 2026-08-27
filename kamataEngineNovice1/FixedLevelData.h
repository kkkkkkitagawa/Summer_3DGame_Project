#pragma once

#include "GameData.h"

#include <cstddef>
#include <cstdint>

// Four low bits correspond to Top, Bottom, Front and Back block faces.
struct FixedBlockDefinition {
	uint8_t normalObstacleFaces = 0;
	uint8_t slimeObstacleFaces = 0;
};

struct FixedLevelData {
	const FixedBlockDefinition* blocks = nullptr;
	std::size_t blockCount = 0;
	uint32_t visualSeed = 0;
};

const FixedLevelData& GetFixedLevelData(LevelDifficulty difficulty);
