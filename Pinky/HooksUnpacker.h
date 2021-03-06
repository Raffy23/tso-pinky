#pragma once
#include <Windows.h>
#include"HookDefinitions.h"

namespace pinky {
	namespace hook {

		extern DWORD __stdcall UnpackerHookEntryPoint(LPVOID lpThreadParameter);

		/**
		 * Function for hooking the TSO unpacker, hooks the CreateMutex functions
		 * to enable multiple TSO instances.
		 */
		extern bool hookUnpacker(bool isSecondInstance);

		extern Kernel32_t kernel32Func;

		HANDLE DetourCreateMutexExW(
			LPSECURITY_ATTRIBUTES lpMutexAttributes,
			LPCWSTR               lpName,
			DWORD                 dwFlags,
			DWORD                 dwDesiredAccess
		);

		BOOL DetourCreateProcessW(
			LPCWSTR               lpApplicationName,
			LPWSTR                lpCommandLine,
			LPSECURITY_ATTRIBUTES lpProcessAttributes,
			LPSECURITY_ATTRIBUTES lpThreadAttributes,
			BOOL                  bInheritHandles,
			DWORD                 dwCreationFlags,
			LPVOID                lpEnvironment,
			LPCWSTR               lpCurrentDirectory,
			LPSTARTUPINFOW        lpStartupInfo,
			LPPROCESS_INFORMATION lpProcessInformation
		);

		BOOL DetourCopyFileExW(
			LPCWSTR            lpExistingFileName,
			LPCWSTR            lpNewFileName,
			LPPROGRESS_ROUTINE lpProgressRoutine,
			LPVOID             lpData,
			LPBOOL             pbCancel,
			DWORD              dwCopyFlags
		);
	}
}

