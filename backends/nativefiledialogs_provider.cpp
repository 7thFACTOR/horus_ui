#include "nativefiledialogs_provider.h"
#include <nfd.h>

namespace hui
{
bool NativeFileDialogsProvider::openFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize)
{
	char* path = 0;
	auto res = NFD_OpenDialog(filterList, defaultPath, &path);

	// we clear the events, some get pushed when dialog is open
	clearInputEventQueue();

	if (res == NFD_OKAY)
	{
		memcpy(outPath, path, std::min((int)maxOutPathSize, (int)strlen(path) + 1));
	}

	delete[] path;

	if (res == NFD_ERROR || res == NFD_CANCEL)
		return false;

	return true;
}

bool NativeFileDialogsProvider::openMultipleFileDialog(const char* filterList, const char* defaultPath, OpenMultipleFileSet& outPathSet)
{
	nfdpathset_t outPaths;
	auto res = NFD_OpenDialogMultiple(filterList, defaultPath, &outPaths);

	clearInputEventQueue();

	if (res != NFD_OKAY)
		return false;

	outPathSet.filenameBuffer = outPaths.buf;
	outPathSet.count = outPaths.count;
	outPathSet.bufferIndices = outPaths.indices;

	return true;
}

bool NativeFileDialogsProvider::saveFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize)
{
	char* path = 0;
	auto res = NFD_SaveDialog(filterList, defaultPath, &path);

	clearInputEventQueue();

	if (res != NFD_OKAY)
		return false;

	memcpy(outPath, path, std::min((int)maxOutPathSize, (int)strlen(path) + 1));
	delete[] path;

	return true;
}

bool NativeFileDialogsProvider::pickFolderDialog(const char* defaultPath, char* outPath, u32 maxOutPathSize)
{
	char* path = 0;
	auto res = NFD_PickFolder(defaultPath, &path);

	clearInputEventQueue();

	if (res != NFD_OKAY)
		return false;

	memcpy(outPath, path, std::min((int)maxOutPathSize, (int)strlen(path) + 1));
	delete[] path;

	return true;
}

}