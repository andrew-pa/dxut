#include "dxut\cmmn.h"
#include "dxut\mesh.h"

using namespace DirectX;
using namespace std;

mesh::mesh(DXDevice* dv, ComPtr<ID3D12GraphicsCommandList> commandList,
	void* vertices, size_t total_vertex_size, size_t vertex_stride, void* indices, size_t indices_size, uint32_t idxcnt)
{	
	D3D12_SUBRESOURCE_DATA srd = {};
#pragma region vertices

	chk(dv->device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(total_vertex_size),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vbufres)));
	
	auto vbufup = dv->new_upload_resource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(total_vertex_size),
		D3D12_RESOURCE_STATE_GENERIC_READ);
	srd.pData = vertices;
	srd.RowPitch = total_vertex_size;
	srd.SlicePitch = srd.RowPitch;

	UpdateSubresources<1>(commandList.Get(), vbufres.Get(), vbufup.Get(), 0, 0, 1, &srd);

	vbv.BufferLocation = vbufres->GetGPUVirtualAddress();
	vbv.StrideInBytes = vertex_stride;
	vbv.SizeInBytes = total_vertex_size;
#pragma endregion

#pragma region indices

	chk(dv->device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices_size),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&ibufres)));
	auto ibufup = dv->new_upload_resource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices_size),
		D3D12_RESOURCE_STATE_GENERIC_READ);
	srd.pData = indices;
	srd.RowPitch = indices_size;
	srd.SlicePitch = srd.RowPitch;

	UpdateSubresources<1>(commandList.Get(), ibufres.Get(), ibufup.Get(), 0, 0, 1, &srd);

	ibv.BufferLocation = ibufres->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R32_UINT;
	ibv.SizeInBytes = indices_size;

	num_indices = idxcnt;
#pragma endregion

	D3D12_RESOURCE_BARRIER barriers[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(vbufres.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
		CD3DX12_RESOURCE_BARRIER::Transition(ibufres.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER)
	};
	commandList->ResourceBarrier(2, barriers);
	
}


void mesh::create_instance_buffer(DXDevice * dv, ComPtr<ID3D12GraphicsCommandList> cmdlist, 
	void * data, size_t total_data_size, size_t stride, D3D12_VERTEX_BUFFER_VIEW * vbv, ComPtr<ID3D12Resource>& res) {

	chk(dv->device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(total_data_size),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&res)));
	auto bufup = dv->new_upload_resource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(total_data_size),
		D3D12_RESOURCE_STATE_GENERIC_READ);
	D3D12_SUBRESOURCE_DATA srd = {};
	srd.pData = data;
	srd.RowPitch = total_data_size;
	srd.SlicePitch = srd.RowPitch;

	UpdateSubresources<1>(cmdlist.Get(), res.Get(), bufup.Get(), 0, 0, 1, &srd);

	vbv->BufferLocation = res->GetGPUVirtualAddress();
	vbv->SizeInBytes = total_data_size;
	vbv->StrideInBytes = stride;

	cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
}

mesh::mesh(DXDevice* dv, ComPtr<ID3D12GraphicsCommandList> commandList,
	const vector<vertex>& vertices, const vector<uint32_t>& indices)
	: mesh(dv, commandList, 
		(void*)vertices.data(), sizeof(vertex)*vertices.size(), sizeof(vertex),
		(void*)indices.data(),  sizeof(uint32_t)*indices.size(), indices.size())
{}



