#include "types.h"
#include <cstring>
#include "view_pane.h"

namespace hui
{
constexpr size_t writeGrowSize = 1024;

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

ViewPane* DockingState::getRootViewPaneOfWindow(HWindow window)
{
	for (auto& pane : rootViewPanes)
	{
		if (pane->window == window)
		{
			return pane;
		}
	}

	return nullptr;
}


}