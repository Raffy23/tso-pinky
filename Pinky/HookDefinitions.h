#pragma once
#include <windows.h>
#include <WinInet.h>
#include "HookUtils.h"

namespace pinky {

	namespace hook {

		namespace wininet {
			typedef function<BOOLEAN, HINTERNET, LPVOID, DWORD, LPDWORD> InternetReadFile;
			typedef function<BOOLEAN, HINTERNET, LPDWORD, DWORD, DWORD_PTR> InternetQueryDataAvailable;

		}

		namespace kernel32 {
			typedef function<HANDLE, LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD, DWORD> CreateMutexExW;
			typedef function<BOOL, LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION> CreateProcessW;
		}

		struct WinInet_t {
			wininet::InternetReadFile InternetReadFile = nullptr;
			wininet::InternetQueryDataAvailable InternetQueryDataAvailable = nullptr;
		};

		struct Kernel32_t {
			kernel32::CreateMutexExW CreateMutexExW = nullptr;
			kernel32::CreateProcessW CreateProcessW = nullptr;
		};

	}

}