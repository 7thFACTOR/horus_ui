#include "types.h"
#include "context.h"
#include "docking.h"
#include <cstring>

namespace hui
{
constexpr size_t writeGrowSize = 1024;

// recursively gathers a dock node, its tab windows and all child nodes so each
// object can be released exactly once during teardown
static void collectDockNodeObjects(DockNode* node, std::unordered_set<DockNode*>& nodes, std::unordered_set<Window*>& windows)
{
	if (!node)
		return;

	nodes.insert(node);

	for (auto& wnd : node->windows)
		windows.insert(wnd);

	for (auto& child : node->children)
		collectDockNodeObjects(child, nodes, windows);
}

DockingState::~DockingState()
{
	// collect every reachable window and dock node, including the ones already
	// scheduled for deferred deletion, then free each one once
	std::unordered_set<Window*> windowsToDestroy = windowsToDelete;
	std::unordered_set<DockNode*> dockNodesToDestroy = dockNodesToDelete;

	for (auto& wnd : windows)
		windowsToDestroy.insert(wnd.second);

	for (auto& pair : rootNativeWindowDockNodes)
		collectDockNodeObjects(pair.second, dockNodesToDestroy, windowsToDestroy);

	for (auto& wnd : windowsToDestroy)
		delete wnd;

	for (auto& node : dockNodesToDestroy)
		delete node;

	// the native windows themselves are owned by the platform services
	if (ctx)
	{
		for (auto& pair : rootNativeWindowDockNodes)
			ctx->settings.services.destroyWindow(pair.first);
	}
}

bool MemoryStream::beginWrite()
{
	currentMode = Mode::Write;
	currentOffset = 0;
	return true;
}

bool MemoryStream::beginRead(const u8* data, size_t dataSize)
{
	currentOffset = 0;
	this->data.clear();
	this->data.insert(this->data.begin(), data, data + dataSize);
	currentMode = Mode::Read;

	return true;
}

void MemoryStream::writeData(const u8* data, size_t dataSize)
{
	if (this->data.capacity() < this->data.size() + dataSize)
	{
		this->data.reserve(this->data.size() + dataSize + writeGrowSize);
	}

	this->data.insert(this->data.end(), data, data + dataSize);
}

bool MemoryStream::readData(u8* outData, size_t dataSize)
{
	if (currentOffset + dataSize > data.size())
	{
		// out of buffer
		return false;
	}

	std::memcpy(outData, &data[currentOffset], dataSize);
	currentOffset += dataSize;

	return true;
}

}