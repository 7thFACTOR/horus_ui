#include "binpack_rectpack_provider.h"
#include <binpack/Rect.h>

namespace hui
{
void BinPackRectPackProvider::reset(u32 atlasWidth, u32 atlasHeight)
{
	skylineBinPack.Init(atlasWidth, atlasHeight, true);
}

bool BinPackRectPackProvider::packRect(u32 width, u32 height, hui::Rect& outPackedRect)
{
	auto rc = skylineBinPack.Insert(
		width,
		height,
		SkylineBinPack::LevelChoiceHeuristic::LevelMinWasteFit);

	if (rc.width && rc.height)
	{
		outPackedRect.x = rc.x;
		outPackedRect.y = rc.y;
		outPackedRect.width = rc.width;
		outPackedRect.height = rc.height;
		return true;
	}

	return false;
}

}