#include "pch.h"
#include "pinky.h"
#include "HookUtils.h"

int pinky::hook::suspendThreadsAndExecute(std::function<bool(void)> f)
{
    HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
    DWORD dwOwnerPID = GetCurrentProcessId();

    // Take a snapshot of all running threads  
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return EXIT_FAILURE;

    THREADENTRY32 te32 = { 0 };
    te32.dwSize = sizeof(THREADENTRY32);
    if (!Thread32First(hThreadSnap, &te32))
    {
        CloseHandle(hThreadSnap);
        return EXIT_FAILURE;
    }

    std::vector<HANDLE> threads;
    do
    {
        if (te32.th32OwnerProcessID == dwOwnerPID)
        {
            HANDLE threadHandle = INVALID_HANDLE_VALUE;
            threadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
        }
    } while (Thread32Next(hThreadSnap, &te32));
    CloseHandle(hThreadSnap);

    // Pause all the threads ...
    for (const auto& thread : threads) {
        SuspendThread(thread);
    }

    pinky::initalize();
    if (!f()) {
        return -255;
    }

    // Resume threads
    for (const auto& thread : threads) {
        ResumeThread(thread);
        CloseHandle(thread);
    }

    return EXIT_SUCCESS;
}

// https://stackoverflow.com/questions/1591342/c-how-to-determine-if-a-windows-process-is-running/5303889
bool pinky::hook::isProcessRunning(const wchar_t* processName) {
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry))
        while (Process32Next(snapshot, &entry))
            if (_wcsicmp(entry.szExeFile, processName) == 0 && entry.th32ProcessID != GetCurrentProcessId()) {
                exists = true;
                break;
            }
                
    CloseHandle(snapshot);
    return exists;
}