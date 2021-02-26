
#include "IconsForkAwesome.h"
#include "imgui/imgui.h"
//#include "DropManager.h"
#include "Win32Window.h"

// Name of the window class for desktop windows created by this application.
#define WINDOW_CLASS_NAME "IMViewer WindowClass"
#if 0
void Win32CreateWindow(Win32Window* window, s32 default_width, s32 default_height, TCHAR* title);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

void Win32RegisterWindowClass()
{
	// Register the window class for our primary window, returning FALSE if unable.
	WNDCLASSEX window_class = {};
	window_class.cbSize = sizeof(window_class);
	window_class.style = CS_OWNDC;
	window_class.lpfnWndProc = WindowProc;
	window_class.hInstance = GetModuleHandle(0);
	//window_class.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
	window_class.hCursor = (HCURSOR)LoadImage(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	window_class.hIcon = (HICON)LoadImage(0, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	window_class.lpszClassName = WINDOW_CLASS_NAME;
	RegisterClassEx(&window_class);
}

void Win32CreateWindow(Win32Window* window, s32 default_width, s32 default_height, TCHAR* title)
{
	// Get the current screen size, so we can center our window.
	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);
	
	// Try to create a valid, centered window rect with the given size.
	RECT rect = { 0, 0, default_width, default_height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;
	LONG window_x = (screen_width - width) / 2;
	LONG window_y = (screen_height - height) / 2;
	if (window_x < 0) window_x = CW_USEDEFAULT;
	if (window_y < 0) window_y = CW_USEDEFAULT;
	if (width < 0) width = CW_USEDEFAULT;
	if (height < 0) height = CW_USEDEFAULT;
	
	window->cached_style = WS_OVERLAPPEDWINDOW;
	window->cached_ex_style = WS_EX_OVERLAPPEDWINDOW;
	// Create the window.
	window->handle = CreateWindowEx(
									window->cached_ex_style,
									WINDOW_CLASS_NAME,
									title,
									window->cached_style,
									window_x,
									window_y,
									width,
									height,
									0,
									0,
									GetModuleHandle(0),
									window);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	Win32Window* window = 0;
	if (message == WM_NCCREATE)
	{
		LPCREATESTRUCT create_struct = (LPCREATESTRUCT)lparam;
		window = (Win32Window*)create_struct->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
	}
	else
	{
		window = (Win32Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	}
	
	if (window)
	{
		if (Win32ImGuiWndProc(hwnd, message, wparam, lparam)) return true;
		
		switch (message)
		{
			case WM_DESTROY:
			{
				if (hwnd == primary_window.handle) PostQuitMessage(0);
				return 0;
			}
			case WM_PAINT:
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				ValidateRect(hwnd, &rect);
				return 0;
			}
			
			case WM_KEYDOWN:
			{
				bool repeat = ((lparam & (1 << 30)) != 0);
				if (!repeat && window->OnKeyDown) window->OnKeyDown((int)wparam);
				return 0;
			}
			case WM_KEYUP:
			{
				if (window->OnKeyUp) window->OnKeyUp((int)wparam);
				return 0;
			}
			
			case WM_SYSKEYDOWN:
			{
				// Handle ALT+ENTER:
				if ((wparam == VK_RETURN) && (lparam & (1 << 29)))
				{
					ToggleFullscreenWindow(window);
					return 0;
				}
				// Send all other WM_SYSKEYDOWN messages to the default WndProc.
				break;
			}
			case WM_SIZE:
			{
				RECT rect = {};
				GetClientRect(window->handle, &rect);
				window->width = rect.right - rect.left;
				window->height = rect.bottom - rect.top;
				window->is_minimized = (wparam == SIZE_MINIMIZED);
				if (window->OnResize) window->OnResize();
				return 0;
			}
			default: return DefWindowProc(hwnd, message, wparam, lparam);
		}
	}
	
	return DefWindowProc(hwnd, message, wparam, lparam);
}

void MyResize()
{
	if (!primary_window.is_minimized) OnSizeChanged(&primary_window);
}

void MyKeyDown(int key)
{
	switch(key)
	{
		case 0x41: is_a_pressed = true; break;
		case 0x44: is_d_pressed = true; break;
		case 0x45: is_e_pressed = true; break;
		case 0x51: is_q_pressed = true; break;
		case VK_SPACE: is_space_pressed = true; break;
		default: break;
	}
}

void MyKeyUp(int key)
{
	switch(key)
	{
		case 0x41: is_a_pressed = false; break;
		case 0x44: is_d_pressed = false; break;
		case 0x45: is_e_pressed = false; break;
		case 0x51: is_q_pressed = false; break;
		case VK_SPACE: is_space_pressed = false; break;
		default: break;
	}
}
#endif

void Platform::FatalError(const char* message, ...)
{
    const size_t output_buffer_size = 4096;
    char output_buffer[output_buffer_size];
    va_list args;
    va_start (args, message);
    vsnprintf(output_buffer, output_buffer_size, message, args);
    Platform::ShowErrorDialog(output_buffer);
    va_end(args);
    exit(-1);
}

bool Platform::ShowAssertDialog(const char* message, const char* file, u32 line)
{
	const size_t output_buffer_size = 4096;
    char buf[output_buffer_size];
	snprintf(buf, output_buffer_size,
			 "Assertion Failed!\n"
			 "    File: %s\n"
			 "    Line: %u\n"
			 "    Statement: ASSERT(%s)\n",
			 file, line, message);
	PrintError(buf);
    snprintf(buf, output_buffer_size,
			 "--File--\n"
			 "%s\n"
			 "\n"
			 "Line %u\n"
			 "\n"
			 "--Statement--\n"
			 "ASSERT(%s)\n"
			 "\n"
			 "Press Abort to stop execution, Retry to set a breakpoint (if debugging), or Ignore to continue execution.\n", file, line, message);
	
	int result = MessageBoxA(0, buf, "Assertion Failed!", MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
	if (result == IDABORT) exit(0);
	else if (result == IDRETRY) return true;
	else return false;
}

void Platform::ShowErrorDialog(const char* message)
{
    MessageBoxA(0, message, "Error!", MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
}