#ifndef D3D_PROTO_H
#define D3D_PROTO_H

struct CoolConstantBuffer
{
	float transform[4][4];
	unsigned int int_vals[4];
	float float_vals[4];
};

struct CoolVertex
{
	float pos[2];
	float uv[2];
	unsigned char col[4];
};

ID3D11Buffer* CreateVertexBuffer(ID3D11Device* device, ID3D11DeviceContext* ctx, CoolVertex* vertices, size_t vertex_count);
ID3D11Buffer* CreateIndexBuffer(ID3D11Device* device, ID3D11DeviceContext* ctx, unsigned int* indices, size_t index_count);
ID3D11Buffer* CreateConstantBuffer(ID3D11Device* device, ID3D11DeviceContext* ctx, CoolConstantBuffer* buffer);
#endif //D3D_PROTO_H
