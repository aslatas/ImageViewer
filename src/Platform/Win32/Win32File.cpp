
bool Platform::ReadFileToBuffer(const char* file_path, void* buffer, u32 buffer_size)
{
	Assert(buffer && buffer_size && file_path);
	
	bool result = false;
	
    HANDLE handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
	
    if (handle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size;
		if (GetFileSizeEx(handle, &file_size))
		{
			Assert(buffer_size >= file_size.QuadPart);
			DWORD dummy;
			result = ReadFile(handle, buffer, buffer_size, &dummy, 0);
			CloseHandle(handle);
		}
	}
	return result;
}

s64 Platform::GetFileSize(const char* file_path)
{
	s64 result = -1;
	u32 open_mode = GENERIC_READ;
	u32 share_mode = 0;
	u32 create_mode = OPEN_EXISTING;
	u32 flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	void* handle = CreateFileA(file_path, open_mode, share_mode, 0, create_mode, flags, 0);
	if (handle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size;
		if (GetFileSizeEx(handle, &file_size)) result = file_size.QuadPart;
		CloseHandle(handle);
	}
	return result;
}

bool Platform::WriteBufferToFile(u8* buffer, u64 size, const char* file_path, bool append)
{
	u32 open_mode = GENERIC_WRITE;
	u32 share_mode = 0;
	u32 create_mode = (append) ? CREATE_ALWAYS : OPEN_ALWAYS;
	u32 flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	void* handle = CreateFileA(file_path, open_mode, 0, 0, create_mode, flags, 0);
	
	bool success = (handle != INVALID_HANDLE_VALUE);
    
	if (success)
	{
		DWORD dummy; // Unused, but Win32 says we have to pass this to WriteFile.
		
		u32 count = (u32)(size / U32_MAX);
		u32 mod = size % U32_MAX;
		
		for (u32 i = 0; i < count; ++i)
		{
			if (!WriteFile(handle, buffer + (u64)(i * U32_MAX), U32_MAX, &dummy, 0))
			{
				success = false;
				break;
			}
		}
		
		if (success && !WriteFile(handle, buffer + (u64)(U32_MAX * count), mod, &dummy, 0)) success = false;
		CloseHandle(handle);
	}
	
	return success;
}

#if 0
Str Platform::GetFullExecutablePath()
{
    char buf[MAX_PATH];
    if (GetCurrentDirectory(MAX_PATH, buf) > MAX_PATH) return {};
    for (u32 i = 0; buf[i]; ++i) if (buf[i] == '\\') buf[i] = '/';
    return Str(buf);
}

Str Platform::NormalizePath(const char* path)
{
    Str result = {};
    
    // If the string is null or has length < 2, just return an empty one.
    if (!path || !path[0] || !path[1]) return result;
    
    // If a relative path, append it to the full executable path.
    if (path[0] == '/' || path[0] == '\\' || path[0] == '.') result = Platform::GetFullExecutablePath();
    if (path[0] == '.') result += '/';
    result += path;
    
    // Swap any back slashes for forward slashes.
    for (u32 i = 0; i < (u32)result.size; ++i) if (result[i] == '\\') result[i] = '/';
    
    // Strip double separators.
    for (u32 i = 0; i < (u32)result.size - 1; ++i)
    {
        if (result[i] == '/' && result[i + 1] == '/')
        {
            for (u32 j = i; j < (u32)result.size; ++j) result[j] = result[j + 1];
            --result.size;
            --i;
        }
    }
    
    // Evaluate any relative specifiers (./).
    if (result[0] == '.' && result[1] == '/')
    {
        for (u32 i = 0; i < (u32)result.size - 1; ++i) result[i] = result[i + 2];
        result.size -= 2;
    }
    for (u32 i = 0; i < (u32)result.size - 1; ++i)
    {
        if (result[i] != '.' && result[i + 1] == '.' && result[i + 2] == '/')
        {
            for (u32 j = i + 1; result[j + 1]; ++j)
            {
                result[j] = result[j + 2];
            }
            result.size -= 2;
        }
    }
    
    // Evaluate any parent specifiers (../).
    u32 last_separator = 0;
    for (u32 i = 0; (i < (u32)result.size - 1); ++i)
    {
        if (result[i] == '.' && result[i + 1] == '.' && result[i + 2] == '/')
        {
            u32 base = i + 2;
            u32 count = result.size - base;
            
            for (u32 j = 0; j <= count; ++j)
            {
                result[last_separator + j] = result[base + j];
            }
            
            result.size -= base - last_separator;
            i = last_separator;
            
            if (i > 0)
            {
                bool has_separator = false;
                for (s32 j = last_separator - 1; j >= 0; --j)
                {
                    if (result[j] == '/')
                    {
                        last_separator = j;
                        has_separator = true;
                        break;
                    }
                }
                if (!has_separator) return {};
            }
        }
        if (i > 0 && result[i - 1] == '/') last_separator = i - 1;
    }
    
    // Strip any leading or trailing separators.
    if (result[0] == '/')
    {
        for (u32 i = 0; i < (u32)result.size; ++i) result[i] = result[i + 1];
        --result.size;
    }
    
    if (result[result.size - 1] == '/')
    {
        result[result.size - 1] = '\0';
        --result.size;
    }
    return result;
}
#endif

