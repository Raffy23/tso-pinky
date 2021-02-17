#include "pch.h"
#include "pinky.h"
#include "HookDefinitions.h"
#include "HooksUnpacker.h"
#include "HooksAirClient.h"
#include "Utils.h"

std::string dsoUserID;
pinky::hook::Kernel32_t pinky::hook::kernel32Func;


LARGE_INTEGER getFileSize(const wchar_t* name);

DWORD __stdcall pinky::hook::UnpackerHookEntryPoint(LPVOID lpThreadParameter) {
    char *data = reinterpret_cast<char*>(lpThreadParameter);
    std::string uri(data);

    std::regex dsoUserIDRegex("dsoAuthUser=(\\d+)");
    std::smatch match;

    if (std::regex_search(uri, match, dsoUserIDRegex) && match.size() > 1) {
        dsoUserID = match.str(1);
    } else {
        MessageBox(nullptr, L"Could not parse the login URI!", L"Pinky", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    pinky::initalize();

    wchar_t* filename = new wchar_t[MAX_PATH];
    GetModuleFileNameW(GetModuleHandle(NULL), filename, MAX_PATH);

    std::wstring fName = filename;
    std::wstring procName = fName.substr(fName.find_last_of(L"\\") + 1);

    bool isRunning = true;
    if (isProcessRunning(procName.c_str())) {
        isRunning = true;
    }

    if (!pinky::hook::hookUnpacker(isRunning)) {
        MessageBox(nullptr, L"Something went wrong, can not hook The Settlers Online.exe!", L"Pinky", MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }

   
    delete[] filename;
    return EXIT_SUCCESS;
}

bool pinky::hook::hookUnpacker(bool isSecondInstance) {
    bool res4 = hookApi("kernel32.dll", "CreateMutexExW", DetourCreateMutexExW, &kernel32Func.CreateMutexExW);
    bool res5 = hookApi("kernel32.dll", "CreateProcessW", DetourCreateProcessW, &kernel32Func.CreateProcessW);
    bool res6 = true;
    
    if (isSecondInstance)
        res6 = hookApi("kernel32.dll", "CopyFileExW", DetourCopyFileExW, &kernel32Func.CopyFileExW);

    return res4 && res5 && res6;
}

HANDLE pinky::hook::DetourCreateMutexExW(LPSECURITY_ATTRIBUTES lpMutexAttributes, LPCWSTR lpName, DWORD dwFlags, DWORD dwDesiredAccess) {
    if (lstrcmpW(lpName, L"The Settlers Online") == 0) {
        auto newName = std::wstring(L"The Settlers Online") + std::wstring(dsoUserID.cbegin(), dsoUserID.cend());

        return pinky::hook::kernel32Func.CreateMutexExW(lpMutexAttributes, newName.c_str(), dwFlags, dwDesiredAccess);
    }

    return pinky::hook::kernel32Func.CreateMutexExW(lpMutexAttributes, lpName, dwFlags, dwDesiredAccess);
}

BOOL pinky::hook::DetourCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) {
    
    DWORD newCreationFlags = dwCreationFlags | CREATE_SUSPENDED | DEBUG_PROCESS;
    
    if (!pinky::hook::kernel32Func.CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, newCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation)) {
        return FALSE;
    }
    
    ProcessInfo clientEXE = { 0 };
    memcpy(&clientEXE.pi, lpProcessInformation, sizeof(PROCESS_INFORMATION));
    memcpy(&clientEXE.siw, lpStartupInfo, sizeof(STARTUPINFOW));
    
    if (hookProcess(&clientEXE, 0x0000013f0, AdobeAirHookEntryPoint, nullptr, 0) != EXIT_SUCCESS)
        utils::perrorMsgBox("Unable to hook client.exe!\nAborting ...");

    return TRUE;
}

BOOL pinky::hook::DetourCopyFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags) {
    /* do nothing and trust the first client that everything is updated */
    return TRUE;
}

