#include "stdio_log_provider.h"
#include <string>

namespace hui
{
void StdioLogProvider::log(const char* msg, LogType logType)
{
	std::string msgType;

	switch (logType)
	{
	case LogProvider::LogType::Info: msgType = "[I]"; break;
	case LogProvider::LogType::Warn: msgType = "[W]"; break;
	case LogProvider::LogType::Error: msgType = "[E]"; break;
	case LogProvider::LogType::Debug: msgType = "[D]"; break;
	default:
		break;
	};

	printf("%s %s\n", msgType.c_str(), msg);
}

}