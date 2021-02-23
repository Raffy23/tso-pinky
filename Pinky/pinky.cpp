#include "pch.h"
#include<psapi.h>
#include<algorithm>

#include "pinky.h"
#include "resource.h"
#include "HooksAirClient.h"
#include "HooksUnpacker.h"
#include "Utils.h"

HMODULE pinky::dllModule = nullptr;
std::atomic_bool pinky::isMHInitalized(false);

BYTE* pinky::replacementImage = nullptr;
size_t pinky::replacementImageSize = 0;

bool pinky::initalize(bool loadImages)
{
    bool result = true;
    if (loadImages)
        result = loadDllResources();

    return MH_Initialize() && result;
}

extern bool pinky::loadDllResources()
{
    const HRSRC hResource = FindResource(dllModule, MAKEINTRESOURCE(PINK_SQUARE), RT_RCDATA);
    if (hResource == nullptr)
        return FALSE;

    const HGLOBAL hMemory = LoadResource(dllModule, hResource);
    if (hMemory == nullptr)
        return FALSE;

    const DWORD dwSize = SizeofResource(dllModule, hResource);
    LPVOID lpAddress = LockResource(hMemory);

    replacementImageSize = dwSize;
    replacementImage     = reinterpret_cast<BYTE*>(std::malloc(dwSize));
    if (replacementImage == nullptr)
        utils::perror("malloc");

    memcpy(replacementImage, lpAddress, dwSize);

    UnlockResource(hMemory);

    return true;
}

const std::array<BYTE, 1> invokeDebugger = { 0xCC /* int3 */ };

