
#include "ImageLoader.h"
#include "d3d_proto.h"

ImagePanel LoadImageFromFile(ID3D11Device* device, ID3D11DeviceContext* ctx, char* image_path, int panel_id, Vec2 viewport_size)
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
	
	result.source_data = stbi_load(image_path, &result.source_width, &result.source_height, &result.source_channel_count, 4);
	
	
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
	sr_data.pSysMem = result.source_data;
	sr_data.SysMemPitch = tex_desc.Width * 4; // True only for 4 component images.
	sr_data.SysMemSlicePitch = 0;
	device->CreateTexture2D(&tex_desc, &sr_data, &result.texture);
	
	D3D11_SHADER_RESOURCE_VIEW_DESC src_srv_desc = {};
	src_srv_desc.Format = tex_desc.Format;
	src_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	src_srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	src_srv_desc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(result.texture, &src_srv_desc, &result.src_srv);
	
	D3D11_TEXTURE2D_DESC render_target_desc = {};
	render_target_desc.Width = (int)viewport_size.x;
	render_target_desc.Height = (int)viewport_size.y;
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
    result.last_image_size = viewport_size;
    result.selection_start = {-1, -1};
    result.selection_end = {-1, -1};
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
    
    stbi_image_free(image.source_data);
}

static Vec2 CanvasPosToImagePos(ImagePanel* panel, Vec2 canvas_pos)
{
    Vec2 src_half_size = panel->image_size / 2.0f;
    Vec2 dst_half_size = panel->last_image_size / 2.0f;
    Vec2 src_tl = dst_half_size - src_half_size + panel->image_offset;
    Vec2 src_br = src_tl + panel->image_size;
    Vec2 src_image_size = Vec2((float)panel->source_width, (float)panel->source_height);
    return (canvas_pos - src_tl) / (src_br - src_tl) * src_image_size;
}

static Vec2 ImagePosToCanvasPos(ImagePanel* panel, Vec2 image_pos)
{
    Vec2 src_half_size = panel->image_size / 2.0f;
    Vec2 dst_half_size = panel->last_image_size / 2.0f;
    Vec2 src_tl = dst_half_size - src_half_size + panel->image_offset;
    Vec2 src_br = src_tl + panel->image_size;
    Vec2 src_image_size = Vec2((float)panel->source_width, (float)panel->source_height);
    return (image_pos / src_image_size) * (src_br - src_tl) + src_tl;
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
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
		Vec2 relative_mouse_pos = mouse_pos - cursor_pos;
		ImTextureID tex_id = (ImTextureID)panel->dst_srv;
		ImGui::Image(tex_id, current_image_size, Vec2(0, 0), Vec2(1, 1));
		
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsItemHovered()) panel->is_dragging_rmb = true;
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) panel->is_dragging_rmb = false;
		if (panel->is_dragging_rmb && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
		{
			Vec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);
			Vec2 delta = {drag_delta.x, drag_delta.y};
			ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
			panel->should_redraw = true;
			panel->image_offset += delta;
		}
		
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
        {
            panel->is_dragging_lmb = true;
            
            Vec2 src_half_size = panel->image_size / 2.0f;
            Vec2 dst_half_size = current_image_size / 2.0f;
            Vec2 src_tl = dst_half_size - src_half_size + panel->image_offset;
            Vec2 src_br = src_tl + panel->image_size;
            Vec2 src_image_size = Vec2((float)panel->source_width, (float)panel->source_height);
            panel->selection_start = CanvasPosToImagePos(panel, relative_mouse_pos);
            panel->selection_start.x = Clamp(panel->selection_start.x, 0, panel->source_width);
            panel->selection_start.y = Clamp(panel->selection_start.y, 0, panel->source_height);
        }
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) panel->is_dragging_lmb = false;
		if (panel->is_dragging_lmb && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
		{
			ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
            Vec2 src_half_size = panel->image_size / 2.0f;
            Vec2 dst_half_size = current_image_size / 2.0f;
            Vec2 src_tl = dst_half_size - src_half_size + panel->image_offset;
            Vec2 src_br = src_tl + panel->image_size;
            Vec2 src_image_size = Vec2((float)panel->source_width, (float)panel->source_height);
            panel->selection_end = IVec2(CanvasPosToImagePos(panel, relative_mouse_pos));
            panel->selection_end.x = Clamp(panel->selection_end.x, 0, panel->source_width);
            panel->selection_end.y = Clamp(panel->selection_end.y, 0, panel->source_height);
		}
        
        
		static float zoom_sens = 1.1f;
		
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
        
        // If there is a selection rectangle, draw it.
        if (panel->selection_start.x >= 0 && panel->selection_start.y >= 0 && panel->selection_end.x >= 0 && panel->selection_end.y >= 0)
        {
            // If the user pressed ESC, clear the selection rectangle.
            if (ImGui::IsKeyPressed(VK_ESCAPE, false))
            {
                panel->selection_start = {-1, -1};
                panel->selection_end = {-1, -1};
            }
            else
            {
                
                IVec2 int_tl = IVec2(Min(panel->selection_start.x, panel->selection_end.x), Min(panel->selection_start.y, panel->selection_end.y));
                IVec2 int_br = IVec2(Max(panel->selection_start.x, panel->selection_end.x), Max(panel->selection_start.y, panel->selection_end.y)) + IVec2::One;
                
                int_tl.x = Clamp(int_tl.x, 0, panel->source_width - 1);
                int_tl.y = Clamp(int_tl.y, 0, panel->source_height - 1);
                int_br.x = Clamp(int_br.x, 0, panel->source_width);
                int_br.y = Clamp(int_br.y, 0, panel->source_height);
                Vec2 start = cursor_pos + ImagePosToCanvasPos(panel, panel->selection_start);
                Vec2 end = cursor_pos + ImagePosToCanvasPos(panel, panel->selection_end);
                
                Vec2 tl = cursor_pos + ImagePosToCanvasPos(panel, int_tl);
                Vec2 br = cursor_pos + ImagePosToCanvasPos(panel, int_br);
                dl->AddRect(tl, br, IM_COL32(255, 0, 255, 255));
            }
        }
        
		window_has_focus = ImGui::IsWindowFocused();
	}
	ImGui::End();
    ImGui::PopStyleVar(1);
	return window_has_focus;
}

