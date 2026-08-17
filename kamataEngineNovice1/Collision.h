#pragma once

#include "KamataEngine.h"

struct AABB {
	KamataEngine::Vector3 min;
	KamataEngine::Vector3 max;
};

inline AABB MakeAABB(
    const KamataEngine::Vector3& center,
    const KamataEngine::Vector3& halfSize) {
	return {
	    {
	        center.x - halfSize.x,
	        center.y - halfSize.y,
	        center.z - halfSize.z,
	    },
	    {
	        center.x + halfSize.x,
	        center.y + halfSize.y,
	        center.z + halfSize.z,
	    },
	};
}

inline bool IsCollision(const AABB& first, const AABB& second) {
	return first.min.x <= second.max.x && first.max.x >= second.min.x &&
	       first.min.y <= second.max.y && first.max.y >= second.min.y &&
	       first.min.z <= second.max.z && first.max.z >= second.min.z;
}
