// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#define GMATH_IMPLEMENTATION
#include "GMath.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "ImageLoader.h"
#include "FileDialog.h"
#include "imgui/imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_extensions.h"

#include "d3d_proto.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#pragma comment( lib, "dxguid.lib")

#define NAME_OBJECT(object) static const char object##_name[] = #object;\
object->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(object##_name) - 1, object##_name);

// Data
//static ID3D11Device*            g_pd3dDevice = NULL;
//static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;
static ID3D11Debug* g_pDebug = NULL;


ImagePanel* image_panels = 0;

static int next_panel_id = 1;
// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImageViewer Window"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
	
    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }
	
    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
	
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigViewportsNoDefaultParent = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.
	
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
	
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
	
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
	
    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
	
    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	
    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }
		
		// If any of the image panels should close, go ahead and destroy them.
		for (int i = (int)arrlen(image_panels) - 1; i >= 0; --i)
		{
			if (!image_panels[i].is_visible)
			{
				ReleaseImagePanel(image_panels[i]);
				arrdelswap(image_panels, i);
			}
		}
        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
		
		//ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_NoResize);
		ImGuiID dockspace_id = ImGui::SetUpInitialDockSpace("Controls", 0.25f);
		// Show our cool image window
		static float img_clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		
		{
#if 0
			ImGui::Begin("My Cool Image Window", 0);
			
			static Vec2 last_image_size = Vec2(0, 0);
			
			Vec2 current_image_size = ImGui::GetContentRegionAvail();
			if (current_image_size.x < 1.0f) current_image_size.x = 1.0f;
			if (current_image_size.y < 1.0f) current_image_size.y = 1.0f;
			
			if (last_image_size != current_image_size)
			{
				last_image_size = current_image_size;
				ResizeImagePanelCanvas(g_pd3dDevice, &img, (int)current_image_size.x, (int)current_image_size.y);
			}
			Vec2 cursor_pos = ImGui::GetCursorScreenPos();
			Vec2 mouse_pos = ImGui::GetMousePos();
			ImTextureID tex_id = (ImTextureID)img.dst_srv;
			ImGui::Image(tex_id, current_image_size, Vec2(0, 0), Vec2(1, 1));
			
			static bool is_down_inside = false;
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsItemHovered()) is_down_inside = true;
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) is_down_inside = false;
			if (is_down_inside && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
			{
				Vec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);
				Vec2 delta = {drag_delta.x, drag_delta.y};
				ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
				img.image_offset += delta;
			}
			
			static float zoom_sens = 1.1f;
			Vec2 relative_mouse_pos = mouse_pos - cursor_pos;
			
			Vec2 src_half_size = img.image_size / 2.0f;
			Vec2 dst_half_size = current_image_size / 2.0f;
			Vec2 src_tl = dst_half_size - src_half_size + img.image_offset;
			Vec2 src_br = src_tl + img.image_size;
			
			Vec2 src_relative_pos = (relative_mouse_pos - src_tl) / (src_br - src_tl);
			if (io.MouseWheel && ImGui::IsWindowHovered())
			{
				float amt = Pow(zoom_sens, io.MouseWheel);
				img.image_size *= amt;
				Vec2 dst_tl = relative_mouse_pos - (src_relative_pos * img.image_size);
				Vec2 new_half_size = img.image_size / 2.0f;
				Vec2 desired_tl = relative_mouse_pos - (src_relative_pos * img.image_size);
				img.image_offset = desired_tl + new_half_size - dst_half_size;
			}
			Vec2 dst_tl = relative_mouse_pos - (src_relative_pos * img.image_size);
			
			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->AddRect(cursor_pos + dst_tl, cursor_pos + dst_tl + img.image_size, ImGui::GetColorU32(ImGuiCol_Border));
			
			ImGui::End();
