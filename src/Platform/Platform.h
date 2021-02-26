#ifndef _PLATFORM_H
#define _PLATFORM_H

struct AssetGUID;

namespace Platform
{
	enum class ConsoleColor : u8
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
	
	struct OpenFileResult
	{
		// NOTE(Matt): Both the array and each individual string it contains are heap allocated, so you
		// must free them all individually. I know, I'm not happy about it either.
		char** paths;
		int path_count;
	};
	bool ShowAssertDialog(const char* message, const char* file, u32 line);
	void ShowErrorDialog(const char* message);
    void FatalError(const char*  message, ...);
    void PrintMessage(const char* message, ConsoleColor text_color = ConsoleColor::Grey, ConsoleColor background_color = ConsoleColor::Black);
    void PrintError(const char* message, ConsoleColor text_color = ConsoleColor::Red, ConsoleColor background_color = ConsoleColor::Black);
    
    // TODO(Matt): Do a better string library.
	// NOTE(Matt): Because 0 is a valid file size, this function will return -1 if the file can't be opened
	// for whatever reason.
	s64 GetFileSize(const char* file_path); // TODO(Matt): I don't think this supports unicode paths.
    //void ReadFileToBuffer(const char* file_path, u8** buffer, u32* size);
	bool ReadFileToBuffer(const char* file_path, void* buffer, u32 buffer_size);
	bool WriteBufferToFile(u8* buffer, u64 size, const char* file_path, bool append);
    //Str GetFullExecutablePath();
    //Str NormalizePath(const char* path);
	
	char** ShowOpenFileDialog(int* count);
	//Str ShowBasicFileDialog(int type = 0, int resource_type = -1);
};
#endif //_PLATFORM_H
