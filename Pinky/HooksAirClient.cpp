#include "pch.h"
#include <map>
#include <mutex>

#include "HooksAirClient.h"
#include "pinky.h"
#include "Utils.h"

constexpr size_t MAX_PATH_LENGTH = 256;

pinky::hook::WinInet_t pinky::hook::winInetFunc;


static std::map<HINTERNET, size_t> requestHandles;
static std::mutex mutexRequestHandles;


BOOLEAN pinky::hook::DetourInternetReadFile(
    HINTERNET hFile,
    LPVOID    lpBuffer,
    DWORD     dwNumberOfBytesToRead,
    LPDWORD   lpdwNumberOfBytesRead) {

    DWORD bufferSize = MAX_PATH_LENGTH;
    char buffer[MAX_PATH_LENGTH];

    // Query name of the request
    InternetQueryOptionA(
        hFile,
        INTERNET_OPTION_URL,
        buffer,
        &bufferSize
    );

    // Check if we should handle the file
    if (pinky::isReplacementFile(buffer)) {
        
        *lpdwNumberOfBytesRead = 0;
        if (dwNumberOfBytesToRead == 0)
            return TRUE;

        std::lock_guard<std::mutex> guard(mutexRequestHandles);

        size_t transferedBytes = 0;
        {
            auto request = requestHandles.find(hFile);
            if (request != requestHandles.end()) {
                transferedBytes = request->second;
                //MessageBoxA(nullptr, std::to_string(transferedBytes).c_str(), "Pinky", MB_OK | MB_ICONEXCLAMATION);
            }
                
        }

        if (transferedBytes >= replacementImageSize)
            return TRUE;
        
        const DWORD bytesToCopy = min(replacementImageSize - transferedBytes, dwNumberOfBytesToRead);
        /*
        const auto str =
            std::string("hFile: ") + std::to_string(reinterpret_cast<uint64_t>(hFile)) +
            std::string("\ntransferedBytes: ") + std::to_string(transferedBytes) +
            std::string("\nbytesToCopy: ") + std::to_string(bytesToCopy) +
            std::string("\ndwNumberOfBytesToRead") + std::to_string(dwNumberOfBytesToRead);
        MessageBoxA(nullptr, str.c_str(), "Pinky", MB_OK | MB_ICONEXCLAMATION);
        */
        requestHandles[hFile] = (transferedBytes + bytesToCopy);

        if (bytesToCopy > 0)
            memcpy(lpBuffer, replacementImage + transferedBytes, bytesToCopy);

        *lpdwNumberOfBytesRead = bytesToCopy;
        return TRUE;
    }

    // Otherwise invoke the original function
    return pinky::hook::winInetFunc.InternetReadFile(
        hFile,
        lpBuffer,
        dwNumberOfBytesToRead,
        lpdwNumberOfBytesRead
    );
}

BOOLEAN pinky::hook::DetourInternetQueryDataAvailable(HINTERNET hFile, LPDWORD lpdwNumberOfBytesAvailable, DWORD dwFlags, DWORD_PTR dwContext)
{
    DWORD bufferSize = MAX_PATH_LENGTH;
    char buffer[MAX_PATH_LENGTH];
    
    InternetQueryOptionA(
        hFile,
        INTERNET_OPTION_URL,
        buffer,
        &bufferSize
    );

    if (pinky::isReplacementFile(buffer)) {
        *lpdwNumberOfBytesAvailable = replacementImageSize;
        return TRUE;
    }

    return winInetFunc.InternetQueryDataAvailable(hFile, lpdwNumberOfBytesAvailable, dwFlags, dwContext);
}

bool pinky::hook::hookAirClient() {
    bool res1 = hookApi("wininet.dll", "InternetReadFile", DetourInternetReadFile, &winInetFunc.InternetReadFile);
    bool res2 = hookApi("wininet.dll", "InternetQueryDataAvailable", DetourInternetQueryDataAvailable, &winInetFunc.InternetQueryDataAvailable);

    return res1 & res2;
}

DWORD __stdcall pinky::hook::AdobeAirHookEntryPoint(LPVOID lpThreadParameter) {
    /*
    const auto result = pinky::hook::suspendThreadsAndExecute(pinky::hook::hookAirClient);
    if (result != EXIT_SUCCESS) {
        MessageBox(nullptr, L"Something went wrong, can not hook client.exe!", L"Pinky", MB_OK | MB_ICONERROR);
    }
    */

    pinky::initalize(true);
    pinky::utils::shouldDisplayMessageBox = true;

    if (!pinky::hook::hookAirClient()) {
        MessageBox(nullptr, L"Something went wrong, can not hook client.exe!", L"Pinky", MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}