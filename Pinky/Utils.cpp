#include "pch.h"
#include <iostream>
#include "Utils.h"

bool pinky::utils::shouldDisplayMessageBox = true;

void pinky::utils::perror(const char* str) {
	std::cerr << "Error: " << str << std::endl;
	exit(EXIT_FAILURE);
}

void pinky::utils::perrorWin32(const char* str) {
	if (shouldDisplayMessageBox)
		perrorMsgBox(str);

	std::cerr << "Error: " << str << " (" << GetLastError() << ")" << std::endl;
	exit(EXIT_FAILURE);
}

void pinky::utils::perrorMsgBox(const char* str) {
	const auto error = std::string("Error: ") + str + " (" + std::to_string(GetLastError()) + ")";

	MessageBoxA(nullptr, error.c_str(), "Pinky", MB_OK | MB_ICONERROR);
	exit(EXIT_FAILURE);
}