DWORD pinky::hookProcess(ProcessInfo* procInfo, uint64_t breakPointOffset, LPTHREAD_START_ROUTINE hookFunction, void* param, size_t paramSize) {

    // Get base address of suspended process
    PVOID imageBaseAddress = nullptr;
    {
        CONTEXT* threadContext = reinterpret_cast<CONTEXT*>(std::calloc(1, sizeof(CONTEXT)));
        if (threadContext == nullptr)
            utils::perrorWin32("calloc");

        threadContext->ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(procInfo->pi.hThread, threadContext))
            utils::perrorWin32("GetThreadContext");

#if defined _M_IX86
        ULONG_PTR* peb = (ULONG_PTR*)threadContext->Edx;
#elif defined _M_X64
        ULONG_PTR* peb = (ULONG_PTR*)threadContext->Rdx;
#endif
       
        if (!ReadProcessMemory(procInfo->pi.hProcess, static_cast<PVOID>(&peb[2]), &imageBaseAddress, sizeof(PVOID), NULL))
            pinky::utils::perrorWin32("ReadProcessMemory");

        std::free(threadContext);
    }
    
    // Now we place 'int3' at the binary startup and let it run until we hit it
    // then kernel32.dll should be loaded and we can inject the pinky.dll. 
    // Debugger is detached as soon as possible ...
    {
        const auto breakpointLocation = reinterpret_cast<BYTE*>(imageBaseAddress) + breakPointOffset;
        BYTE instructionBackup = 0x00;
        SIZE_T instBytesRead = 0;
        SIZE_T instBytesWritten = 0;

        if (!ReadProcessMemory(procInfo->pi.hProcess, breakpointLocation, &instructionBackup, sizeof(BYTE), &instBytesRead))
            pinky::utils::perrorWin32("ReadProcessMemory");

        if (!WriteProcessMemory(procInfo->pi.hProcess, breakpointLocation, invokeDebugger.data(), invokeDebugger.size(), &instBytesWritten))
            pinky::utils::perrorWin32("WriteProcessMemory");

        if (!FlushInstructionCache(procInfo->pi.hProcess, breakpointLocation, sizeof(BYTE)))
            pinky::utils::perrorWin32("WriteProcessMemory");

        // Resume thread and wait for it to hit the breakpoint
        ResumeThread(procInfo->pi.hThread);

        DEBUG_EVENT* dbgEvent = reinterpret_cast<DEBUG_EVENT*>(std::calloc(1, sizeof(DEBUG_EVENT)));

        do {
            WaitForDebugEvent(dbgEvent, INFINITE);

            if (dbgEvent->dwDebugEventCode != EXCEPTION_DEBUG_EVENT) {
                ContinueDebugEvent(dbgEvent->dwProcessId, dbgEvent->dwThreadId, DBG_CONTINUE);
            }

        } while (dbgEvent->dwDebugEventCode != EXCEPTION_DEBUG_EVENT && dbgEvent->u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT);

        // Restore instructions
        if (!WriteProcessMemory(procInfo->pi.hProcess, breakpointLocation, &instructionBackup, sizeof(BYTE), &instBytesWritten))
            pinky::utils::perrorWin32("WriteProcessMemory");
        
        if (!FlushInstructionCache(procInfo->pi.hProcess, breakpointLocation, sizeof(BYTE)))
            pinky::utils::perrorWin32("WriteProcessMemory");

        {
            CONTEXT* threadContext = reinterpret_cast<CONTEXT*>(std::calloc(1, sizeof(CONTEXT)));

            GetThreadContext(procInfo->pi.hThread, threadContext);

#if defined _M_IX86
            threadContext->Eip -= 1;
#elif defined _M_X64
            threadContext->Rip -= 1;
#endif
            
            SetThreadContext(procInfo->pi.hThread, threadContext);

            std::free(threadContext);
        }

        // Supsend Thread and detach debugger
        SuspendThread(procInfo->pi.hThread);
        if (!DebugActiveProcessStop(procInfo->pi.dwProcessId))
            pinky::utils::perrorWin32("DebugActiveProcessStop");

        std::free(dbgEvent);
    }
    
    // Inject Pinky.dll into process
    {
        char* pinkyDllPath = new char[MAX_PATH];

        if (GetModuleFileNameA(pinky::dllModule, pinkyDllPath, MAX_PATH) == 0)
            pinky::utils::perrorWin32("GetModuleFileNameA");
        
        const auto mKernel32 = GetModuleHandle(L"kernel32.dll");
        if (mKernel32 == nullptr)
            pinky::utils::perrorWin32("GetModuleHandle(kernel32.dll)");

        const auto LoadLibraryAPTR = GetProcAddress(mKernel32, "LoadLibraryA");
        if (LoadLibraryAPTR == nullptr)
            pinky::utils::perrorWin32("LoadLibraryA(kernel32.dll)");

        const auto remotePinkyDllPathMem = VirtualAllocEx(procInfo->pi.hProcess, NULL, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (remotePinkyDllPathMem == nullptr)
            pinky::utils::perrorWin32("VirtualAllocEx");

        if (WriteProcessMemory(procInfo->pi.hProcess, remotePinkyDllPathMem, pinkyDllPath, MAX_PATH, NULL) == 0)
            pinky::utils::perrorWin32("WriteProcessMemory");

        const auto hRemoteThread = CreateRemoteThread(procInfo->pi.hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibraryAPTR, (LPVOID)remotePinkyDllPathMem, NULL, NULL);
        if (hRemoteThread == INVALID_HANDLE_VALUE)
            pinky::utils::perrorWin32("CreateRemoteThread");

        // Now we wait until the pinky.dll is loaded, should be after the thread exited
        // BUG: if not ResumeThread / SuspendThread for the main thread the attachment fails (Status: WaitQueue:UserRequest)
        ResumeThread(procInfo->pi.hThread);
        WaitForSingleObject(hRemoteThread, INFINITE);
        SuspendThread(procInfo->pi.hThread);

        delete[] pinkyDllPath;
    }

    // Find Base Address of Pinky.dll in remote process
    HMODULE remotePinky = nullptr;
    BOOLEAN found = FALSE;
    {
        constexpr int HMODULE_COUNT = 2048;
        
        MODULEINFO moduleInfo;
        HMODULE* hModules = reinterpret_cast<HMODULE*>(std::calloc(HMODULE_COUNT, sizeof(HMODULE)));
        DWORD cbNeeded;
        if (!EnumProcessModules(procInfo->pi.hProcess, hModules, HMODULE_COUNT, &cbNeeded))
            pinky::utils::perrorWin32("EnumProcessModules");

        const auto modulesCount = cbNeeded / sizeof(HMODULE);
        for (auto i = 0; i < modulesCount; i++) {
            char filename[MAX_PATH];
            if (!GetModuleFileNameExA(procInfo->pi.hProcess, hModules[i], filename, sizeof(filename))) {
                continue;
            }

            const std::string moduleName = filename;
            if (moduleName.find("Pinky.dll") != std::string::npos) {
                if (!GetModuleInformation(procInfo->pi.hProcess, hModules[i], &moduleInfo, sizeof(moduleInfo))) {
                    continue;
                }

                //std::cout << "Found Pinky.dll: 0x" << static_cast<void*>(hModules[i]) << std::endl;
                remotePinky = hModules[i];
                break;
            }
        }

        std::free(hModules);
        if (remotePinky == nullptr)
            pinky::utils::perrorWin32("Pinky.dll is *not* attached to the target process!");
    }

    // Invoke the hooking function in the process
    {
        // Calculate the pointer of the hooking function in the remote thread
        const auto remoteThreadMainOffset = reinterpret_cast<BYTE*>(hookFunction) - reinterpret_cast<BYTE*>(pinky::dllModule);
        const void* remoteThreadMain = reinterpret_cast<void*>(reinterpret_cast<BYTE*>(remotePinky) + remoteThreadMainOffset);

        // Copy thread parameter to other process if given
        LPVOID threadParameter = nullptr;
        if (param != nullptr && paramSize != 0) {
            threadParameter = VirtualAllocEx(procInfo->pi.hProcess, NULL, paramSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (threadParameter == nullptr)
                pinky::utils::perrorWin32("VirtualAllocEx");

            if (WriteProcessMemory(procInfo->pi.hProcess, threadParameter, param, paramSize, NULL) == 0)
                pinky::utils::perrorWin32("WriteProcessMemory");
        }

        // Invoke the function ...
        const auto hRemotePinkyThread = CreateRemoteThread(procInfo->pi.hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remoteThreadMain, threadParameter, NULL, NULL);
        if (hRemotePinkyThread == INVALID_HANDLE_VALUE || hRemotePinkyThread == nullptr)
            pinky::utils::perrorWin32("Can initalize hook function");

        // prevent deadlock; for some uknown reason thread can
        // wait for main thread resume before exit ...
        DWORD threadState = 0;
        while ((threadState = WaitForSingleObject(hRemotePinkyThread, 100)) != WAIT_OBJECT_0) {
            ResumeThread(procInfo->pi.hThread);
            Sleep(1);
            SuspendThread(procInfo->pi.hThread);
        }

    }
  
    // Resume the reomte process
    ResumeThread(procInfo->pi.hThread);
    return EXIT_SUCCESS;
}

bool pinky::isReplacementFile(const char* filepath) {
    const auto len = std::strlen(filepath);

    int index = 0;
    for (const auto& hash : PINKY_FILES) {
        for (; index < FILE_LENGTH - 1; index++) {
            if (hash[FILE_LENGTH - index - 2] != filepath[len - index - 1])
                break;
        }

        if (index >= FILE_LENGTH - 1)
            return true;
    }

    return false;
}