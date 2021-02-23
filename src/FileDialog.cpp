#pragma comment(lib, "Ole32.lib")
#include <shobjidl.h>

char* Win32ShowOpenFileDialog()
{
	char* result = 0;
	//Str result = {};
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | 
								COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog *pFileOpen;
		
		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
							  IID_IFileOpenDialog, (void**)&pFileOpen);
		
		if (SUCCEEDED(hr))
		{
			COMDLG_FILTERSPEC rgSpec[] =
			{
				{ L"any image", L"*.png;*.bmp;*.jpg;*.jpeg;*.tga;*.psd;*.gif" },
				{ L"png image", L"*.png" },
				{ L"bmp image", L"*.bmp" },
				{ L"jpeg image", L"*.jpg;*.jpeg" },
				{ L"tga image", L"*.tga" },
				{ L"psd image", L"*.psd" },
				{ L"gif image", L"*.gif" }
			};
			pFileOpen->SetFileTypes(sizeof(rgSpec) / sizeof(rgSpec[0]), rgSpec);
			pFileOpen->SetDefaultExtension(L"png");
			
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
						result = (char*)malloc(size + 1);
						WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, result, size, 0, 0);
						result[size] = 0;
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		
		CoUninitialize();
	}
	return result;
}
#if 0
char* Win32ShowBasicFileDialog(int dialog_type)
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
#endif