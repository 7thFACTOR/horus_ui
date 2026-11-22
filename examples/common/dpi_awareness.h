#pragma once

// Force per-monitor DPI awareness as early as possible, before SDL or any window is created.
// The embedded manifest already does this, but an explicit call also covers stale
// per-exe Windows compatibility overrides and launchers that don't honor the manifest.

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace horusExamples
{
	inline void makeProcessDpiAware()
	{
		// DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == ((DPI_AWARENESS_CONTEXT)-4)
		auto perMonitorV2 = reinterpret_cast<void*>(static_cast<intptr_t>(-4));

		HMODULE user32 = LoadLibraryW(L"user32.dll");
		if (user32)
		{
			using SetContextFn = BOOL(WINAPI*)(void*);
			auto setContext = reinterpret_cast<SetContextFn>(
				GetProcAddress(user32, "SetProcessDpiAwarenessContext"));

			if (setContext && setContext(perMonitorV2))
			{
				FreeLibrary(user32);
				return;
			}

			HMODULE shcore = LoadLibraryW(L"shcore.dll");
			if (shcore)
			{
				// PROCESS_PER_MONITOR_DPI_AWARE == 2
				using SetAwarenessFn = HRESULT(WINAPI*)(int);
				auto setAwareness = reinterpret_cast<SetAwarenessFn>(
					GetProcAddress(shcore, "SetProcessDpiAwareness"));

				if (setAwareness && SUCCEEDED(setAwareness(2)))
				{
					FreeLibrary(shcore);
					FreeLibrary(user32);
					return;
				}

				FreeLibrary(shcore);
			}

			// The parent process can force a DPI awareness context onto a child
			// (Total Commander does this). If the process-level calls failed because
			// a context was already assigned, override it for this thread, which is
			// the thread SDL creates the window on.
			using SetThreadContextFn = void* (WINAPI*)(void*);
			auto setThreadContext = reinterpret_cast<SetThreadContextFn>(
				GetProcAddress(user32, "SetThreadDpiAwarenessContext"));

			if (setThreadContext && setThreadContext(perMonitorV2))
			{
				FreeLibrary(user32);
				return;
			}

			using SetDpiAwareFn = BOOL(WINAPI*)();
			auto setDpiAware = reinterpret_cast<SetDpiAwareFn>(
				GetProcAddress(user32, "SetProcessDPIAware"));

			if (setDpiAware)
				setDpiAware();

			FreeLibrary(user32);
		}
	}

}

#else

namespace horusExamples
{
	inline void makeProcessDpiAware()
	{
	}
}

#endif
