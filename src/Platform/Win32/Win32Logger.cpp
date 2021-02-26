
#include "Platform/Platform.h"

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 512
#endif

struct Win32StandardStream {
	HANDLE handle;      // Stream handle (STD_OUTPUT_HANDLE or STD_ERROR_HANDLE).
	bool is_redirected; // True if redirected to file.
	bool is_wide;       // True if appending to a UTF-16 file.
	bool is_little_endian; // True if file is UTF-16 little endian.
};

// Sets up a standard stream (stdout or stderr).
static Win32StandardStream Win32GetStandardStream(u32 stream_type) {
	Win32StandardStream result = {};
	
	// If we don't have our own stream and can't find a parent console,
	// allocate a new console.
	result.handle = GetStdHandle(stream_type);
	if (!result.handle || result.handle == INVALID_HANDLE_VALUE) {
		if (!AttachConsole(ATTACH_PARENT_PROCESS))
			AllocConsole();
		result.handle = GetStdHandle(stream_type);
	}
	
	// Check if the stream is redirected to a file. If it is, check if
	// the file already exists. If so, parse the encoding.
	if (result.handle != INVALID_HANDLE_VALUE) {
		DWORD type = GetFileType(result.handle) & (~FILE_TYPE_REMOTE);
		DWORD dummy;
		result.is_redirected = (type == FILE_TYPE_CHAR)
			? !GetConsoleMode(result.handle, &dummy)
			: true;
		if (type == FILE_TYPE_DISK) {
			LARGE_INTEGER file_size;
			GetFileSizeEx(result.handle, &file_size);
			if (file_size.QuadPart > 1) {
				u16 bom = 0;
				SetFilePointerEx(result.handle, {}, 0, FILE_BEGIN);
				ReadFile(result.handle, &bom, 2, &dummy, 0);
				SetFilePointerEx(result.handle, {}, 0, FILE_END);
				result.is_wide = (bom == (u16)0xfeff || bom == (u16)0xfffe);
				result.is_little_endian = (bom == (u16)0xfffe);
			}
		}
	}
	return result;
}

// Translates foreground/background color into a WORD text attribute.
static WORD Win32TranslateConsoleColors(Platform::ConsoleColor text_color,
                                        Platform::ConsoleColor background_color) {
	WORD result = 0;
	switch (text_color) {
		case Platform::ConsoleColor::White:
		result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
			FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkGrey:
		result |= FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::Grey:
		result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		break;
		case Platform::ConsoleColor::DarkRed:
		result |= FOREGROUND_RED;
		break;
		case Platform::ConsoleColor::Red:
		result |= FOREGROUND_RED | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkGreen:
		result |= FOREGROUND_GREEN;
		break;
		case Platform::ConsoleColor::Green:
		result |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkBlue:
		result |= FOREGROUND_BLUE;
		break;
		case Platform::ConsoleColor::Blue:
		result |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkCyan:
		result |= FOREGROUND_GREEN | FOREGROUND_BLUE;
		break;
		case Platform::ConsoleColor::Cyan:
		result |= FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkPurple:
		result |= FOREGROUND_RED | FOREGROUND_BLUE;
		break;
		case Platform::ConsoleColor::Purple:
		result |= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkYellow:
		result |= FOREGROUND_RED | FOREGROUND_GREEN;
		break;
		case Platform::ConsoleColor::Yellow:
		result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		break;
		default:
		break;
	}
	
	switch (background_color) {
		case Platform::ConsoleColor::White:
		result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
			FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkGrey:
		result |= FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::Grey:
		result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		break;
		case Platform::ConsoleColor::DarkRed:
		result |= FOREGROUND_RED;
		break;
		case Platform::ConsoleColor::Red:
		result |= FOREGROUND_RED | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkGreen:
		result |= FOREGROUND_GREEN;
		break;
		case Platform::ConsoleColor::Green:
		result |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkBlue:
		result |= FOREGROUND_BLUE;
		break;
		case Platform::ConsoleColor::Blue:
		result |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkCyan:
		result |= FOREGROUND_GREEN | FOREGROUND_BLUE;
		break;
		case Platform::ConsoleColor::Cyan:
		result |= FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkPurple:
		result |= FOREGROUND_RED | FOREGROUND_BLUE;
		break;
		case Platform::ConsoleColor::Purple:
		result |= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
		case Platform::ConsoleColor::DarkYellow:
		result |= FOREGROUND_RED | FOREGROUND_GREEN;
		break;
		case Platform::ConsoleColor::Yellow:
		result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		break;
		default:
		break;
	}
	
	return result;
}

// Prints a message to a platform stream. If the stream is a console, uses
// supplied colors.
static void Win32PrintToStream(const char *message, Win32StandardStream stream,
                               Platform::ConsoleColor text_color,
                               Platform::ConsoleColor background_color) {
	
	// If redirected, write to a file instead of console.
	DWORD dummy;
	if (stream.is_redirected) {
		if (stream.is_wide) {
			static wchar_t buf[LOG_BUFFER_SIZE];
			s32 required_size =
				MultiByteToWideChar(CP_UTF8, 0, message, -1, 0, 0) - 1;
			s32 offset;
			for (offset = 0; offset + LOG_BUFFER_SIZE, required_size;
				 offset += LOG_BUFFER_SIZE) {
				// TODO(Matt): Little endian BOM.
				MultiByteToWideChar(CP_UTF8, 0, &message[offset], LOG_BUFFER_SIZE, buf,
									LOG_BUFFER_SIZE);
				WriteFile(stream.handle, buf, LOG_BUFFER_SIZE * 2, &dummy, 0);
			}
			s32 mod = required_size % LOG_BUFFER_SIZE;
			s32 size = MultiByteToWideChar(CP_UTF8, 0, &message[offset], mod, buf,
										   LOG_BUFFER_SIZE) *
				2;
			WriteFile(stream.handle, buf, size, &dummy, 0);
		} else {
			WriteFile(stream.handle, message, (DWORD)StrLen(message), &dummy, 0);
		}
	} else {
		WORD attribute = Win32TranslateConsoleColors(text_color, background_color);
		SetConsoleTextAttribute(stream.handle, attribute);
		WriteConsole(stream.handle, message, (DWORD)StrLen(message), &dummy, 0);
		attribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		SetConsoleTextAttribute(stream.handle, attribute);
	}
}

void Platform::PrintMessage(const char *message, Platform::ConsoleColor text_color,
                            Platform::ConsoleColor background_color) {
	// If we are in the debugger, output there.
	if (IsDebuggerPresent()) {
		OutputDebugStringA(message);
		return;
	}
	
	// Otherwise, output to stdout.
	static Win32StandardStream stream = Win32GetStandardStream(STD_OUTPUT_HANDLE);
	Win32PrintToStream(message, stream, text_color, background_color);
}

void Platform::PrintError(const char *message, Platform::ConsoleColor text_color,
                          Platform::ConsoleColor background_color) {
	// If we are in the debugger, output there.
	if (IsDebuggerPresent()) {
		OutputDebugStringA(message);
		return;
	}
	
	// Otherwise, output to stderr.
	static Win32StandardStream stream = Win32GetStandardStream(STD_ERROR_HANDLE);
	Win32PrintToStream(message, stream, text_color, background_color);
}