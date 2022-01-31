#pragma once
#include <horus.h>
#include <horus_interfaces.h>
#include "SkylineBinPack.h"

namespace hui
{
struct BinPackRectPackProvider : RectPackProvider
{
	HRectPacker createRectPacker() override;
	void deleteRectPacker(HRectPacker packer) override;
	void reset(HRectPacker packer, u32 atlasWidth, u32 atlasHeight) override;
	bool packRects(HRectPacker packer, PackRect* rects, size_t rectCount) override;
};

}