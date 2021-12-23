#pragma once
#include <horus.h>
#include <horus_interfaces.h>

namespace hui
{
struct NativeFileDialogsProvider : FileDialogsProvider
{
	bool openFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize) override;
	bool openMultipleFileDialog(const char* filterList, const char* defaultPath, OpenMultipleFileSet& outPathSet) override;
	bool saveFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize) override;
	bool pickFolderDialog(const char* defaultPath, char* outPath, u32 maxOutPathSize) override;
};
}