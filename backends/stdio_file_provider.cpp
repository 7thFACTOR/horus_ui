#include "stdio_file_provider.h"

namespace hui
{
static HFile open(const char* path, const char* mode)
{
	return fopen(path, mode);
}

static size_t read(HFile file, void* outData, size_t bytesToRead)
{
#ifdef _WINDOWS
	return fread_s(outData, bytesToRead, bytesToRead, 1, (FILE*)file) * bytesToRead;
#else
	return fread(outData, bytesToRead, 1, (FILE*)file) * bytesToRead;
#endif
}

static size_t write(HFile file, void* data, size_t bytesToWrite)
{
	return fwrite(data, bytesToWrite, 1, (FILE*)file) * bytesToWrite;
}

static void close(HFile file)
{
	fclose((FILE*)file);
}

static bool seek(HFile file, FileSeekMode mode, size_t pos)
{
#ifdef _WIN64
	return 0 == _fseeki64((FILE*)file, pos, (int)mode);
#else
	return 0 == fseek((FILE*)file, pos, (int)mode);
#endif
}

static size_t tell(HFile file)
{
#ifdef _WIN64
	return _ftelli64((FILE*)file);
#else
	return ftell((FILE*)file);
#endif
}

void initStdioFileService(Services& services)
{
	services.open = open;
	services.read = read;
	services.write = write;
	services.close = close;
	services.seek = seek;
	services.tell = tell;
}

void shutdownStdioFileService(Services& services)
{
	services.open = nullptr;
	services.read = nullptr;
	services.write = nullptr;
	services.close = nullptr;
	services.seek = nullptr;
	services.tell = nullptr;
}

}