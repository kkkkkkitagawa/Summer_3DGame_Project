#include "LevelGenerator.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <numbers>
#include <numeric>
#include <utility>
#include <vector>

using namespace KamataEngine;

namespace {
constexpr int kSolverFaceCount = 4;
constexpr uint8_t kAllSolverFaces = 0x0f;
constexpr int kMaximumGenerationAttempts = 256;

int ToFaceIndex(SolverFace solverFace) {
	return static_cast<int>(solverFace);
}
} // namespace

LevelGenerator::LevelGenerator(LevelDifficulty difficulty)
    : difficulty_(difficulty) {
	Reset();
}

const char* LevelGenerator::GetDifficultyName() const {
	switch (difficulty_) {
	case LevelDifficulty::Easy:
		return "EASY";
	case LevelDifficulty::Normal:
		return "NORMAL";
	case LevelDifficulty::Hard:
		return "HARD";
	}
	return "UNKNOWN";
}

void LevelGenerator::Reset() {
	std::random_device randomDevice;
	seed_ = randomDevice();
	randomEngine_.seed(seed_);
	pendingPlans_.clear();
	// ゲーム開始時点のプレイヤー面は0度。
	reachableFaces_ = 0x01;
	recentObstacleMasks_ = {};
}

MapBlockSpawnPlan LevelGenerator::CreateInitialBlockPlan() {
	// 初期20ブロックは空。空ブロック中のA/D移動も求解状態へ反映する。
	reachableFaces_ =
	    AdvanceReachableFaces(reachableFaces_, recentObstacleMasks_[0] |
	                                                 recentObstacleMasks_[1]);
	recentObstacleMasks_[1] = recentObstacleMasks_[0];
	recentObstacleMasks_[0] = 0;
	return {};
}

MapBlockSpawnPlan LevelGenerator::CreateReplacementBlockPlan() {
	if (pendingPlans_.empty()) {
		QueueNextPattern();
	}

	assert(!pendingPlans_.empty());
	MapBlockSpawnPlan plan = std::move(pendingPlans_.front());
	pendingPlans_.pop_front();
	return plan;
}

void LevelGenerator::QueueNextPattern() {
	std::uniform_int_distribution<int> faceOffsetDistribution(
	    0, kSolverFaceCount - 1);
	std::bernoulli_distribution mirrorDistribution(0.5);

	for (int attempt = 0; attempt < kMaximumGenerationAttempts; ++attempt) {
		const PatternDefinition pattern = GenerateCandidatePattern();
		const int faceOffset = faceOffsetDistribution(randomEngine_);
		const bool isMirrored = mirrorDistribution(randomEngine_);
		if (TryQueuePattern(pattern, faceOffset, isMirrored)) {
			return;
		}
	}

	// ランダム候補が成立しない場合は、2つの短い壁を持つ安全形を使う。
	constexpr FaceMask face0 = 1u << 0;
	constexpr FaceMask face2 = 1u << 2;
	const PatternDefinition fallback = {
	    0, 0, face0, face0, 0, 0, face2, face2, 0, 0};
	const bool queued = TryQueuePattern(fallback, 0, false);
	assert(queued);
	(void)queued;
}

LevelGenerator::PatternDefinition LevelGenerator::GenerateCandidatePattern() {
	std::uniform_int_distribution<int> bufferDistribution(2, 3);
	std::uniform_int_distribution<int> occupiedBlockDistribution(4, 6);
	std::discrete_distribution<int> obstacleCountDistribution({45, 35, 20});
	std::bernoulli_distribution continueWallDistribution(0.45);

	for (int attempt = 0; attempt < 64; ++attempt) {
		PatternDefinition pattern = {};
		const int bufferLength = bufferDistribution(randomEngine_);
		std::vector<int> availableBlockIndices;
		for (
		    int blockIndex = bufferLength;
		    blockIndex < static_cast<int>(kPatternLength); ++blockIndex) {
			availableBlockIndices.push_back(blockIndex);
		}
		std::shuffle(
		    availableBlockIndices.begin(), availableBlockIndices.end(),
		    randomEngine_);

		const int occupiedBlockCount = (std::min)(
		    occupiedBlockDistribution(randomEngine_),
		    static_cast<int>(availableBlockIndices.size()));
		for (int index = 0; index < occupiedBlockCount; ++index) {
			pattern[static_cast<std::size_t>(availableBlockIndices[index])] = 0x01;
		}
		if (!HasMultipleObstacleGroups(pattern)) {
			continue;
		}

		FaceMask previousFaceMask = 0;
		for (FaceMask& blockFaceMask : pattern) {
			if (blockFaceMask == 0) {
				previousFaceMask = 0;
				continue;
			}

			const int obstacleCount = obstacleCountDistribution(randomEngine_) + 1;
			const bool continuesPreviousWall =
			    previousFaceMask != 0 && continueWallDistribution(randomEngine_);
			blockFaceMask = CreateRandomFaceMask(
			    obstacleCount,
			    continuesPreviousWall ? previousFaceMask : 0);
			previousFaceMask = blockFaceMask;
		}
		return pattern;
	}

	constexpr FaceMask face0 = 1u << 0;
	constexpr FaceMask face2 = 1u << 2;
	return {0, 0, face0, face0, 0, 0, face2, face2, 0, 0};
}

