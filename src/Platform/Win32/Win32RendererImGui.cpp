
#include "Win32Renderer.h"
#include "imgui/imgui.h"
#include <d3d12.h>
#include <dxgi1_6.h>

struct ImGuiFrameResources
{
    ID3D12Resource* index_buffer;
    ID3D12Resource* vertex_buffer;
	u32 vertex_buffer_size;
	u32 index_buffer_size;
};

static ID3D12Device* imgui_d3d_device = 0;
static ID3D12RootSignature*imgui_d3d_root_signature = 0;
static ID3D12PipelineState* imgui_d3d_pipeline_state = 0;
static DXGI_FORMAT imgui_rtv_format = DXGI_FORMAT_UNKNOWN;
static ID3D12Resource* imgui_font_texture_resource = 0;
static D3D12_CPU_DESCRIPTOR_HANDLE imgui_font_srv_cpu_desc_handle = {};
static D3D12_GPU_DESCRIPTOR_HANDLE  imgui_font_srv_gpu_desc_handle = {};

static ImGuiFrameResources*  imgui_frame_resources = 0;
static u32 imgui_frames_in_flight_count = 0;
static u32 imgui_frame_index = U32_MAX;

static void SetupImGuiRenderState(ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx, ImGuiFrameResources* fr);
static void CreateImGuiFontsTexture();

bool InitializeImGuiRenderer(ID3D12Device* device,
							 u32 in_flight_frame_count,
							 DXGI_FORMAT rtv_format,
							 ID3D12DescriptorHeap* cbv_srv_heap,
							 D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle)
{
	// Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "Prototype ImGui DX12 Integration";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	
	imgui_d3d_device = device;
	imgui_frames_in_flight_count = in_flight_frame_count;
	imgui_rtv_format = rtv_format;
	imgui_font_srv_cpu_desc_handle = font_srv_cpu_desc_handle;
	imgui_font_srv_gpu_desc_handle = font_srv_gpu_desc_handle;
	imgui_frame_resources = (ImGuiFrameResources*)malloc(sizeof(ImGuiFrameResources) * imgui_frames_in_flight_count); // @malloc
    imgui_frame_index = U32_MAX;
    IM_UNUSED(cbv_srv_heap); // Unused in master branch (will be used by multi-viewports)
	
    // Create buffers with a default size (they will later be grown as needed)
    for (u32 i = 0; i < imgui_frames_in_flight_count; ++i)
    {
        ImGuiFrameResources* fr = &imgui_frame_resources[i];
        fr->index_buffer = 0;
        fr->vertex_buffer = 0;
        fr->index_buffer_size = 10000;
        fr->vertex_buffer_size = 5000;
    }
	
    return true;
}

void ShutdownImGuiRenderer()
{
    InvalidateDeviceObjectsForImGui();
	free(imgui_frame_resources); // @malloc
	imgui_frame_resources = 0;
	imgui_d3d_device = 0;
	imgui_rtv_format = DXGI_FORMAT_UNKNOWN;
	imgui_font_srv_cpu_desc_handle.ptr = 0;
	imgui_font_srv_gpu_desc_handle.ptr = 0;
	imgui_frames_in_flight_count = 0;
    imgui_frame_index = U32_MAX;
}

void RenderImGuiFrame()
{
	if (!imgui_d3d_pipeline_state) CreateDeviceObjectsForImGui();
}

