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