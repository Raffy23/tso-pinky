#pragma once
#include <windows.h>
#include <WinInet.h>
#include "HookDefinitions.h"

namespace pinky {
    namespace hook {

        extern WinInet_t winInetFunc;

        /**
         * Function for hooking the Adobe AIR Client.
         * The function hooks into InternetReadFile from wininet.dll, which
         * is redirected to your own function that can return another picture
         * if needed.
         */
        extern bool hookAirClient();

        extern BOOLEAN DetourInternetReadFile(
            HINTERNET hFile,
            LPVOID    lpBuffer,
            DWORD     dwNumberOfBytesToRead,
            LPDWORD   lpdwNumberOfBytesRead
        );

        extern BOOLEAN DetourInternetQueryDataAvailable(
            HINTERNET hFile,
            LPDWORD lpdwNumberOfBytesAvailable,
            DWORD dwFlags,
            DWORD_PTR dwContext
        );

        extern DWORD __stdcall AdobeAirHookEntryPoint(LPVOID lpThreadParameter);

    }

}

