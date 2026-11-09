#pragma once
#include <horus.h>

namespace hui
{
struct OpenMultipleFileSet
{
	char* filenameBuffer = nullptr;
	size_t* bufferIndices = nullptr;
	u32 count = 0;

	~OpenMultipleFileSet();
};

bool openFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize);
bool openMultipleFileDialog(const char* filterList, const char* defaultPath, OpenMultipleFileSet& outPathSet);
bool saveFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize);
bool pickFolderDialog(const char* defaultPath, char* outPath, u32 maxOutPathSize);
}