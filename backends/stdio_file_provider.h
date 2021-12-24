#pragma once
#include <horus_interfaces.h>

namespace hui
{
struct StdioFileProvider : FileProvider
{
	HFile open(const char* path, const char* mode) override;
	size_t read(HFile file, void* outData, size_t maxDataSize, size_t bytesToRead) override;
	size_t write(HFile file, void* data, size_t bytesToWrite) override;
	void close(HFile file) override;
	bool seek(HFile file, FileSeekMode mode, size_t pos = 0) override;
	size_t tell(HFile file) override;
};

}