bool LevelGenerator::TryQueuePattern(
    const PatternDefinition& pattern, int faceOffset, bool isMirrored) {
	// 0、90、180、270度のどの面から入っても最低1つの解が残ることを確認する。
	for (int entryFace = 0; entryFace < kSolverFaceCount; ++entryFace) {
		const FaceMask entryMask = static_cast<FaceMask>(1u << entryFace);
		const SimulationResult result = SimulatePattern(
		    pattern, faceOffset, isMirrored, entryMask, {});
		if (result.reachableFaces == 0) {
			return false;
		}
	}

	const SimulationResult result = SimulatePattern(
	    pattern, faceOffset, isMirrored, reachableFaces_,
	    recentObstacleMasks_);
	if (result.reachableFaces == 0) {
		return false;
	}

	for (FaceMask rawFaceMask : pattern) {
		MapBlockSpawnPlan blockPlan;
		const FaceMask transformedFaceMask =
		    TransformFaceMask(rawFaceMask, faceOffset, isMirrored);
		for (int faceIndex = 0; faceIndex < kSolverFaceCount; ++faceIndex) {
			const FaceMask faceBit = static_cast<FaceMask>(1u << faceIndex);
			if ((transformedFaceMask & faceBit) == 0) {
				continue;
			}

			ObstacleSpawnPlan obstaclePlan;
			obstaclePlan.solverFace = static_cast<SolverFace>(faceIndex);
			obstaclePlan.attachedFace =
			    ConvertSolverFaceToBlockFace(obstaclePlan.solverFace);
			blockPlan.obstacles.push_back(obstaclePlan);
		}
		pendingPlans_.push_back(std::move(blockPlan));
	}
	reachableFaces_ = result.reachableFaces;
	recentObstacleMasks_ = result.recentObstacleMasks;
	return true;
}

LevelGenerator::SimulationResult LevelGenerator::SimulatePattern(
    const PatternDefinition& pattern, int faceOffset, bool isMirrored,
    FaceMask entryFaces,
    const std::array<FaceMask, 2>& entryRecentMasks) const {
	SimulationResult result = {entryFaces, entryRecentMasks};
	for (FaceMask rawFaceMask : pattern) {
		const FaceMask activeObstacleFaces = static_cast<FaceMask>(
		    result.recentObstacleMasks[0] |
		    result.recentObstacleMasks[1]);
		result.reachableFaces = AdvanceReachableFaces(
		    result.reachableFaces, activeObstacleFaces);

		const FaceMask currentObstacleFaces =
		    TransformFaceMask(rawFaceMask, faceOffset, isMirrored);
		result.reachableFaces = static_cast<FaceMask>(
		    result.reachableFaces &
		    static_cast<FaceMask>(~currentObstacleFaces));
		if (result.reachableFaces == 0) {
			return result;
		}

		result.recentObstacleMasks[1] = result.recentObstacleMasks[0];
		result.recentObstacleMasks[0] = currentObstacleFaces;
	}
	return result;
}

LevelGenerator::FaceMask LevelGenerator::AdvanceReachableFaces(
    FaceMask reachableFaces, FaceMask activeObstacleFaces) const {
	// 前2ブロックの障害物がまだプレイヤーXと重なるものとして保守的に扱う。
	const FaceMask safeCurrentFaces = static_cast<FaceMask>(
	    reachableFaces & static_cast<FaceMask>(~activeObstacleFaces));
	FaceMask expandedFaces = safeCurrentFaces;
	for (int faceIndex = 0; faceIndex < kSolverFaceCount; ++faceIndex) {
		const FaceMask faceBit = static_cast<FaceMask>(1u << faceIndex);
		if ((safeCurrentFaces & faceBit) == 0) {
			continue;
		}

		const int rightFace = (faceIndex + 1) % kSolverFaceCount;
		const int leftFace =
		    (faceIndex + kSolverFaceCount - 1) % kSolverFaceCount;
		const FaceMask rightFaceBit = static_cast<FaceMask>(1u << rightFace);
		const FaceMask leftFaceBit = static_cast<FaceMask>(1u << leftFace);
		if ((activeObstacleFaces & rightFaceBit) == 0) {
			expandedFaces = static_cast<FaceMask>(expandedFaces | rightFaceBit);
		}
		if ((activeObstacleFaces & leftFaceBit) == 0) {
			expandedFaces = static_cast<FaceMask>(expandedFaces | leftFaceBit);
		}
	}
	return static_cast<FaceMask>(expandedFaces & kAllSolverFaces);
}