#endif
			for (int i = 0; i < arrlen(image_panels); ++i)
			{
				DrawImagePanel(&image_panels[i], dockspace_id);
			}
			//DrawImagePanel(&img);
			ImGui::Begin("Controls", 0, ImGuiWindowFlags_None);
			//ImGui::DragFloat2("Offset", img.image_offset.data, 1.0f);
			//ImGui::DragFloat2("Size", img.image_size.data, 1.0f);
			//ImGui::ColorEdit4("Background", img_clear_color);
			ImGui::End();
			
		}
		
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
		
		//~ Show main menu bar.
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					
				}
				if (ImGui::MenuItem("Open", "Ctrl+O"))
				{
					char* file_name = Win32ShowOpenFileDialog();
					
					if (file_name)
					{
						ImagePanel new_panel = LoadImageFromFile(g_pd3dDevice, g_pd3dDeviceContext, file_name, next_panel_id, 512, 512);
						++next_panel_id;
						arrput(image_panels, new_panel);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View"))
			{
				ImGui::EndMenu();
			}
			
			ImGui::EndMainMenuBar();
		}
		
		
        // Rendering
        ImGui::Render();
		
		for (int i = 0; i < arrlen(image_panels); ++i)
		{
			ImagePanel* panel = &image_panels[i];
			// Re-upload the constant buffer for our image.
			{
				D3D11_MAPPED_SUBRESOURCE mapped_resource;
				if (g_pd3dDeviceContext->Map(panel->constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK) assert(false);
				VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource.pData;
				
				Vec2 ortho_size = {2, 2};
				Mat4 ortho = CreateOrthoMatrix(ortho_size.x, ortho_size.y, 1, 0.0f);
				ortho[1][1] *= -1;
				
				D3D11_TEXTURE2D_DESC src_desc = {};
				D3D11_TEXTURE2D_DESC dst_desc = {};
				panel->texture->GetDesc(&src_desc);
				panel->render_target->GetDesc(&dst_desc);
				
				IVec2 src_size = {(int)src_desc.Width, (int)src_desc.Height};
				IVec2 dst_size = {(int)dst_desc.Width, (int)dst_desc.Height};
				
				Vec2 offset = ortho_size * panel->image_offset / (Vec2)dst_size;
				
				Vec2 scale = panel->image_size / (Vec2)dst_size;
				Mat4 mvp = ortho * CreateTranslationMatrix(Vec3(offset, 0)) * CreateScalingMatrix(Vec3(scale, 1));
				
				CopyMemory(&constant_buffer->mvp, &mvp, sizeof(mvp));
				g_pd3dDeviceContext->Unmap(panel->constant_buffer, 0);
			}
			
			
			g_pd3dDeviceContext->OMSetRenderTargets(1, &panel->rtv, 0);
			g_pd3dDeviceContext->ClearRenderTargetView(panel->rtv, img_clear_color);
			
			// Setup viewport
			{
				D3D11_VIEWPORT vp = {};
				D3D11_TEXTURE2D_DESC desc = {};
				panel->render_target->GetDesc(&desc);
				vp.Width = (float)desc.Width;
				vp.Height = (float)desc.Height;
				vp.MinDepth = 0.0f;
				vp.MaxDepth = 1.0f;
				vp.TopLeftX = vp.TopLeftY = 0;
				g_pd3dDeviceContext->RSSetViewports(1, &vp);
				// Setup shader and vertex buffers
				g_pd3dDeviceContext->IASetInputLayout(g_pInputLayout);
				UINT stride = (UINT)sizeof(CoolVertex);
				UINT offset = 0;
				g_pd3dDeviceContext->IASetVertexBuffers(0, 1, &panel->vertex_buffer, &stride, &offset);
				g_pd3dDeviceContext->IASetIndexBuffer(panel->index_buffer, DXGI_FORMAT_R32_UINT, 0);
				g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				g_pd3dDeviceContext->VSSetShader(g_pVertexShader, NULL, 0);
				g_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &panel->constant_buffer);
				g_pd3dDeviceContext->PSSetShader(g_pPixelShader, NULL, 0);
				g_pd3dDeviceContext->PSSetSamplers(0, 1, &g_pFontSampler);
				g_pd3dDeviceContext->GSSetShader(NULL, NULL, 0);
				g_pd3dDeviceContext->HSSetShader(NULL, NULL, 0);
				g_pd3dDeviceContext->DSSetShader(NULL, NULL, 0);
				g_pd3dDeviceContext->CSSetShader(NULL, NULL, 0);
				D3D11_RECT scissor = {0, 0, (LONG)desc.Width, (LONG)desc.Height};
				g_pd3dDeviceContext->RSSetScissorRects(1, &scissor);
				
				// Setup blend state
				const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
				g_pd3dDeviceContext->OMSetBlendState(g_pBlendState, blend_factor, 0xffffffff);
				g_pd3dDeviceContext->OMSetDepthStencilState(g_pDepthStencilState, 0);
				g_pd3dDeviceContext->RSSetState(g_pRasterizerState);
				
				// Bind texture, Draw
				g_pd3dDeviceContext->PSSetShaderResources(0, 1, &panel->src_srv);
				g_pd3dDeviceContext->DrawIndexed(6, 0, 0);
			}
		}
		
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		
		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
		
		g_pSwapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync
	}
	
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	
	for (int i = 0; i < arrlen(image_panels); ++i)
	{
		ReleaseImagePanel(image_panels[i]);
	}
	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
	
	return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	
#ifndef NDEBUG
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
    UINT createDeviceFlags = 0;
#endif
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
	{
        return false;
	}
	NAME_OBJECT(g_pd3dDevice);
	NAME_OBJECT(g_pSwapChain);
	
	HRESULT result = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&g_pDebug);
	if (result != S_OK) assert(false);
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	
	//g_pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	if (g_pDebug) {g_pDebug->Release(); g_pDebug = NULL;}
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
	
    switch (msg)
    {
		case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
		case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
		case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
		case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}