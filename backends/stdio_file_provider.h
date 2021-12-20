#pragma once
#include <horus_interfaces.h>

namespace hui
{
struct StdioFileProvider : FileProvider
{
	FileHandle open(const char* path, const char* mode) override;
	size_t read(FileHandle file, void* outData, size_t maxDataSize, size_t bytesToRead) override;
	size_t write(FileHandle file, void* data, size_t bytesToWrite) override;
	void close(FileHandle file) override;
	bool seek(FileHandle file, FileSeekMode mode, size_t pos = 0) override;
	size_t tell(FileHandle file) override;
};

}