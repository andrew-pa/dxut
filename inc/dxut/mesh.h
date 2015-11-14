#pragma once
#include "dxut\cmmn.h"
#include "dxut\DXDevice.h"
using namespace std;


struct vertex {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texcoord;
	DirectX::XMFLOAT3 tangent;
	vertex() {}
	vertex(FXMVECTOR p, FXMVECTOR n, FXMVECTOR tx, GXMVECTOR tg) {
		XMStoreFloat3(&position, p);
		XMStoreFloat3(&normal, n);
		XMStoreFloat3(&tangent, tg);
		XMStoreFloat2(&texcoord, tx);
	}
	vertex(float px, float py, float pz, float nx, float ny, float nz, float tx = 0, float ty = 0, float tz = 0, float u = 0, float v = 0)
		: position(px, py, pz), normal(nx, ny, nz), texcoord(u, v), tangent(tx, ty, tz)
	{}
};

struct mesh {
	ComPtr<ID3D12Resource> vbufres, ibufres;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW  ibv;
	uint32_t num_indices;

	mesh() { }

	mesh(DXDevice* dv, ComPtr<ID3D12GraphicsCommandList> commandList,
		void* vertices, size_t total_vertex_size, size_t vertex_stride, void* indices, size_t idxsz, uint32_t idxcnt);

	mesh(DXDevice* dv, ComPtr<ID3D12GraphicsCommandList> commandList,
		const vector<vertex>& vertices, const vector<uint32_t>& indices);

	//this function generates a mesh with only vec2f positions in the vertex buffer
	static unique_ptr<mesh> create_full_screen_quad(DXDevice* dv,
		ComPtr<ID3D12GraphicsCommandList> commandList, XMFLOAT2 ext = XMFLOAT2(1.f, 1.f));

	void draw(ComPtr<ID3D12GraphicsCommandList> cmdlist) const {
		cmdlist->IASetVertexBuffers(0, 1, &vbv);
		cmdlist->IASetIndexBuffer(&ibv);
		cmdlist->DrawIndexedInstanced(num_indices, 1, 0, 0, 0);
	}
	void draw(ComPtr<ID3D12GraphicsCommandList> cmdlist, uint32_t num_instances, vector<D3D12_VERTEX_BUFFER_VIEW> instance_data) const {
		cmdlist->IASetVertexBuffers(0, 1, &vbv);
		cmdlist->IASetVertexBuffers(1, instance_data.size(), instance_data.data());
		cmdlist->IASetIndexBuffer(&ibv);
		cmdlist->DrawIndexedInstanced(num_indices, num_instances, 0, 0, 0);
	}
};


void generate_cube_mesh(vector<vertex>& vertices, vector<uint32_t>& indices, DirectX::XMFLOAT3 extents);
void generate_quad_mesh(vector<vertex>& vertices, vector<uint32_t>& indices, DirectX::XMFLOAT2 extents, bool xz = true);
void generate_plane_mesh(vector<vertex>& vertices, vector<uint32_t>& indices, XMFLOAT2 dims, XMFLOAT2 div, XMFLOAT3 norm = XMFLOAT3(0, 1, 0));
