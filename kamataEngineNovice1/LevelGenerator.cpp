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

struct DifficultySettings {
	bool usesGlobalSafeBuffer;
	int minimumBufferLength;
	int maximumBufferLength;
	int minimumOccupiedBlockCount;
	int maximumOccupiedBlockCount;
	int maximumConsecutiveEmptyBlockCount;
	std::array<double, 3> obstacleCountWeights;
	double continueWallProbability;
};

DifficultySettings GetDifficultySettings(LevelDifficulty difficulty) {
	switch (difficulty) {
	case LevelDifficulty::Easy:
		// 普通難度より余裕を残しつつ、障害物を以前より近く頻繁に出す。
		return {true, 2, 3, 4, 5, 7, {70.0, 30.0, 0.0}, 0.30};
	case LevelDifficulty::Normal:
		// 障害物の最低出現数だけを増やし、頻度を少し高くする。
		return {true, 2, 3, 5, 6, 5, {45.0, 35.0, 20.0}, 0.45};
	case LevelDifficulty::Hard:
		// 困難難度は全体空白バッファを使わず、各面の安全距離で制限する。
		return {false, 0, 0, 6, 7, 0, {25.0, 40.0, 35.0}, 0.60};
	}
	return {true, 2, 3, 4, 6, 5, {45.0, 35.0, 20.0}, 0.45};
}

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
	if (difficulty_ == LevelDifficulty::Easy) {
		Reset(kSavedEasySeed);
		return;
	}
	if (difficulty_ == LevelDifficulty::Normal) {
		Reset(kSavedNormalSeed);
		return;
	}
	if (difficulty_ == LevelDifficulty::Hard) {
		Reset(kSavedHardSeed);
		return;
	}
}

void LevelGenerator::Reset(uint32_t seed) {
	seed_ = seed;
	randomEngine_.seed(seed_);
	// Obstacle types use an independent stream so choosing a slime never changes
	// the accepted seed's face layout or later random patterns.
	obstacleTypeRandomEngine_.seed(seed_ ^ kObstacleTypeSeedSalt);
	pendingPlans_.clear();
	// ゲーム開始時点のプレイヤー面は0度。
	reachableFaces_ = 0x01;
	recentObstacleMasks_ = {};
	straightClearBlockCounts_ = {};
	consecutiveEmptyBlockCount_ = 0;
	blocksSinceLastSlime_ = 0;
	RollNextSlimeInterval();
}

MapBlockSpawnPlan LevelGenerator::CreateInitialBlockPlan() {
	if (difficulty_ != LevelDifficulty::Easy) {
		++blocksSinceLastSlime_;
	}
	// 初期マップブロックは空。空ブロック中のA/D移動も求解状態へ反映する。
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

	// ランダム候補が成立しない場合は、空白を置かず4面を順番に塞ぐ。
	// 現在の到達面と直近2ブロックに適合する順番を全24通りから選ぶ。
	std::array<int, kSolverFaceCount> fallbackFaceOrder = {0, 1, 2, 3};
	do {
		PatternDefinition fallback = {};
		for (std::size_t blockIndex = 0; blockIndex < kPatternLength;
		     ++blockIndex) {
			const int orderIndex = static_cast<int>(
			    blockIndex % static_cast<std::size_t>(kSolverFaceCount));
			fallback[blockIndex] = static_cast<FaceMask>(
			    1u << fallbackFaceOrder[orderIndex]);
		}
		if (TryQueuePattern(fallback, 0, false)) {
			return;
		}
	} while (std::next_permutation(
	    fallbackFaceOrder.begin(), fallbackFaceOrder.end()));

	// TryQueuePatternの末尾余裕と継続可能性の検査により到達しない。
	assert(false);
}

