#include "binpack_rectpack_provider.h"
#include <binpack/Rect.h>

namespace hui
{
struct BinPackProxy
{
	//GuillotineBinPack guillotineBinPack;
	//MaxRectsBinPack maxRectsBinPack;
	//ShelfBinPack shelfBinPack;
	SkylineBinPack skylineBinPack;
};

HRectPacker BinPackRectPackProvider::createRectPacker()
{
	return new BinPackProxy();
}

void BinPackRectPackProvider::deleteRectPacker(HRectPacker packer)
{
	delete (BinPackProxy*)packer;
}

void BinPackRectPackProvider::reset(HRectPacker packer, u32 atlasWidth, u32 atlasHeight)
{
	((BinPackProxy*)packer)->skylineBinPack.Init(atlasWidth, atlasHeight, true);
}

bool BinPackRectPackProvider::packRect(HRectPacker packer, u32 width, u32 height, hui::Rect& outPackedRect)
{
	auto rc = ((BinPackProxy*)packer)->skylineBinPack.Insert(
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