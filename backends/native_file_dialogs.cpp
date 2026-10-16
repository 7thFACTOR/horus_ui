#include "native_file_dialogs.h"
#include <nfd.h>
#include <stdlib.h>
#include <string.h>

namespace hui
{
// nfd allocates with malloc, so it must be released with free
OpenMultipleFileSet::~OpenMultipleFileSet()
{
	free(filenameBuffer);
	free(bufferIndices);
}

// copies an nfd path, always null terminating the destination
static void copyPath(char* outPath, u32 maxOutPathSize, const char* path)
{
	if (!maxOutPathSize)
		return;

	size_t len = strlen(path);

	if (len >= maxOutPathSize)
		len = maxOutPathSize - 1;

	memcpy(outPath, path, len);
	outPath[len] = 0;
}

bool openFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize)
{
	char* path = 0;
	auto res = NFD_OpenDialog(filterList, defaultPath, &path);

	// we clear the events, some get pushed when dialog is open
	inputEventClearQueue();

	if (res == NFD_OKAY)
	{
		copyPath(outPath, maxOutPathSize, path);
	}

	free(path);

	if (res == NFD_ERROR || res == NFD_CANCEL)
		return false;

	return true;
}

bool openMultipleFileDialog(const char* filterList, const char* defaultPath, OpenMultipleFileSet& outPathSet)
{
	nfdpathset_t outPaths;
	auto res = NFD_OpenDialogMultiple(filterList, defaultPath, &outPaths);

	inputEventClearQueue();

	if (res != NFD_OKAY)
		return false;

	// release paths from a previous call, they would be leaked otherwise
	free(outPathSet.filenameBuffer);
	free(outPathSet.bufferIndices);

	outPathSet.filenameBuffer = outPaths.buf;
	outPathSet.count = outPaths.count;
	outPathSet.bufferIndices = outPaths.indices;

	return true;
}

bool saveFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize)
{
	char* path = 0;
	auto res = NFD_SaveDialog(filterList, defaultPath, &path);

	inputEventClearQueue();

	if (res != NFD_OKAY)
		return false;

	copyPath(outPath, maxOutPathSize, path);
	free(path);

	return true;
}

bool pickFolderDialog(const char* defaultPath, char* outPath, u32 maxOutPathSize)
{
	char* path = 0;
	auto res = NFD_PickFolder(defaultPath, &path);

	inputEventClearQueue();

	if (res != NFD_OKAY)
		return false;

	copyPath(outPath, maxOutPathSize, path);
	free(path);

	return true;
}

}