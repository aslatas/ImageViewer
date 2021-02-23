#ifndef _PLATFORM_H
#define _PLATFORM_H

struct AssetGUID;

namespace Platform
{
	enum class EConsoleColor : u8
	{
		Black = 0, White,
		DarkGrey, Grey,
		DarkRed, Red,
		DarkGreen, Green,
		DarkBlue, Blue,
		DarkCyan, Cyan,
		DarkPurple, Purple,
		DarkYellow, Yellow,
	};
	
	bool ShowAssertDialog(const char* message, const char* file, u32 line);
	void ShowErrorDialog(const char* message);
    void FatalError(const char*  message, ...);
    void PrintMessage(const char* message, EConsoleColor text_color = EConsoleColor::Grey, EConsoleColor background_color = EConsoleColor::Black);
    void PrintError(const char* message, EConsoleColor text_color = EConsoleColor::Red, EConsoleColor background_color = EConsoleColor::Black);
    
    // TODO(Matt): Do a better string library.
	s64 GetFileSize(const char* file_path);
    void ReadFileToBuffer(const char* file_path, u8** buffer, u32* size);
	bool ReadFileToBuffer(const char* file_path, void* buffer, u32 buffer_size);
    void WriteBufferToFile(u8* buffer, u64 size, const char* file_path, bool append);
    Str GetFullExecutablePath();
    Str NormalizePath(const char* path);
	// TODO(Matt): Replace these params with enums.
	Str ShowBasicFileDialog(int type = 0, int resource_type = -1);
};
#endif //_PLATFORM_H