LevelGenerator::FaceMask LevelGenerator::CreateRandomFaceMask(
    int obstacleCount, FaceMask requiredFaces) {
	assert(obstacleCount >= 1 && obstacleCount <= 3);
	std::uniform_int_distribution<int> faceDistribution(
	    0, kSolverFaceCount - 1);

	FaceMask result = 0;
	if (requiredFaces != 0) {
		std::array<int, kSolverFaceCount> requiredFaceIndices = {};
		int requiredFaceCount = 0;
		for (int faceIndex = 0; faceIndex < kSolverFaceCount; ++faceIndex) {
			if ((requiredFaces & static_cast<FaceMask>(1u << faceIndex)) != 0) {
				requiredFaceIndices[requiredFaceCount] = faceIndex;
				++requiredFaceCount;
			}
		}
		std::uniform_int_distribution<int> requiredFaceDistribution(
		    0, requiredFaceCount - 1);
		result = static_cast<FaceMask>(
		    1u << requiredFaceIndices[requiredFaceDistribution(randomEngine_)]);
	}

	while (std::popcount(static_cast<unsigned int>(result)) < obstacleCount) {
		result = static_cast<FaceMask>(
		    result | static_cast<FaceMask>(
		                 1u << faceDistribution(randomEngine_)));
	}
	return result;
}

LevelGenerator::FaceMask LevelGenerator::TransformFaceMask(
    FaceMask faceMask, int faceOffset, bool isMirrored) const {
	FaceMask transformedMask = 0;
	for (int faceIndex = 0; faceIndex < kSolverFaceCount; ++faceIndex) {
		if ((faceMask & static_cast<FaceMask>(1u << faceIndex)) == 0) {
			continue;
		}
		const SolverFace transformedFace = TransformSolverFace(
		    faceIndex, faceOffset, isMirrored);
		transformedMask = static_cast<FaceMask>(
		    transformedMask |
		    static_cast<FaceMask>(1u << ToFaceIndex(transformedFace)));
	}
	return transformedMask;
}

SolverFace LevelGenerator::TransformSolverFace(
    int faceIndex, int faceOffset, bool isMirrored) const {
	int transformedFace = faceIndex;
	if (isMirrored) {
		transformedFace =
		    (kSolverFaceCount - transformedFace) % kSolverFaceCount;
	}
	transformedFace =
	    (transformedFace + faceOffset) % kSolverFaceCount;
	return static_cast<SolverFace>(transformedFace);
}

BlockFace LevelGenerator::ConvertSolverFaceToBlockFace(
    SolverFace solverFace) const {
	constexpr std::array<BlockFace, kSolverFaceCount> blockFaces = {
	    BlockFace::Top,
	    BlockFace::Bottom,
	    BlockFace::Front,
	    BlockFace::Back,
	};
	constexpr std::array<Vector3, kSolverFaceCount> localNormals = {
	    Vector3{0.0f, 1.0f, 0.0f},
	    Vector3{0.0f, -1.0f, 0.0f},
	    Vector3{0.0f, 0.0f, 1.0f},
	    Vector3{0.0f, 0.0f, -1.0f},
	};

	const float rotationX =
	    -static_cast<float>(ToFaceIndex(solverFace)) *
	    std::numbers::pi_v<float> * 0.5f;
	const Matrix4x4 rotation = MathUtility::MakeRotateXMatrix(rotationX);

	BlockFace playerFacingBlockFace = blockFaces.front();
	float highestWorldY = -1.0f;
	for (std::size_t index = 0; index < blockFaces.size(); ++index) {
		Vector3 worldNormal =
		    MathUtility::TransformNormal(localNormals[index], rotation);
		MathUtility::Normalize(worldNormal);
		if (worldNormal.y > highestWorldY) {
			highestWorldY = worldNormal.y;
			playerFacingBlockFace = blockFaces[index];
		}
	}
	return playerFacingBlockFace;
}

bool LevelGenerator::HasMultipleObstacleGroups(
    const PatternDefinition& pattern) const {
	int groupCount = 0;
	bool previousBlockWasOccupied = false;
	for (FaceMask faceMask : pattern) {
		const bool isOccupied = faceMask != 0;
		if (isOccupied && !previousBlockWasOccupied) {
			++groupCount;
		}
		previousBlockWasOccupied = isOccupied;
	}
	return groupCount >= 2;
}
