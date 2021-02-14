#pragma once

namespace pinky {
	namespace utils {

		extern bool shouldDisplayMessageBox;

		void perror(const char* str);

		void perrorWin32(const char* str);

		void perrorMsgBox(const char* str);

	}
}

