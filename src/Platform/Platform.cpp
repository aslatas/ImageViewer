
// Generic platform API, which all platform layers need to provide.
#include "Platform/Platform.h"

// Windows platform layer.
#ifdef _WIN32

// Include as little of windows as possible.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

// TODO(Matt): Link this library dynamically instead.
#pragma comment(lib, "Ole32.lib")

// Needed for file dialogs.
#include <shobjidl_core.h>

// File I/O and open/new/save dialogs.
#include "Platform/Win32/Win32File.cpp"

// Stream I/O.
#include "Platform/Win32/Win32Logger.cpp"

// Entry point and window message handler.
#include "Platform/Win32/Win32Main.cpp"

#endif