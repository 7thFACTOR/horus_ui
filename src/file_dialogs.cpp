#include "horus_interfaces.h"

namespace hui
{
bool openFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize)
{
	return HORUS_FILEDIALOGS->openFileDialog(filterList, defaultPath, outPath, maxOutPathSize);
}

bool openMultipleFileDialog(const char* filterList, const char* defaultPath, OpenMultipleFileSet& outPathSet)
{
	return HORUS_FILEDIALOGS->openMultipleFileDialog(filterList, defaultPath, outPathSet);
}

bool saveFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize)
{
	return HORUS_FILEDIALOGS->saveFileDialog(filterList, defaultPath, outPath, maxOutPathSize);
}

bool pickFolderDialog(const char* defaultPath, char* outPath, u32 maxOutPathSize)
{
	return HORUS_FILEDIALOGS->pickFolderDialog(defaultPath, outPath, maxOutPathSize);
}

}