char** Platform::ShowOpenFileDialog(int* count)
{
	char** result = 0;
	*count = 0;
	
	int success = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (success >= 0)
	{
		IFileOpenDialog *open_dialog;
		
		// Create the FileOpenDialog object.
		success = CoCreateInstance(CLSID_FileOpenDialog, 0, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&open_dialog);
		
		if (success >= 0)
		{
			FILEOPENDIALOGOPTIONS flags = 0;
			success = open_dialog->GetOptions(&flags);
			
			if (success >= 0)
			{
				success = open_dialog->SetOptions(flags | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT);
			}
			COMDLG_FILTERSPEC filters[] =
			{
				{ L"any image", L"*.png;*.bmp;*.jpg;*.jpeg;*.tga;*.psd;*.gif" },
				{ L"png image", L"*.png" },
				{ L"bmp image", L"*.bmp" },
				{ L"jpeg image", L"*.jpg;*.jpeg" },
				{ L"tga image", L"*.tga" },
				{ L"psd image", L"*.psd" },
				{ L"gif image", L"*.gif" }
			};
			open_dialog->SetFileTypes(sizeof(filters) / sizeof(filters[0]), filters);
			open_dialog->SetDefaultExtension(L"png");
			
			success = open_dialog->Show(NULL);
			
			if (success >= 0)
			{
				IShellItemArray* result_array;
				
				success = open_dialog->GetResults(&result_array);
				
				if (success >= 0)
				{
					DWORD item_count = 0;
					success = result_array->GetCount(&item_count);
					if (success >= 0)
					{
						*count = (int)item_count;
						result = (char**)malloc(sizeof(char*) * item_count); // @malloc
						
						for (u32 i = 0; i < item_count; ++i)
						{
							IShellItem* item = 0;
							success = result_array->GetItemAt(i, &item);
							
							if (success >= 0)
							{
								wchar_t* wide_path;
								success = item->GetDisplayName(SIGDN_FILESYSPATH, &wide_path);
								
								if (success >= 0)
								{
									int size = WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, 0, 0, 0, 0);
									result[i] = (char*)malloc(size + 1);
									WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, result[i], size, 0, 0);
									result[i][size] = 0;
									CoTaskMemFree(wide_path);
								}
								item->Release();
							}
							else
							{
								result[i] = 0;
							}
						}
					}
				}
			}
			open_dialog->Release();
		}
		
		CoUninitialize();
	}
	return result;
}
/*
bool Win32ShowBasicFileDialog()
{
    // CoCreate the File Open Dialog object.
    IFileDialog *pfd = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, 
								  NULL, 
								  CLSCTX_INPROC_SERVER, 
								  _uuidof(IFileDialog), (void**)&pfd);
    if (SUCCEEDED(hr))
    {
        // Create an event handling object, and hook it up to the dialog.
        IFileDialogEvents *pfde = NULL;
        hr = CDialogEventHandler_CreateInstance(_uuidof(IFileDialogEvents), (void**)&pfde);
        if (SUCCEEDED(hr))
        {
            // Hook up the event handler.
            DWORD dwCookie;
            hr = pfd->Advise(pfde, &dwCookie);
            if (SUCCEEDED(hr))
            {
                // Set the options on the dialog.
                DWORD dwFlags;
				
                // Before setting, always get the options first in order 
                // not to override existing options.
                hr = pfd->GetOptions(&dwFlags);
                if (SUCCEEDED(hr))
                {
                    // In this case, get shell items only for file system items.
                    hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
                    if (SUCCEEDED(hr))
                    {
                        // Set the file types to display only. 
                        // Notice that this is a 1-based array.
                        hr = pfd->SetFileTypes(ARRAYCOUNT(c_rgSaveTypes), c_rgSaveTypes);
                        if (SUCCEEDED(hr))
                        {
                            // Set the selected file type index to Word Docs for this example.
                            hr = pfd->SetFileTypeIndex(INDEX_WORDDOC);
                            if (SUCCEEDED(hr))
                            {
                                // Set the default extension to be ".doc" file.
                                hr = pfd->SetDefaultExtension(L"doc;docx");
                                if (SUCCEEDED(hr))
                                {
                                    // Show the dialog
                                    hr = pfd->Show(NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        // Obtain the result once the user clicks 
                                        // the 'Open' button.
                                        // The result is an IShellItem object.
                                        IShellItem *psiResult;
                                        hr = pfd->GetResult(&psiResult);
                                        if (SUCCEEDED(hr))
                                        {
                                            // We are just going to print out the 
                                            // name of the file for sample sake.
                                            PWSTR pszFilePath = NULL;
                                            hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, 
																		   &pszFilePath);
                                            if (SUCCEEDED(hr))
                                            {
                                                TaskDialog(NULL,
                                                           NULL,
                                                           L"CommonFileDialogApp",
                                                           pszFilePath,
                                                           NULL,
                                                           TDCBF_OK_BUTTON,
                                                           TD_INFORMATION_ICON,
                                                           NULL);
                                                CoTaskMemFree(pszFilePath);
                                            }
                                            psiResult->Release();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                // Unhook the event handler.
                pfd->Unadvise(dwCookie);
            }
            pfde->Release();
        }
        pfd->Release();
    }
    return true;
}
*/