void RenderImGuiDrawData(ID3D12GraphicsCommandList* ctx)
{
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	
	// Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f) return;
	
    // FIXME: I'm assuming that this only gets called once per frame!
    // If not, we can't just re-allocate the IB or VB, we'll have to do a proper allocator.
    imgui_frame_index = imgui_frame_index + 1;
    ImGuiFrameResources* fr = &imgui_frame_resources[imgui_frame_index % imgui_frames_in_flight_count];
	
    // Create and grow vertex/index buffers if needed
    if (fr->vertex_buffer == NULL || fr->vertex_buffer_size < (u32)draw_data->TotalVtxCount)
    {
		if (fr->vertex_buffer) fr->vertex_buffer->Release();
        fr->vertex_buffer_size = draw_data->TotalVtxCount + 5000;
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = fr->vertex_buffer_size * sizeof(ImDrawVert);
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (imgui_d3d_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&fr->vertex_buffer)) < 0)
            return;
    }
    if (fr->index_buffer == NULL || fr->index_buffer_size < (u32)draw_data->TotalIdxCount)
    {
		if (fr->index_buffer) fr->index_buffer->Release();
        fr->index_buffer_size = draw_data->TotalIdxCount + 10000;
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = fr->index_buffer_size * sizeof(ImDrawIdx);
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (imgui_d3d_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, _uuidof(ID3D12Resource), (void**)&fr->index_buffer)) return;
    }
	
    // Upload vertex/index data into a single contiguous GPU buffer
    void* vtx_resource, *idx_resource;
    D3D12_RANGE range = {};
    if (fr->vertex_buffer->Map(0, &range, &vtx_resource) != S_OK) return;
    if (fr->index_buffer->Map(0, &range, &idx_resource) != S_OK) return;
    ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource;
    ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource;
    for (u32 n = 0; n < (u32)draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        CopyMemory(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        CopyMemory(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    fr->vertex_buffer->Unmap(0, &range);
    fr->index_buffer->Unmap(0, &range);
	
    // Setup desired DX state.
    SetupImGuiRenderState(draw_data, ctx, fr);
	
    // Render command lists.
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (u32 n = 0; n < (u32)draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (u32 i = 0; i < (u32)cmd_list->CmdBuffer.Size; ++i)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[i];
            if (pcmd->UserCallback)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) SetupImGuiRenderState(draw_data, ctx, fr);
                else pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Apply Scissor, Bind texture, Draw
                const D3D12_RECT r = {(LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y)};
				
				u64 tex_data = (u64)pcmd->TextureId;
				u32 layer = (u32)(tex_data & 0xffffffff);
				u32 idx = (u32)(tex_data >> 32);
				D3D12_GPU_DESCRIPTOR_HANDLE handle = render::g_context->shader_visible_heap.GetGPUHandle(idx);
				ctx->SetGraphicsRoot32BitConstant(2, layer, 0);
                ctx->SetGraphicsRootDescriptorTable(1, handle);
                ctx->RSSetScissorRects(1, &r);
                ctx->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

void InvalidateDeviceObjectsForImGui()
{
	if (!imgui_d3d_device) return;
	
	if (imgui_d3d_root_signature) imgui_d3d_root_signature->Release();
    if (imgui_d3d_pipeline_state) imgui_d3d_pipeline_state->Release();
    if (imgui_font_texture_resource) imgui_font_texture_resource->Release();
	
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->TexID = NULL;
	
    for (u32 i = 0; i < imgui_frames_in_flight_count; ++i)
    {
        ImGuiFrameResources* fr = &imgui_frame_resources[i];
		if (fr->index_buffer) fr->index_buffer->Release();
        if (fr->vertex_buffer) fr->vertex_buffer->Release();
    }
}

bool CreateDeviceObjectsForImGui()
{
	if (!imgui_d3d_device) return false;
    if (imgui_d3d_pipeline_state) InvalidateDeviceObjectsForImGui();
	
    // Create the root signature
    {
        D3D12_DESCRIPTOR_RANGE range = {};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0;
        range.RegisterSpace = 0;
        range.OffsetInDescriptorsFromTableStart = 0;
		
        D3D12_ROOT_PARAMETER params[3] = {};
		
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[0].Constants.ShaderRegister = 0;
        params[0].Constants.RegisterSpace = 0;
        params[0].Constants.Num32BitValues = 16;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.NumDescriptorRanges = 1;
        params[1].DescriptorTable.pDescriptorRanges = &range;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		
		params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[2].Constants.ShaderRegister = 1;
        params[2].Constants.RegisterSpace = 0;
        params[2].Constants.Num32BitValues = 1;
        params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		
        D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
        sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler_desc.MipLODBias = 0.f;
        sampler_desc.MaxAnisotropy = 0;
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler_desc.MinLOD = 0.f;
        sampler_desc.MaxLOD = 0.f;
        sampler_desc.ShaderRegister = 0;
        sampler_desc.RegisterSpace = 0;
        sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = ARRAYCOUNT(params);
        desc.pParameters = params;
        desc.NumStaticSamplers = 1;
        desc.pStaticSamplers = &sampler_desc;
        desc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		
        ID3DBlob* blob = 0;
        if (D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, 0) != S_OK) return false;
		
        imgui_d3d_device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), _uuidof(ID3D12RootSignature), (void**)&imgui_d3d_root_signature);
        blob->Release();
    }
	
	// Load the shader compiler library.
	HMODULE d3d_compiler_library = LoadLibraryA("d3dcompiler_47.dll");
	typedef HRESULT (*PFN_D3DCOMPILE)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
	PFN_D3DCOMPILE _D3DCompile = 0;
	_D3DCompile = (PFN_D3DCOMPILE)GetProcAddress(d3d_compiler_library, "D3DCompile");
	
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.NodeMask = 1;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.pRootSignature = imgui_d3d_root_signature;
    pso_desc.SampleMask = U32_MAX;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = imgui_rtv_format;
    pso_desc.SampleDesc.Count = 1;
    pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	
    ID3DBlob* vertex_blob;
    ID3DBlob* pixel_blob;
	
    // Create the vertex shader
    {
        static const char* vertex_shader =
            "cbuffer vertexBuffer : register(b0) \
		{\
			float4x4 ProjectionMatrix; \
		};\
		struct VS_INPUT\
		{\
			float2 pos : POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
		};\
		\
		struct PS_INPUT\
		{\
			float4 pos : SV_POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
		};\
		\
		PS_INPUT main(VS_INPUT input)\
		{\
			PS_INPUT output;\
			output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
			output.col = input.col;\
			output.uv  = input.uv;\
			return output;\
		}";
		
		if (FAILED(_D3DCompile(vertex_shader, StrLen(vertex_shader), 0, 0, 0, "main", "vs_5_0", 0, 0, &vertex_blob, 0))) return false;
        pso_desc.VS = {vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize()};
		
        // Create the input layout
        static D3D12_INPUT_ELEMENT_DESC local_layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        pso_desc.InputLayout = { local_layout, 3 };
    }
	
    // Create the pixel shader
    {
        static const char* pixel_shader =
            "cbuffer uniform_input : register(b1) \
{\
  uint layer;\
	};\
	struct PS_INPUT\
		{\
			float4 pos : SV_POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
		};\
		SamplerState sampler0 : register(s0);\
		Texture2DArray texture0 : register(t0);\
		\
		float4 main(PS_INPUT input) : SV_Target\
		{\
			float4 out_col = input.col * texture0.Sample(sampler0, float3(input.uv, (float)layer)); \
			return out_col; \
		}";
		
		if (FAILED(_D3DCompile(pixel_shader, StrLen(pixel_shader), 0, 0, 0, "main", "ps_5_0", 0, 0, &pixel_blob, 0)))
        {
            vertex_blob->Release();
            return false;
        }
        pso_desc.PS = {pixel_blob->GetBufferPointer(), pixel_blob->GetBufferSize()};
    }
	
    // Create the blending setup
    {
        D3D12_BLEND_DESC& desc = pso_desc.BlendState;
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
	
    // Create the rasterizer state
    {
        D3D12_RASTERIZER_DESC& desc = pso_desc.RasterizerState;
        desc.FillMode = D3D12_FILL_MODE_SOLID;
        desc.CullMode = D3D12_CULL_MODE_NONE;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        desc.DepthClipEnable = true;
        desc.MultisampleEnable = FALSE;
        desc.AntialiasedLineEnable = FALSE;
        desc.ForcedSampleCount = 0;
        desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }
	
    // Create depth-stencil State
    {
        D3D12_DEPTH_STENCIL_DESC& desc = pso_desc.DepthStencilState;
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.BackFace = desc.FrontFace;
    }
	
    HRESULT result_pipeline_state = imgui_d3d_device->CreateGraphicsPipelineState(&pso_desc, _uuidof(ID3D12PipelineState), (void**)&imgui_d3d_pipeline_state);
    vertex_blob->Release();
    pixel_blob->Release();
	_D3DCompile = 0;
	FreeLibrary(d3d_compiler_library);
    if (result_pipeline_state != S_OK) return false;
	
    CreateImGuiFontsTexture();
	
    return true;
}