void generate_cube_mesh(vector<vertex>& v, vector<uint32_t>& i, XMFLOAT3 extents) {
	//vertex* v = new vertex[24];
	v.resize(24);

	float w2 = 0.5f*extents.x;
	float h2 = 0.5f*extents.y;
	float d2 = 0.5f*extents.z;

	// Fill in the front face vertex data.
	v[0] = vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8] = vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9] = vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	//UINT* i = new UINT[36];
	i.resize(36);

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

}
void generate_quad_mesh(vector<vertex>& v, vector<uint32_t>& i, DirectX::XMFLOAT2 extents, bool xz) {
	v.resize(4);
	if (xz) {
		v[0] = vertex(extents.x, 0, extents.y,   0.f, 1.f, 0.f);
		v[1] = vertex(extents.x, 0, -extents.y,  0.f, 1.f, 0.f, 0.f, 1.f);
		v[2] = vertex(-extents.x, 0, -extents.y, 0.f, 1.f, 0.f, 1.f, 1.f);
		v[3] = vertex(-extents.x, 0, extents.y,  0.f, 1.f, 0.f, 1.f, 0.f);
	} else {
		v[0] = vertex(extents.x, extents.y,   0.f, 0.f, 0.f, 1.f);
		v[1] = vertex(extents.x, -extents.y,  0.f, 0.f, 0.f, 1.f, 0.f, 1.f);
		v[2] = vertex(-extents.x, -extents.y, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f);
		v[3] = vertex(-extents.x, extents.y,  0.f, 0.f, 0.f, 1.f, 1.f, 0.f);
	}
	i.resize(6);
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 2; i[4] = 3; i[5] = 0;
}

unique_ptr<mesh> mesh::create_full_screen_quad(DXDevice* dv, ComPtr<ID3D12GraphicsCommandList> commandList, XMFLOAT2 extents) {
	vector<XMFLOAT2> v(4);
	v[0] = XMFLOAT2(extents.x, extents.y);
	v[1] = XMFLOAT2(extents.x, -extents.y);
	v[2] = XMFLOAT2(-extents.x, -extents.y);
	v[3] = XMFLOAT2(-extents.x, extents.y);
	vector<uint32_t> i(6);
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 2; i[4] = 3; i[5] = 0;
	return make_unique<mesh>(
		dv, commandList, (void*)v.data(), v.size()*sizeof(XMFLOAT2), sizeof(XMFLOAT2), (void*)i.data(), i.size()*sizeof(uint32_t), i.size());
}


void generate_plane_mesh(vector<vertex>& vertices, vector<uint32_t>& indices, XMFLOAT2 dims, XMFLOAT2 div, XMFLOAT3 norm)
{

	XMVECTOR nw = XMVector3Normalize(XMLoadFloat3(&norm));
	XMVECTOR t = (fabsf(XMVectorGetX(nw)) > .1 ? XMVectorSet(0, 1, 0, 0) : XMVectorSet(1, 0, 0, 0));
	XMVECTOR nu = XMVector3Normalize(XMVector3Cross(t, nw));
	XMVECTOR nv = XMVector3Cross(nw, nu);

	XMFLOAT2 hdims{ .5f*dims.x, .5f*dims.y };

	XMVECTOR divm1 = XMVectorSet(div.x - 1.f, div.y - 1.f, 0.f, 0.f);
	XMVECTOR dxy = XMLoadFloat2(&dims) / divm1;

	XMVECTOR duv = XMVectorReciprocal(divm1);

	for (float i = 0; i < div.y; ++i)
	{
		float y = hdims.y - i*XMVectorGetY(dxy);
		for (float j = 0; j < div.x; ++j)
		{
			float x = hdims.x - j*XMVectorGetX(dxy);

			XMVECTOR p = nu*x + nv*y;
			XMVECTOR tx = XMVectorSet(j, i, 0, 0)*duv;
			vertices.push_back(vertex(p, -nw, tx, nu));
		}
	}

	for (uint32_t i = 0; i < div.x - 1; ++i)
	{
		for (uint32_t j = 0; j < div.y - 1; ++j)
		{
			indices.push_back(i*(uint16_t)div.y + j);
			indices.push_back(i*(uint16_t)div.y + j + 1);
			indices.push_back((i + 1)*(uint16_t)div.y + j);

			indices.push_back((i + 1)*(uint16_t)div.y + j);
			indices.push_back(i*(uint16_t)div.y + j + 1);
			indices.push_back((i + 1)*(uint16_t)div.y + j + 1);
		}
	}

	reverse(indices.begin(), indices.end());

}

