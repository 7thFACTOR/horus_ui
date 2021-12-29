#include "stdio_file_provider.h"

namespace hui
{
HFile StdioFileProvider::open(const char* path, const char* mode)
{
	return fopen(path, mode);
}

size_t StdioFileProvider::read(HFile file, void* outData, size_t bytesToRead)
{
#ifdef _WINDOWS
	return fread_s(outData, bytesToRead, bytesToRead, 1, (FILE*)file) * bytesToRead;
#else
	return fread(outData, bytesToRead, 1, (FILE*)file) * bytesToRead;
#endif
}

size_t StdioFileProvider::write(HFile file, void* data, size_t bytesToWrite)
{
	return fwrite(data, bytesToWrite, 1, (FILE*)file) * bytesToWrite;
}

void StdioFileProvider::close(HFile file)
{
	fclose((FILE*)file);
}

bool StdioFileProvider::seek(HFile file, FileSeekMode mode, size_t pos)
{
#ifdef _WIN64
	return 0 == _fseeki64((FILE*)file, pos, (int)mode);
#else
	return 0 == fseek((FILE*)file, pos, (int)mode);
#endif
}

size_t StdioFileProvider::tell(HFile file)
{
#ifdef _WIN64
	return _ftelli64((FILE*)file);
#else
	return ftell((FILE*)file, pos, (int)mode);
#endif
}

}