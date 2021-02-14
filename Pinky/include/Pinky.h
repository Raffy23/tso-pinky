#pragma once
#include <windows.h>

#ifdef PINKY_EXPORTS
#define PINKY_API __declspec(dllexport)
#else
#define PINKY_API __declspec(dllimport)
#endif

typedef struct PinkyConfig {
    const char* tso;
    const char* cwd;
    const char* uri;
};

extern "C" {

    PINKY_API DWORD pinky_hookUnpacker(PinkyConfig* config);

    PINKY_API void TEST();

}
