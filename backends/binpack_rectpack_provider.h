#pragma once
#include <horus.h>
#include <horus_interfaces.h>
#include "SkylineBinPack.h"

namespace hui
{
struct BinPackRectPackProvider : RectPackProvider
{
	void reset(u32 atlasWidth, u32 atlasHeight) override;
	bool packRect(u32 width, u32 height, hui::Rect& outPackedRect) override;

private:
	//GuillotineBinPack guillotineBinPack;
	//MaxRectsBinPack maxRectsBinPack;
	//ShelfBinPack shelfBinPack;
	SkylineBinPack skylineBinPack;
};

}