static void SetupImGuiRenderState(ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx, ImGuiFrameResources* fr)
{
    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
	
	
	float L = draw_data->DisplayPos.x;
	float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float T = draw_data->DisplayPos.y;
	float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
	
	Mat4 vertex_constant_buffer = {};
	vertex_constant_buffer[0] = {2.0f/(R-L), 0.0f, 0.0f, 0.0f};;
	vertex_constant_buffer[1] = {0.0f, 2.0f/(T-B), 0.0f, 0.0f};
	vertex_constant_buffer[2] = {0.0f, 0.0f, 0.5f, 0.0f};
	vertex_constant_buffer[3] = {(R+L)/(L-R), (T+B)/(B-T), 0.5, 1.0f};
    
	// Setup viewport
    D3D12_VIEWPORT vp = {};
    vp.Width = draw_data->DisplaySize.x;
    vp.Height = draw_data->DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0.0f;
    ctx->RSSetViewports(1, &vp);
	
    // Bind shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    D3D12_VERTEX_BUFFER_VIEW vbv;
    memset(&vbv, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
    vbv.BufferLocation = fr->vertex_buffer->GetGPUVirtualAddress() + offset;
    vbv.SizeInBytes = fr->vertex_buffer_size * stride;
    vbv.StrideInBytes = stride;
    ctx->IASetVertexBuffers(0, 1, &vbv);
    D3D12_INDEX_BUFFER_VIEW ibv;
    memset(&ibv, 0, sizeof(D3D12_INDEX_BUFFER_VIEW));
    ibv.BufferLocation = fr->index_buffer->GetGPUVirtualAddress();
    ibv.SizeInBytes = fr->index_buffer_size * sizeof(ImDrawIdx);
    ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    ctx->IASetIndexBuffer(&ibv);
    ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->SetPipelineState(imgui_d3d_pipeline_state);
    ctx->SetGraphicsRootSignature(imgui_d3d_root_signature);
    ctx->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);
	
    // Setup blend factor
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    ctx->OMSetBlendFactor(blend_factor);
}

