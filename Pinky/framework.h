#pragma once

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Komponenten aus Windows-Headern ausschließen

// Windows-Headerdateien
#include <Windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <WinInet.h>

#include <array>
#include <atomic>
#include <functional>
#include <vector>
#include <regex>

#include <MinHook.h>

#include "HookDefinitions.h"