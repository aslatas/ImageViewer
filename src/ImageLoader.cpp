
#include "ImageLoader.h"
#include "d3d_proto.h"

ImagePanel LoadImageFromFile(ID3D11Device* device, ID3D11DeviceContext* ctx, char* image_path, int panel_id, int viewport_width, int viewport_height)
{
	assert(image_path);
	ImagePanel result = {};
	result.file_path = image_path;
	
	size_t len = strlen(result.file_path);
	size_t start = len;
	while (start > 0 && result.file_path[start - 1] != '\\' && result.file_path[start - 1] != '/') --start;
	result.file_name = &result.file_path[start];
	
	size_t label_size = len - start + 12;
	char* label = (char*)malloc(label_size);
	sprintf_s(label, label_size, "%s###%d", result.file_name, panel_id);
	result.window_label = label;
	
	unsigned char* image_data = stbi_load(image_path, &result.source_width, &result.source_height, &result.source_channel_count, 4);
	
	
	D3D11_TEXTURE2D_DESC tex_desc = {};
	tex_desc.Width = result.source_width;
	tex_desc.Height = result.source_height;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags = 0;
	
	D3D11_SUBRESOURCE_DATA sr_data;
	sr_data.pSysMem = image_data;
	sr_data.SysMemPitch = tex_desc.Width * 4; // True only for 4 component images.
	sr_data.SysMemSlicePitch = 0;
	device->CreateTexture2D(&tex_desc, &sr_data, &result.texture);
	stbi_image_free(image_data);
	
	D3D11_SHADER_RESOURCE_VIEW_DESC src_srv_desc = {};
	src_srv_desc.Format = tex_desc.Format;
	src_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	src_srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	src_srv_desc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(result.texture, &src_srv_desc, &result.src_srv);
	
	D3D11_TEXTURE2D_DESC render_target_desc = {};
	render_target_desc.Width = viewport_width;
	render_target_desc.Height = viewport_height;
	render_target_desc.MipLevels = render_target_desc.ArraySize = 1;
	render_target_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	render_target_desc.SampleDesc.Count = 1;
	render_target_desc.Usage = D3D11_USAGE_DEFAULT;
	render_target_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	render_target_desc.CPUAccessFlags = 0;
	render_target_desc.MiscFlags = 0;
	
	device->CreateTexture2D( &render_target_desc, 0, &result.render_target);
	
	D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.Format = render_target_desc.Format;
	rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtv_desc.Texture2D.MipSlice = 0;
	
	device->CreateRenderTargetView(result.render_target, &rtv_desc, &result.rtv);
	
	D3D11_SHADER_RESOURCE_VIEW_DESC dst_srv_desc = {};
	dst_srv_desc.Format = rtv_desc.Format;
	dst_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	dst_srv_desc.Texture2D.MipLevels = 1;
	dst_srv_desc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(result.render_target, &dst_srv_desc, &result.dst_srv);
	
	CoolVertex vertices[4] =
	{
		{{-1, -1}, {0, 0}, {255, 255, 255 ,255}},
		{{1, -1}, {1, 0}, {255, 255, 255 ,255}},
		{{1, 1}, {1, 1}, {255, 255, 255 ,255}},
		{{-1, 1}, {0, 1}, {255, 255, 255 ,255}}
	};
	unsigned int indices[] = {0, 1, 2, 2, 3, 0};
	CoolConstantBuffer constant_buffer =
	{
		{
            {1, 0, 0, 0},
            {0, -1, 0, 0},
            {0, 0, 0.5f, 0},
            {0, 0, 0.5f, 1},
        },
		{},
		{}
	};
	result.vertex_buffer = CreateVertexBuffer(device, ctx, vertices, 4);
	result.index_buffer = CreateIndexBuffer(device, ctx, indices, 6);
	result.constant_buffer = CreateConstantBuffer(device, ctx, &constant_buffer);
	
	result.image_offset = {};
	result.image_size = Vec2((float)result.source_width, (float)result.source_height);
	result.is_visible = true;
	result.should_redraw = true;
	result.panel_id = panel_id;
	return result;
}

void ReleaseImagePanel(ImagePanel image)
{
	image.texture->Release();
	image.render_target->Release();
	image.src_srv->Release();
	image.dst_srv->Release();
	image.rtv->Release();
	image.vertex_buffer->Release();
	image.index_buffer->Release();
	image.constant_buffer->Release();
	
	free(image.file_path);
	free(image.window_label);
}