static void CreateImGuiFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    u8* pixels;
	s32 width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    // Upload texture to graphics system
    {
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_DEFAULT;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment = 0;
        desc.Width = width;
        desc.Height = height;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		
        ID3D12Resource* texture_ptr = 0;
        imgui_d3d_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
												  D3D12_RESOURCE_STATE_COPY_DEST, NULL, _uuidof(ID3D12Resource), (void**)&texture_ptr);
		
		u32 upload_pitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
		u32 upload_size = height * upload_pitch;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = upload_size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		
        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		
        ID3D12Resource* upload_buffer = 0;
        HRESULT hr = imgui_d3d_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
															   D3D12_RESOURCE_STATE_GENERIC_READ, 0, _uuidof(ID3D12Resource), (void**)&upload_buffer);
        IM_ASSERT(SUCCEEDED(hr));
		
        void* mapped = 0;
        D3D12_RANGE range = {0, upload_size};
        hr = upload_buffer->Map(0, &range, &mapped);
        IM_ASSERT(SUCCEEDED(hr));
        for (int y = 0; y < height; y++)
            CopyMemory((void*) ((uintptr_t) mapped + y * upload_pitch), pixels + y * width * 4, width * 4);
        upload_buffer->Unmap(0, &range);
		
        D3D12_TEXTURE_COPY_LOCATION src_location = {};
        src_location.pResource = upload_buffer;
        src_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src_location.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src_location.PlacedFootprint.Footprint.Width = width;
        src_location.PlacedFootprint.Footprint.Height = height;
        src_location.PlacedFootprint.Footprint.Depth = 1;
        src_location.PlacedFootprint.Footprint.RowPitch = upload_pitch;
		
        D3D12_TEXTURE_COPY_LOCATION dst_location = {};
        dst_location.pResource = texture_ptr;
        dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst_location.SubresourceIndex = 0;
		
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = texture_ptr;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		
        ID3D12Fence* fence = 0;
        hr = imgui_d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, _uuidof(ID3D12Fence), (void**)&fence);
        IM_ASSERT(SUCCEEDED(hr));
		
        HANDLE event = CreateEvent(0, 0, 0, 0);
        IM_ASSERT(event != NULL);
		
        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queue_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queue_desc.NodeMask = 1;
		
        ID3D12CommandQueue* cmd_queue = 0;
        hr = imgui_d3d_device->CreateCommandQueue(&queue_desc, _uuidof(ID3D12CommandQueue), (void**)&cmd_queue);
        IM_ASSERT(SUCCEEDED(hr));
		
        ID3D12CommandAllocator* cmd_alloc = 0;
        hr = imgui_d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, _uuidof(ID3D12CommandAllocator), (void**)&cmd_alloc);
        IM_ASSERT(SUCCEEDED(hr));
		
        ID3D12GraphicsCommandList* cmd_list = 0;
        hr = imgui_d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_alloc, 0, _uuidof(ID3D12GraphicsCommandList), (void**)&cmd_list);
        IM_ASSERT(SUCCEEDED(hr));
		
        cmd_list->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, 0);
        cmd_list->ResourceBarrier(1, &barrier);
		
        hr = cmd_list->Close();
        IM_ASSERT(SUCCEEDED(hr));
		
        cmd_queue->ExecuteCommandLists(1, (ID3D12CommandList* const*) &cmd_list);
        hr = cmd_queue->Signal(fence, 1);
        IM_ASSERT(SUCCEEDED(hr));
		
        fence->SetEventOnCompletion(1, event);
        WaitForSingleObject(event, INFINITE);
		
        cmd_list->Release();
        cmd_alloc->Release();
        cmd_queue->Release();
        CloseHandle(event);
        fence->Release();
        upload_buffer->Release();
		
        // Create texture view
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = desc.MipLevels;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		imgui_d3d_device->CreateShaderResourceView(texture_ptr, &srv_desc, imgui_font_srv_cpu_desc_handle);
		if (imgui_font_texture_resource) imgui_font_texture_resource->Release();
		imgui_font_texture_resource = texture_ptr;
    }
	
    // Store our identifier
    io.Fonts->TexID = (void*)(((u64)render::EShaderDescriptor::Texture0) << 32 | 0);
}