#ifndef _WIN32_WINDOW_H
#define _WIN32_WINDOW_H

struct Win32Window
{
	HWND handle;
	
	u32 width;
	u32 height;
	
	bool is_minimized;
	bool is_fullscreen;
	
	RECT cached_rect;
	DWORD cached_style;
	DWORD cached_ex_style;
	// Callbacks for window events.
	void (*OnKeyDown)(s32 key);
	void (*OnKeyUp)(s32 key);
	void (*OnResize)();
};

//Win32Window primary_window = {};
#endif //_WIN32_WINDOW_H
