#pragma once

#include "Obstacle.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <random>
#include <vector>

// プレイヤーの開始面を0度とし、Dキーで右回転したときに
// プレイヤー側へ来る面を90度、180度、270度と定義する。
enum class SolverFace : uint8_t {
	Spawn0 = 0,
	Right90 = 1,
	Right180 = 2,
	Right270 = 3,
};

enum class LevelDifficulty : uint8_t {
	Easy,
	Normal,
	Hard,
};

struct ObstacleSpawnPlan {
	SolverFace solverFace = SolverFace::Spawn0;
	BlockFace attachedFace = BlockFace::Top;
	KamataEngine::Vector3 size = {1.0f, 1.0f, 1.0f};
	ObstacleInteractionRules interactionRules = {};
};

// 1ブロックの異なる面に複数の障害物を配置できる。
struct MapBlockSpawnPlan {
	std::vector<ObstacleSpawnPlan> obstacles;
};

class LevelGenerator {
public:
	explicit LevelGenerator(
	    LevelDifficulty difficulty = LevelDifficulty::Normal);

	void Reset();
	void Reset(uint32_t seed);
	MapBlockSpawnPlan CreateInitialBlockPlan();
	MapBlockSpawnPlan CreateReplacementBlockPlan();

	uint32_t GetSeed() const { return seed_; }
	LevelDifficulty GetDifficulty() const { return difficulty_; }
	const char* GetDifficultyName() const;

private:
	using FaceMask = uint8_t;
	static inline constexpr std::size_t kPatternLength = 10;
	// 各面は10ブロックまで連続で空けられる。11ブロック目が空く候補は
	// 種子の境界をまたぐ場合も不採用にして生成し直す。
	static inline constexpr int kMaximumStraightClearBlocks = 10;
	// 次の10ブロック片で4面を順番に塞げるよう、末尾に3ブロック分の
	// 生成余裕を残す。
	static inline constexpr int kMaximumEndingClearStreak = 7;
	// 実機テストで採用した簡単難度の固定シード。
	static inline constexpr uint32_t kSavedEasySeed = 3747538539u;
	// 実機テストで採用した困難難度の固定シード。
	static inline constexpr uint32_t kSavedHardSeed = 3606607211u;
	using PatternDefinition = std::array<FaceMask, kPatternLength>;

	struct SimulationResult {
		FaceMask reachableFaces = 0;
		std::array<FaceMask, 2> recentObstacleMasks = {};
	};

	void QueueNextPattern();
	PatternDefinition GenerateCandidatePattern();
	bool TryQueuePattern(
	    const PatternDefinition& pattern, int faceOffset, bool isMirrored);
	SimulationResult SimulatePattern(
	    const PatternDefinition& pattern, int faceOffset, bool isMirrored,
	    FaceMask entryFaces,
	    const std::array<FaceMask, 2>& entryRecentMasks) const;
	FaceMask AdvanceReachableFaces(
	    FaceMask reachableFaces, FaceMask activeObstacleFaces) const;
	FaceMask CreateRandomFaceMask(int obstacleCount, FaceMask requiredFaces);
	FaceMask TransformFaceMask(
	    FaceMask faceMask, int faceOffset, bool isMirrored) const;
	SolverFace TransformSolverFace(
	    int faceIndex, int faceOffset, bool isMirrored) const;
	BlockFace ConvertSolverFaceToBlockFace(SolverFace solverFace) const;
	bool HasMultipleObstacleGroups(const PatternDefinition& pattern) const;

	std::deque<MapBlockSpawnPlan> pendingPlans_;
	std::mt19937 randomEngine_;
	uint32_t seed_ = 0;
	LevelDifficulty difficulty_ = LevelDifficulty::Normal;
	FaceMask reachableFaces_ = 0x01;
	std::array<FaceMask, 2> recentObstacleMasks_ = {};
	std::array<int, 4> straightClearBlockCounts_ = {};
	int consecutiveEmptyBlockCount_ = 0;
};
