#include "stb_rectpack_provider.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
#include <vector>

namespace hui
{
struct StbRectPackProxy
{
	stbrp_context ctx;
	std::vector<stbrp_rect> rects;
	std::vector<stbrp_node> nodes;
};

HRectPacker StbRectPackProvider::createRectPacker()
{
	return new StbRectPackProxy();
}

void StbRectPackProvider::deleteRectPacker(HRectPacker packer)
{
	delete (StbRectPackProxy*)packer;
}

void StbRectPackProvider::reset(HRectPacker packer, u32 atlasWidth, u32 atlasHeight)
{
	StbRectPackProxy* stbPacker = (StbRectPackProxy*)packer;
	stbPacker->rects.clear();
	stbPacker->nodes.resize(atlasWidth);
	stbrp_init_target(&stbPacker->ctx, atlasWidth, atlasHeight, stbPacker->nodes.data(), stbPacker->nodes.size());
}

bool StbRectPackProvider::packRects(HRectPacker packer, PackRect* rects, size_t rectCount)
{
	StbRectPackProxy* stbPacker = (StbRectPackProxy*)packer;

	stbPacker->rects.clear();

	for (size_t i = 0; i < rectCount; ++i)
	{
		stbrp_rect rc;
		rc.w = rects[i].rect.width;
		rc.h = rects[i].rect.height;
		stbPacker->rects.push_back(rc);
	}

	auto ret = stbrp_pack_rects(&stbPacker->ctx, stbPacker->rects.data(), stbPacker->rects.size());

	for (size_t i = 0; i < rectCount; ++i)
	{
		rects[i].rect.x = stbPacker->rects[i].x;
		rects[i].rect.y = stbPacker->rects[i].y;
		rects[i].packedOk = stbPacker->rects[i].was_packed;
	}

	return ret == 1;
}

}