void ResizeImagePanelCanvas(ID3D11Device* device, ImagePanel* image, int width, int height)
{
	assert(image);
	if (image->render_target) image->render_target->Release();
	if (image->rtv) image->rtv->Release();
	if (image->dst_srv) image->dst_srv->Release();
	
	D3D11_TEXTURE2D_DESC render_target_desc = {};
	render_target_desc.Width = width;
	render_target_desc.Height = height;
	render_target_desc.MipLevels = render_target_desc.ArraySize = 1;
	render_target_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	render_target_desc.SampleDesc.Count = 1;
	render_target_desc.Usage = D3D11_USAGE_DEFAULT;
	render_target_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	render_target_desc.CPUAccessFlags = 0;
	render_target_desc.MiscFlags = 0;
	
	device->CreateTexture2D( &render_target_desc, 0, &image->render_target);
	
	D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.Format = render_target_desc.Format;
	rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtv_desc.Texture2D.MipSlice = 0;
	
	device->CreateRenderTargetView(image->render_target, &rtv_desc, &image->rtv);
	
	D3D11_SHADER_RESOURCE_VIEW_DESC dst_srv_desc = {};
	dst_srv_desc.Format = rtv_desc.Format;
	dst_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	dst_srv_desc.Texture2D.MipLevels = 1;
	dst_srv_desc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(image->render_target, &dst_srv_desc, &image->dst_srv);
}

// Returns true if the image panel has focus.
bool DrawImagePanel(ImagePanel* panel, ImGuiID dockspace_id, bool force_focus)
{
	assert(panel);
	bool window_has_focus = false;
	if (!panel->is_visible) return window_has_focus;
	
	ImGuiIO& io = ImGui::GetIO();
	if (dockspace_id) ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Appearing);
	if (force_focus) ImGui::SetNextWindowFocus();
	if (ImGui::Begin(panel->window_label, &panel->is_visible))
	{
		
		Vec2 current_image_size = ImGui::GetContentRegionAvail();
		if (current_image_size.x < 1.0f) current_image_size.x = 1.0f;
		if (current_image_size.y < 1.0f) current_image_size.y = 1.0f;
		
		if (panel->last_image_size != current_image_size)
		{
			panel->should_redraw = true;
			panel->last_image_size = current_image_size;
			ResizeImagePanelCanvas(g_pd3dDevice, panel, (int)current_image_size.x, (int)current_image_size.y);
		}
		Vec2 cursor_pos = ImGui::GetCursorScreenPos();
		Vec2 mouse_pos = ImGui::GetMousePos();
		ImTextureID tex_id = (ImTextureID)panel->dst_srv;
		ImGui::Image(tex_id, current_image_size, Vec2(0, 0), Vec2(1, 1));
		
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsItemHovered()) panel->is_cursor_down_inside = true;
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) panel->is_cursor_down_inside = false;
		if (panel->is_cursor_down_inside && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
		{
			Vec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);
			Vec2 delta = {drag_delta.x, drag_delta.y};
			ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
			panel->should_redraw = true;
			panel->image_offset += delta;
		}
		
		static float zoom_sens = 1.1f;
		Vec2 relative_mouse_pos = mouse_pos - cursor_pos;
		
		Vec2 src_half_size = panel->image_size / 2.0f;
		Vec2 dst_half_size = current_image_size / 2.0f;
		Vec2 src_tl = dst_half_size - src_half_size + panel->image_offset;
		Vec2 src_br = src_tl + panel->image_size;
		
		Vec2 src_relative_pos = (relative_mouse_pos - src_tl) / (src_br - src_tl);
		if (io.MouseWheel && ImGui::IsItemHovered())
		{
			float amt = Pow(zoom_sens, io.MouseWheel);
			panel->image_size *= amt;
			Vec2 dst_tl = relative_mouse_pos - (src_relative_pos * panel->image_size);
			Vec2 new_half_size = panel->image_size / 2.0f;
			Vec2 desired_tl = relative_mouse_pos - (src_relative_pos * panel->image_size);
			panel->should_redraw = true;
			panel->image_offset = desired_tl + new_half_size - dst_half_size;
		}
		Vec2 dst_tl = relative_mouse_pos - (src_relative_pos * panel->image_size);
		
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRect(cursor_pos + dst_tl, cursor_pos + dst_tl + panel->image_size, ImGui::GetColorU32(ImGuiCol_Border));
		
		window_has_focus = ImGui::IsWindowFocused();
	}
	ImGui::End();
	return window_has_focus;
}