LevelGenerator::PatternDefinition LevelGenerator::GenerateCandidatePattern() {
	const DifficultySettings settings = GetDifficultySettings(difficulty_);
	std::uniform_int_distribution<int> bufferDistribution(
	    settings.minimumBufferLength, settings.maximumBufferLength);
	std::uniform_int_distribution<int> occupiedBlockDistribution(
	    settings.minimumOccupiedBlockCount,
	    settings.maximumOccupiedBlockCount);
	std::discrete_distribution<int> obstacleCountDistribution(
	    settings.obstacleCountWeights.begin(),
	    settings.obstacleCountWeights.end());
	std::bernoulli_distribution continueWallDistribution(
	    settings.continueWallProbability);

	for (int attempt = 0; attempt < 64; ++attempt) {
		PatternDefinition pattern = {};
		const int bufferLength = settings.usesGlobalSafeBuffer
		                             ? bufferDistribution(randomEngine_)
		                             : 0;
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
	constexpr FaceMask face1 = 1u << 1;
	constexpr FaceMask face2 = 1u << 2;
	constexpr FaceMask face3 = 1u << 3;
	return {0, 0, face0, face1, face2, face3, 0, 0, face0, face2};
}

bool LevelGenerator::TryQueuePattern(
    const PatternDefinition& pattern, int faceOffset, bool isMirrored) {
	const DifficultySettings settings = GetDifficultySettings(difficulty_);
	int nextEmptyBlockCount = consecutiveEmptyBlockCount_;
	if (settings.usesGlobalSafeBuffer) {
		for (FaceMask rawFaceMask : pattern) {
			if (rawFaceMask == 0) {
				++nextEmptyBlockCount;
				if (nextEmptyBlockCount >
				    settings.maximumConsecutiveEmptyBlockCount) {
					return false;
				}
			} else {
				nextEmptyBlockCount = 0;
			}
		}
		// 次パターンが最短の安全間隔から始まっても生成可能な余裕を残す。
		const int maximumEndingEmptyBlockCount =
		    settings.maximumConsecutiveEmptyBlockCount -
		    settings.minimumBufferLength;
		if (nextEmptyBlockCount > maximumEndingEmptyBlockCount) {
			return false;
		}
	}

	std::array<int, kSolverFaceCount> nextClearBlockCounts =
	    straightClearBlockCounts_;
	for (FaceMask rawFaceMask : pattern) {
		const FaceMask transformedFaceMask =
		    TransformFaceMask(rawFaceMask, faceOffset, isMirrored);
		for (int faceIndex = 0; faceIndex < kSolverFaceCount; ++faceIndex) {
			const FaceMask faceBit = static_cast<FaceMask>(1u << faceIndex);
			if ((transformedFaceMask & faceBit) != 0) {
				nextClearBlockCounts[faceIndex] = 0;
				continue;
			}

			++nextClearBlockCounts[faceIndex];
			if (nextClearBlockCounts[faceIndex] >
			    kMaximumStraightClearBlocks) {
				// この面が11ブロック以上連続で空く候補は採用せず、
				// QueueNextPatternで別の並びを生成し直す。
				return false;
			}
		}
	}
	// 次の片で最も遅い面を3ブロック後に補う余裕を確保する。
	for (int clearBlockCount : nextClearBlockCounts) {
		if (clearBlockCount > kMaximumEndingClearStreak) {
			return false;
		}
	}

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
	// 現在の片を抜けた直後にも最低1面が残ることを確認する。
	// これがないと次回は最初の障害物を調べる前に全経路が消える。
	const FaceMask continuingFaces = AdvanceReachableFaces(
	    result.reachableFaces,
	    static_cast<FaceMask>(
	        result.recentObstacleMasks[0] |
	        result.recentObstacleMasks[1]));
	if (continuingFaces == 0) {
		return false;
	}

	std::vector<MapBlockSpawnPlan> queuedPlans;
	queuedPlans.reserve(kPatternLength);
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
		queuedPlans.push_back(std::move(blockPlan));
	}

	// Slime frequency is measured in map blocks. Once its seeded interval is
	// reached, wait for a single-obstacle block whose face is also clear on the
	// immediately preceding block. A normal obstacle may follow it, but the slime
	// itself will not be placed in the middle of an existing wall.
	auto createFaceMask = [](const MapBlockSpawnPlan& blockPlan) {
		FaceMask faceMask = 0;
		for (const ObstacleSpawnPlan& obstaclePlan : blockPlan.obstacles) {
			faceMask = static_cast<FaceMask>(
			    faceMask |
			    static_cast<FaceMask>(
			        1u << static_cast<int>(obstaclePlan.solverFace)));
		}
		return faceMask;
	};
	for (std::size_t blockIndex = 0; blockIndex < queuedPlans.size();
	     ++blockIndex) {
		if (difficulty_ == LevelDifficulty::Easy) {
			break;
		}
		++blocksSinceLastSlime_;
		MapBlockSpawnPlan& blockPlan = queuedPlans[blockIndex];
		if (blocksSinceLastSlime_ < nextSlimeInterval_ ||
		    blockPlan.obstacles.size() != 1) {
			continue;
		}

		const FaceMask slimeFace = static_cast<FaceMask>(
		    1u << static_cast<int>(
		              blockPlan.obstacles.front().solverFace));
		const FaceMask previousFaceMask =
		    blockIndex == 0 ? recentObstacleMasks_[0]
		                    : createFaceMask(queuedPlans[blockIndex - 1]);
		if ((previousFaceMask & slimeFace) != 0) {
			continue;
		}

		blockPlan.obstacles.front().type = ObstacleType::Slime;
		blocksSinceLastSlime_ = 0;
		RollNextSlimeInterval();
	}
	for (MapBlockSpawnPlan& blockPlan : queuedPlans) {
		pendingPlans_.push_back(std::move(blockPlan));
	}
	reachableFaces_ = result.reachableFaces;
	recentObstacleMasks_ = result.recentObstacleMasks;
	straightClearBlockCounts_ = nextClearBlockCounts;
	consecutiveEmptyBlockCount_ = nextEmptyBlockCount;
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

void LevelGenerator::RollNextSlimeInterval() {
	switch (difficulty_) {
	case LevelDifficulty::Easy:
		nextSlimeInterval_ = 0;
		return;
	case LevelDifficulty::Normal: {
		std::uniform_int_distribution<std::size_t> intervalDistribution(17, 20);
		nextSlimeInterval_ =
		    intervalDistribution(obstacleTypeRandomEngine_);
		return;
	}
	case LevelDifficulty::Hard: {
		std::uniform_int_distribution<std::size_t> intervalDistribution(12, 15);
		nextSlimeInterval_ =
		    intervalDistribution(obstacleTypeRandomEngine_);
		return;
	}
	}
}
