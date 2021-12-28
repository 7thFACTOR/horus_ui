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

bool BinPackRectPackProvider::packRects(HRectPacker packer, PackRect* rects, size_t rectCount)
{
	auto binPacker = (BinPackProxy*)packer;
	bool allPackedOk = true;

	for (size_t i = 0; i < rectCount; ++i)
	{
		auto rc = binPacker->skylineBinPack.Insert(
			rects[i].rect.width,
			rects[i].rect.height,
			SkylineBinPack::LevelChoiceHeuristic::LevelMinWasteFit);
		rects[i].rect.x = rc.x;
		rects[i].rect.y = rc.y;
		rects[i].rect.width = rc.width;
		rects[i].rect.height = rc.height;
		rects[i].packedOk = (rc.width && rc.height);

		if (!rects[i].packedOk)
		{
			allPackedOk = false;
		}
	}

	return allPackedOk;
}

}