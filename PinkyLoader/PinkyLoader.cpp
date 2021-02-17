#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#include<psapi.h>

#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <array>
#include <vector>

#include <Pinky.h>
#include <nlohmann\json.hpp>

std::string BANNER = R"(
 _______     _              __                 
|_   __ \   (_)            [  |  _             
  | |__) |  __    _ .--.    | | / ]    _   __  
  |  ___/  [  |  [ `.-. |   | '' <    [ \ [  ] 
 _| |_      | |   | | | |   | |`\ \    \ '/ /  
|_____|    [___] [___||__] [__|  \_] [\_:  /   
                                      \__.'  
)";

LONG GetStringRegKey(HKEY hKey, const std::wstring& strValueName, std::wstring& strValue);
void error(const char* str);

int main(int argc, char **argv)
{
    std::cout << BANNER << std::endl;

    // Set working directory
    {
        HKEY hKey;
        LSTATUS s;

        if ((s=RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\tso\\DefaultIcon", 0, KEY_READ, &hKey)) != ERROR_SUCCESS) 
            error("Unable to read registry!");
       
        std::wstring cwd;
        GetStringRegKey(hKey, L"", cwd);

        std::wstring path = cwd.substr(0, cwd.find_last_of(L"\\"));
        SetCurrentDirectoryW(path.c_str());
    }
    
    std::ifstream configFile("Pinky.json");
    nlohmann::json jsonConfig;
    configFile >> jsonConfig;

    if (jsonConfig.is_discarded())
        error("Failed to parse Pinky.json");

    std::string binary = jsonConfig["binary"];

    char* CWD = new char[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, CWD);
    
    PinkyConfig config = {
        binary.c_str(),
        CWD,
        argv[1]
    };
    
    if (pinky_hookUnpacker(&config) != EXIT_SUCCESS)
        error("Unable to hook 'The Settlers Online.exe'");
    
    delete[] CWD;
    return EXIT_SUCCESS;
}

void error(const char* str) {

    std::cout << "Error: " << str << std::endl;
    std::cout << "Press any key to exit ..." << std::endl;
    std::cin.get();
    exit(EXIT_FAILURE);
}

LONG GetStringRegKey(HKEY hKey, const std::wstring& strValueName, std::wstring& strValue)
{
    WCHAR szBuffer[MAX_PATH];
    DWORD dwBufferSize = sizeof(szBuffer);
    
    ULONG nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (nError == ERROR_SUCCESS) {
        strValue = szBuffer;
    }

    return nError;
}