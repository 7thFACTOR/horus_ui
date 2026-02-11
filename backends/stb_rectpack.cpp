#include "stb_rectpack.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
#include <vector>

namespace hui
{
bool packRects(PackedRect* rects, size_t rectCount, u32 atlasWidth, u32 atlasHeight)
{
	stbrp_context rpctx = {};
	std::vector<stbrp_rect> stbrects;
	std::vector<stbrp_node> nodes;
	
	nodes.resize(atlasWidth);
	stbrp_init_target(&rpctx, atlasWidth, atlasHeight, nodes.data(), nodes.size());

	for (size_t i = 0; i < rectCount; ++i)
	{
		stbrp_rect rc;
		rc.w = rects[i].rect.width;
		rc.h = rects[i].rect.height;
		stbrects.push_back(rc);
	}

	auto ret = stbrp_pack_rects(&rpctx, stbrects.data(), stbrects.size());

	for (size_t i = 0; i < rectCount; ++i)
	{
		rects[i].rect.x = stbrects[i].x;
		rects[i].rect.y = stbrects[i].y;
		rects[i].packedOk = stbrects[i].was_packed;
	}

	return ret == 1;
}

void initStbRectPack(Services& services)
{
	services.packRects = packRects;
}

void shutdownStbRectPack(Services& services)
{
	services.packRects = nullptr;
}

}