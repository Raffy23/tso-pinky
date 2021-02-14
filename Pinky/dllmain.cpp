#include "pch.h"
#include "pinky.h"
#include "HooksAirClient.h"
#include "HooksUnpacker.h"
#include "Utils.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {

    case DLL_PROCESS_ATTACH:
        pinky::dllModule = hModule;
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        if (pinky::isMHInitalized.load())
            MH_Uninitialize();

        break;
    }

    return TRUE;
}

/* Exported methods */

DWORD pinky_hookUnpacker(PinkyConfig* config) {

    auto tsoProcInfo = reinterpret_cast<pinky::ProcessInfo*>(std::calloc(1, sizeof(pinky::ProcessInfo)));
    if (tsoProcInfo == nullptr)
        pinky::utils::perror("calloc(ProcessInfo)");

    std::string cmdline = std::string(config->tso) + " \"" + config->uri + "\"";
    LPSTR cmdlinePtr = (LPSTR)cmdline.c_str();

    if (!CreateProcessA(
        config->tso,                            // Module name
        cmdlinePtr,                             // Command line
        NULL,                                   // process handle
        NULL,                                   // thread handle
        FALSE,                                  // handle inheritance
        CREATE_SUSPENDED | CREATE_NEW_CONSOLE | DEBUG_PROCESS,  // creation flags
        NULL,                                   // parents env block
        config->cwd,                            // working directory
        &tsoProcInfo->si,
        &tsoProcInfo->pi)
        ) {
        pinky::utils::perrorWin32("CreateProcessA");
    }

    const auto result = pinky::hookProcess(tsoProcInfo, 0x0000126d0, pinky::hook::UnpackerHookEntryPoint, const_cast<char *>(config->uri), strlen(config->uri));
    pinky::utils::shouldDisplayMessageBox = true;

    return result;
}