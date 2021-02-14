#pragma once
#include <Windows.h>
#include <functional>
#include <string>

#include <MinHook.h>

namespace pinky {

	namespace hook {

        template<typename R, typename ...ARGS> using function = R(*)(ARGS...);

        template<typename T>
        T loadDllFunc(const std::string& dllName, const std::string& funcName) {

            const HINSTANCE library = LoadLibraryA(dllName.c_str());
            if (library == nullptr)
                throw "Can not load library!";

            const FARPROC fPointer = GetProcAddress(library, funcName.c_str());
            if (fPointer == nullptr)
                throw "Can not load function!";

            return reinterpret_cast<T>(fPointer);
        }

        template<typename T>
        bool hookApi(const std::string& dllName, const std::string& funcName, T detour, T* backup) {

            const HMODULE hModule = GetModuleHandleA(dllName.c_str());
            if (hModule == nullptr)
                throw "Can not load library!";

            const LPVOID target = reinterpret_cast<LPVOID>(GetProcAddress(hModule, funcName.c_str()));
            if (target == nullptr)
                throw "Can not load function!";

            *backup = reinterpret_cast<T>(target);
            if (MH_CreateHook(target, detour, reinterpret_cast<LPVOID*>(backup)) != MH_OK) {
                return false;
            }
                
            return MH_EnableHook(target) == MH_OK;
        }

        /**
         * Pauses all threads in the process and executes function f, after that
         * all threads are resumed again.
         */
        extern int suspendThreadsAndExecute(std::function<bool(void)> f);

	}

}
