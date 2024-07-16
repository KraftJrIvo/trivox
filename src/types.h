#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <vector>
#include <cstdint>
#include <vector>
#include <list>
#include <map>
#include <fstream>

#include "raylib.h"

namespace trivox {

	struct Triangle { Vector3 a, b, c; };
	struct TriangleLocator { uint32_t offset; uint32_t idx; };
	typedef uint32_t TriangleLocatorIdx;
	typedef Vector3 PackedTriangle3d;
	typedef std::vector<PackedTriangle3d> PackedTriangle3dArray;

	struct RenderPyramidLevel {
		uint8_t pow;
		float rPrt;
		float blkSz;
		uint32_t r;
		uint32_t sz;
		std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, TriangleLocatorIdx>>> pyramid;
	};

	struct RenderPyramid {
		RenderPyramid(std::pair<int8_t, int8_t> minMaxPow, uint32_t radius);

		uint32_t radius;
		std::pair<int8_t, int8_t> minMaxPow;
		std::vector<RenderPyramidLevel> levels;
	};

	class Renderable {
		uint32_t getNtriangles(float detail = 1.0f);
		void getTriangles(PackedTriangle3dArray& trgs, RenderPyramid& pyr, uint32_t offset, float detail = 1.0f);
	};

}