bool SaveSelectedImagePanelRegion(ImagePanel* panel, const char* file_path, ImageExportParams params)
{
    bool result = false;
    if (panel->selection_start.x >= 0 && panel->selection_start.y >= 0 && panel->selection_end.x >= 0 && panel->selection_end.y >= 0)
    {
        IVec2 int_tl = IVec2(Min(panel->selection_start.x, panel->selection_end.x), Min(panel->selection_start.y, panel->selection_end.y));
        IVec2 int_br = IVec2(Max(panel->selection_start.x, panel->selection_end.x), Max(panel->selection_start.y, panel->selection_end.y)) + IVec2::One;
        
        int_tl.x = Clamp(int_tl.x, 0, panel->source_width - 1);
        int_tl.y = Clamp(int_tl.y, 0, panel->source_height - 1);
        int_br.x = Clamp(int_br.x, 0, panel->source_width);
        int_br.y = Clamp(int_br.y, 0, panel->source_height);
        
        result = (SaveImagePanelRect(panel, int_tl, int_br, file_path, params) != 0);
    }
    return result;
}
bool SaveImagePanel(ImagePanel* panel, const char* file_path, ImageExportParams params)
{
    IVec2 full_size = IVec2(panel->source_width, panel->source_height);
    return (SaveImagePanelRect(panel, IVec2::Zero, full_size, file_path, params) != 0);
}

bool SaveImagePanelRect(ImagePanel* panel, IVec2 top_left, IVec2 bottom_right, const char* file_path, ImageExportParams params)
{
    Assert(panel && file_path && file_path[0]);
    bool result = false;
    
    IVec2 full_size = IVec2(panel->source_width, panel->source_height);
    if (bottom_right.x < 0 || bottom_right.x > full_size.x) bottom_right.x = full_size.x;
    if (bottom_right.y < 0 || bottom_right.y > full_size.x) bottom_right.y = full_size.x;
    if (top_left.x < 0 || top_left.x >= full_size.x) top_left.x = 0;
    if (top_left.y < 0 || top_left.y >= full_size.y) top_left.y = 0;
    if (bottom_right.x <= top_left.x) bottom_right.x = full_size.x;
    if (bottom_right.y <= top_left.y) bottom_right.y = full_size.y;
    
    size_t start_offset = top_left.y * 4 * full_size.x + top_left.x * 4;
    unsigned char* start_ptr = panel->source_data + start_offset;
    int stride = full_size.x * 4;
    
    switch(params.type)
    {
        case ImageExportParams::FileType::PNG:
        {
            result = (stbi_write_png(file_path, bottom_right.x - top_left.x, bottom_right.y - top_left.y, 4, start_ptr, stride) != 0); 
        }
        break;
        default: break;
    }
    return result;
}