#ifndef _DROP_MANAGER_H
#define _DROP_MANAGER_H

#include <oleidl.h>
#include <shellapi.h>
#include <imgui/imgui.h>
#include "Win32Window.h"

bool is_dragging_files = false;
bool finish_drag_operation = false;
Str8 g_drag_result = {};

class DropManager : public IDropTarget
{
	public:
	
	ULONG AddRef()  { return 1; }
	ULONG Release() { return 0; }
	
	// we handle drop targets, let others know
	HRESULT QueryInterface(REFIID riid, void **ppvObject)
	{
		if (riid == IID_IDropTarget)
		{
			*ppvObject = this;	// or static_cast<IUnknown*> if preferred
			// AddRef() if doing things properly
			// but then you should probably handle IID_IUnknown as well;
			return S_OK;
		}
		
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
	
	HRESULT DragEnter(IDataObject* pDataObj, DWORD, POINTL, DWORD *pdwEffect)
	{
		// TODO: check whether we can handle this type of object at all and set *pdwEffect &= DROPEFFECT_NONE if not;
		
		// do something useful to flag to our application that files have been dragged from the OS into our application
		is_dragging_files = true;
		
		// trigger MouseDown for button 1 within ImGui
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[0] = true;
		
		*pdwEffect &= DROPEFFECT_COPY;
		
		FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM stgm;
		
		if (SUCCEEDED(pDataObj->GetData(&fmte, &stgm)))
		{
			HDROP hdrop = (HDROP)stgm.hGlobal; // or reinterpret_cast<HDROP> if preferred
			//UINT file_count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
			
			// we can drag more than one file at the same time, so we have to loop here
			TCHAR szFile[MAX_PATH];
			UINT cch = DragQueryFile(hdrop, 0, szFile, MAX_PATH);
			if (cch > 0 && cch < MAX_PATH)
			{
				g_drag_result= szFile;
			}
			ReleaseStgMedium(&stgm);
		}
		return S_OK;
	}
	
	// occurs when we drag files out from our applications view
	HRESULT DragLeave()
	{
		ImGuiIO& io = ImGui::GetIO();
		is_dragging_files = false;
		io.MouseDown[0] = false;
		g_drag_result = 0;
		return S_OK;
	}
	
	// occurs when we drag the mouse over our applications view whilst carrying files (post Enter, pre Leave)
	HRESULT DragOver(DWORD, POINTL pt, DWORD *pdwEffect)
	{
		POINT pos = {pt.x, pt.y};
		ScreenToClient(primary_window.handle, &pos);
		// trigger MouseMove within ImGui, position is within pt.x and pt.y
		// grfKeyState contains flags for control, alt, shift etc
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2((float)pos.x, (float)pos.y);
		
		*pdwEffect &= DROPEFFECT_COPY;
		return S_OK;
	}
	
	// occurs when we release the mouse button to finish the drag-drop operation
	HRESULT Drop(IDataObject*pDataObj, DWORD, POINTL, DWORD *pdwEffect)
	{
		finish_drag_operation = true;
		// grfKeyState contains flags for control, alt, shift etc
		
		// render the data into stgm using the data description in fmte
		FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM stgm;
		
		if (SUCCEEDED(pDataObj->GetData(&fmte, &stgm)))
		{
			HDROP hdrop = (HDROP)stgm.hGlobal; // or reinterpret_cast<HDROP> if preferred
			UINT file_count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
			
			// we can drag more than one file at the same time, so we have to loop here
			TCHAR szFile[MAX_PATH];
			UINT cch = DragQueryFile(hdrop, 0, szFile, MAX_PATH);
			if (cch > 0 && cch < MAX_PATH)
			{
				g_drag_result = szFile;
			}
			for (UINT i = 0; i < file_count; i++)
			{
				cch = DragQueryFile(hdrop, i, szFile, MAX_PATH);
				if (cch > 0 && cch < MAX_PATH)
				{
					// szFile contains the full path to the file, do something useful with it
					// i.e. add it to a vector or something
				}
			}
			
			// we have to release the data when we're done with it
			ReleaseStgMedium(&stgm);
			
			// notify our application somehow that we've finished dragging the files (provide the data somehow)
			// TODO(Matt): ...
		}
		
		// trigger MouseUp for button 1 within ImGui
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[0] = false;
		
		*pdwEffect &= DROPEFFECT_COPY;
		return S_OK;
	}
};
#endif //_DROP_MANAGER_H
