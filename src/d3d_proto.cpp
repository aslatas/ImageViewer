
#include "d3d_proto.h"

ID3D11Buffer* CreateVertexBuffer(ID3D11Device* device, ID3D11DeviceContext* ctx, CoolVertex* vertices, size_t vertex_count)
{
	assert(vertices && vertex_count);
	D3D11_BUFFER_DESC desc = {};
	
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = (UINT)(vertex_count * sizeof(vertices[0]));
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	
	ID3D11Buffer* result = 0;
	if (device->CreateBuffer(&desc, 0, &result) < 0)
	{
		assert(false); // Uh oh
	}
	
	D3D11_MAPPED_SUBRESOURCE mapped;
	
	if (ctx->Map(result, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) != S_OK)
	{
		assert(false);
	};
	
	CopyMemory(mapped.pData, vertices, vertex_count * sizeof(vertices[0]));
	
	ctx->Unmap(result, 0);
	return result;
}

ID3D11Buffer* CreateIndexBuffer(ID3D11Device* device, ID3D11DeviceContext* ctx, unsigned int* indices, size_t index_count)
{
	assert(indices && index_count);
	D3D11_BUFFER_DESC desc = {};
	
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = (UINT)(index_count * sizeof(indices[0]));
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	
	ID3D11Buffer* result = 0;
	if (device->CreateBuffer(&desc, 0, &result) < 0)
	{
		assert(false); // Uh oh
	}
	
	D3D11_MAPPED_SUBRESOURCE mapped;
	
	if (ctx->Map(result, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) != S_OK)
	{
		assert(false);
	};
	
	CopyMemory(mapped.pData, indices, index_count * sizeof(indices[0]));
	
	ctx->Unmap(result, 0);
	return result;
}

ID3D11Buffer* CreateConstantBuffer(ID3D11Device* device, ID3D11DeviceContext* ctx, CoolConstantBuffer* buffer)
{
	assert(buffer);
	
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(*buffer);
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	
	ID3D11Buffer* result = 0;
	if (device->CreateBuffer(&desc, 0, &result) < 0)
	{
		assert(false); // Uh oh
	}
	
	D3D11_MAPPED_SUBRESOURCE mapped;
	
	if (ctx->Map(result, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) != S_OK)
	{
		assert(false);
	};
	
	CopyMemory(mapped.pData, buffer, sizeof(*buffer));
	
	ctx->Unmap(result, 0);
	return result;
}