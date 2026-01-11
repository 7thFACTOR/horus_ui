#pragma once
#include <horus.h>

namespace hui
{
struct StbRectPackProvider : RectPackProvider
{
	HRectPacker createRectPacker() override;
	void deleteRectPacker(HRectPacker packer) override;
	void reset(HRectPacker packer, u32 atlasWidth, u32 atlasHeight) override;
	bool packRects(HRectPacker packer, PackedRect* rects, size_t rectCount) override;
};

}