#if 0
static Str Win32ShowBasicFileDialog(s32 dialog_type, s32 resource_type)
{
	Str result = {};
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | 
								COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		switch (dialog_type)
		{
			case 0: // Open dialog
			{
				IFileOpenDialog *pFileOpen;
				
				// Create the FileOpenDialog object.
				hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
									  IID_IFileOpenDialog, (void**)&pFileOpen);
				
				if (SUCCEEDED(hr))
				{
					if (resource_type == 0)
					{
						COMDLG_FILTERSPEC rgSpec[] =
						{ 
							{ L"region file", L"*.region" }
						};
						pFileOpen->SetFileTypes(ARRAYCOUNT(rgSpec), rgSpec);
						pFileOpen->SetDefaultExtension(L"region");
					}
					else if (resource_type == 1)
					{
						COMDLG_FILTERSPEC rgSpec[] =
						{
							{ L"any image", L"*.png;*.bmp;*.jpg;*.jpeg" },
							{ L"png image", L"*.png" },
							{ L"bmp image", L"*.bmp" },
							{ L"jpeg image", L"*.jpg;*.jpeg" },
						};
						pFileOpen->SetFileTypes(ARRAYCOUNT(rgSpec), rgSpec);
						pFileOpen->SetDefaultExtension(L"png");
					}
					
					// Show the Open dialog box.
					hr = pFileOpen->Show(NULL);
					
					// Get the file name from the dialog box.
					if (SUCCEEDED(hr))
					{
						IShellItem *pItem;
						hr = pFileOpen->GetResult(&pItem);
						if (SUCCEEDED(hr))
						{
							PWSTR pszFilePath;
							hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
							
							// Display the file name to the user.
							if (SUCCEEDED(hr))
							{
								int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, 0, 0, 0, 0);
								result = Str8(size);
								WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, result, size, 0, 0);
								result[size] = 0;
								result.size = size;
								CoTaskMemFree(pszFilePath);
							}
							pItem->Release();
						}
					}
					pFileOpen->Release();
				}
			}
			break;
			case 1: // Save dialog
			{
				IFileSaveDialog *pFileSave;
				
				// Create the FileOpenDialog object.
				hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, 
									  IID_IFileSaveDialog, (void**)&pFileSave);
				
				if (SUCCEEDED(hr))
				{
					if (resource_type == 0)
					{
						COMDLG_FILTERSPEC rgSpec[] =
						{ 
							{ L"region file", L"*.region" }
						};
						pFileSave->SetFileTypes(ARRAYCOUNT(rgSpec), rgSpec);
						pFileSave->SetDefaultExtension(L"region");
					}
					else if (resource_type == 1)
					{
						COMDLG_FILTERSPEC rgSpec[] =
						{
							{ L"any image", L"*.png;*.bmp;*.jpg;*.jpeg" },
							{ L"png image", L"*.png" },
							{ L"bmp image", L"*.bmp" },
							{ L"jpeg image", L"*.jpg;*.jpeg" },
						};
						pFileSave->SetFileTypes(ARRAYCOUNT(rgSpec), rgSpec);
						pFileSave->SetDefaultExtension(L"png");
					}
					
					// Show the Open dialog box.
					hr = pFileSave->Show(NULL);
					
					// Get the file name from the dialog box.
					if (SUCCEEDED(hr))
					{
						IShellItem *pItem;
						hr = pFileSave->GetResult(&pItem);
						if (SUCCEEDED(hr))
						{
							PWSTR pszFilePath;
							hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
							
							// Display the file name to the user.
							if (SUCCEEDED(hr))
							{
								int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, 0, 0, 0, 0);
								result = Str8(size);
								WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, result, size, 0, 0);
								result[size] = 0;
								result.size = size;
								CoTaskMemFree(pszFilePath);
							}
							pItem->Release();
						}
					}
					pFileSave->Release();
				}
			}
			break;
			default: Assert(false);
		}
		
		CoUninitialize();
	}
	return result;
}

Str Platform::ShowBasicFileDialog(int type, int resource_type)
{
	return Win32ShowBasicFileDialog(type, resource_type);
}
void Platform::GenerateGUID(uint64_t* lower, uint64_t* upper)
{
	Assert(upper && lower);
	GUID id;
	HRESULT status = CoCreateGuid(&id);
	Assert(status == S_OK);
	
	void* in_ptr = &id;
	uint64_t* uint_ptr = (uint64_t*)in_ptr;
	*lower = *uint_ptr;
	*upper = *(uint_ptr + 1);
}
#endif