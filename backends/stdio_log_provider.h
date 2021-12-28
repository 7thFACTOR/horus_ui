#pragma once
#include <horus_interfaces.h>

namespace hui
{
struct StdioLogProvider : LogProvider
{
	void log(const char* msg, LogType logType) override;
};

}