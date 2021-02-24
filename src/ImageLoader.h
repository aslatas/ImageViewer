// TODO(Matt): Add like a "draw" function here, so given an array of these structs, we can draw all the GUI windows.

#ifndef _IMAGE_LOADER_H
#define _IMAGE_LOADER_H

#include <d3d11.h>

struct ImagePanel
{
	ID3D11Texture2D* texture; // TODO(Matt): Free me.
	ID3D11Texture2D* render_target;
	
	ID3D11ShaderResourceView* src_srv;
	ID3D11ShaderResourceView* dst_srv;
	ID3D11RenderTargetView* rtv;
	
	ID3D11Buffer* vertex_buffer;
	ID3D11Buffer* index_buffer;
	ID3D11Buffer* constant_buffer;
	
	Vec2 image_size;
	Vec2 image_offset;
	Vec2 last_image_size;
	
	int source_width;
	int source_height;
	int source_channel_count; // Number of channels in the source image.
	
	int panel_id; // Unique ID of the panel (per app instance). Starts at 1 and increments for every new panel.
	
	char* file_path;
	char* file_name; // Pointer to the start of the name within file path.
	char* window_label; // Pointer to an imgui label string, concatenation of file name and panel ID.
	
	bool is_cursor_down_inside; // True if a right-click drag is happening that started in this panel.
	bool is_visible; // True if the panel is open. Panel will be deleted if this is false.
	
	bool should_redraw; // True if the image has changed state and needs to be re-drawn to the canvas.
	bool show_r;
	bool show_g;
	bool show_b;
	bool show_a;
};

ImagePanel LoadImageFromFile(ID3D11Device* device, ID3D11DeviceContext* ctx, char* image_path, int panel_id, int viewport_width, int viewport_height);
void ResizeImagePanelCanvas(ID3D11Device* device, ImagePanel* image, int width, int height);
void ReleaseImagePanel(ImagePanel image);

bool DrawImagePanel(ImagePanel* panel, ImGuiID dockspace_id, bool force_focus);
#endif //_IMAGE_LOADER_H
