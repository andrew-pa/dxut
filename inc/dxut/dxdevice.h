#pragma once
#include "dxut\cmmn.h"
#include "DXWindow.h"

#define SOIL
#ifdef SOIL
#include "SOIL.h"
#endif

struct descriptor_heap {
	ID3D12DescriptorHeap* heap;
	D3D12_CPU_DESCRIPTOR_HANDLE cph;
	D3D12_GPU_DESCRIPTOR_HANDLE gph;
	uint32_t handle_incr;

	descriptor_heap() : heap(nullptr) {

	}

	descriptor_heap(ComPtr<ID3D12Device> device, uint32_t num_descriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible,
			const wchar_t* name = nullptr) {
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = num_descriptors;
		desc.Type = type;
		desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		chk(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
		handle_incr = device->GetDescriptorHandleIncrementSize(type);
		if (name) heap->SetName(name);
		cph = heap->GetCPUDescriptorHandleForHeapStart();
		gph = heap->GetGPUDescriptorHandleForHeapStart();
	}

	descriptor_heap(const descriptor_heap& h) 
		: heap(h.heap), cph(h.cph), gph(h.gph), handle_incr(h.handle_incr)
	{
		heap->AddRef();
	}

	const descriptor_heap& operator =(const descriptor_heap& h) {
		heap = h.heap;
		cph = h.cph;
		gph = h.gph;
		handle_incr = h.handle_incr;
		heap->AddRef();
		return *this;
	}

	inline D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle(int i = 0) const {
		return { cph.ptr + i * handle_incr };
	}
	inline D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle(int i = 0) const {
		return { gph.ptr + i * handle_incr };
	}

	~descriptor_heap() {
		if (heap == nullptr) return;
		auto v = heap->Release();
		heap = nullptr;
	}
};

struct root_parameterh {
	inline static CD3DX12_ROOT_PARAMETER constants(UINT num32BitValues,
		UINT shaderRegister,
		UINT registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		CD3DX12_ROOT_PARAMETER rp{};
		rp.InitAsConstants(num32BitValues, shaderRegister, registerSpace, visibility);
		return rp;
	}

	inline static CD3DX12_ROOT_PARAMETER descriptor_table(vector<CD3DX12_DESCRIPTOR_RANGE> descr,
		D3D12_SHADER_VISIBILITY visib = D3D12_SHADER_VISIBILITY_ALL)
	{
		CD3DX12_ROOT_PARAMETER rp{};
		D3D12_DESCRIPTOR_RANGE* dsrd = new D3D12_DESCRIPTOR_RANGE[descr.size()]; //this gets leaked
		memcpy(dsrd, descr.data(), sizeof(D3D12_DESCRIPTOR_RANGE)*descr.size());
		rp.InitAsDescriptorTable(descr.size(), dsrd, visib);
		return rp;
	}

	inline static CD3DX12_ROOT_PARAMETER descriptor_table(D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors,
		UINT baseShaderRegister,
		UINT registerSpace = 0,
		UINT offsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
		D3D12_SHADER_VISIBILITY visib = D3D12_SHADER_VISIBILITY_ALL)
	{
		return descriptor_table({
			descriptor_range(rangeType, numDescriptors, baseShaderRegister, registerSpace,
			offsetInDescriptorsFromTableStart)
		}, visib);
		/*CD3DX12_ROOT_PARAMETER rp{};
		CD3DX12_DESCRIPTOR_RANGE* dr = new CD3DX12_DESCRIPTOR_RANGE;
		dr->Init(rangeType, numDescriptors, baseShaderRegister, registerSpace, offsetInDescriptorsFromTableStart);
		rp.InitAsDescriptorTable(1, dr, visib);
		return rp;*/
	}

	inline static CD3DX12_DESCRIPTOR_RANGE descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors,
		UINT baseShaderRegister,
		UINT registerSpace = 0,
		UINT offsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		CD3DX12_DESCRIPTOR_RANGE dr{};
		dr.Init(rangeType, numDescriptors, baseShaderRegister, registerSpace, offsetInDescriptorsFromTableStart);
		return dr;
	}
};

const float color_black[] = { 0.f,0.f,0.f,0.f };

class DXDevice {
public:
	static const UINT FrameCount = 3;
	D3D12_VIEWPORT viewport;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12Resource> renderTargets[FrameCount];
	unique_ptr<descriptor_heap> rtvHeap, dsvHeap;
	ComPtr<ID3D12Resource> depthStencil;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	D3D12_RECT scissorRect;

	UINT frameIndex;
	UINT frameCounter;
	HANDLE fenceEvent;
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue, currentFenceValue;

	DXDevice() : frameIndex(0), viewport(), scissorRect(), frameCounter(0), fenceValue(0), currentFenceValue(0) {}


	void init_d3d(DXWindow* win, uint32_t msaa_lvl = 1, bool readableDepth = false) {
		#ifdef _DEBUG
				// Enable the D3D12 debug layer.
			{
				ComPtr<ID3D12Debug> debugController;
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
				{
					debugController->EnableDebugLayer();
				}
			}
		#endif

			ComPtr<IDXGIFactory4> factory;
			chk(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

			if (win->useWarpDevice)
			{
				ComPtr<IDXGIAdapter> warpAdapter;
				ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

				ThrowIfFailed(D3D12CreateDevice(
					warpAdapter.Get(),
					D3D_FEATURE_LEVEL_11_0,
					IID_PPV_ARGS(&device)
					));
			}
			else
			{
				ThrowIfFailed(D3D12CreateDevice(
					nullptr,
					D3D_FEATURE_LEVEL_11_0,
					IID_PPV_ARGS(&device)
					));
			}

			// Describe and create the command queue.
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

			// Describe and create the swap chain.
			DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
			swapChainDesc.BufferCount = FrameCount;
			swapChainDesc.BufferDesc.Width = win->width;
			swapChainDesc.BufferDesc.Height = win->height;
			swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.OutputWindow = win->hwnd;
			swapChainDesc.SampleDesc.Count = msaa_lvl;
			swapChainDesc.SampleDesc.Quality = msaa_lvl > 1 ? 1 : 0;
			swapChainDesc.Windowed = TRUE;

			ComPtr<IDXGISwapChain> xswapChain;
			ThrowIfFailed(factory->CreateSwapChain(
				commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
				&swapChainDesc,
				&xswapChain
				));

			ThrowIfFailed(xswapChain.As(&swapChain));

			frameIndex = swapChain->GetCurrentBackBufferIndex();

			rtvHeap = make_unique<descriptor_heap>(device, FrameCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
			dsvHeap = make_unique<descriptor_heap>(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
				

			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
			depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE; 

			D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
			depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
			depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
			depthOptimizedClearValue.DepthStencil.Stencil = 0;

			ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					readableDepth ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_D32_FLOAT,
					win->width, win->height, 1, 0, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				readableDepth ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&depthStencil)
				));

			device->CreateDepthStencilView(depthStencil.Get(), &depthStencilDesc, dsvHeap->cpu_handle(0));

			for (UINT i = 0; i < FrameCount; i++)
			{
				chk(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
				device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHeap->cpu_handle(i));
			}

			viewport.Width = win->width;
			viewport.Height = win->height;
			viewport.MaxDepth = 1.f;
			scissorRect.right = win->width;
			scissorRect.bottom = win->height;

			chk(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
	}
	void destroy_d3d() {
		const UINT64 fencev = fenceValue;
		const UINT64 lcomf = fence->GetCompletedValue();
		chk(commandQueue->Signal(fence.Get(), fenceValue));
		fenceValue++;
		if (lcomf < fencev) {
			chk(fence->SetEventOnCompletion(fencev, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		empty_upload_pool();
		free_shaders();
		device.Reset();
		swapChain.Reset();
		commandQueue.Reset();
		commandAllocator.Reset();
		commandList.Reset();
		dsvHeap.reset();
		rtvHeap.reset();
		fence.Reset();
		for (int i = 0; i < FrameCount; ++i)
			renderTargets[i].Reset();
	}

	void start_frame() {
		wait_for_gpu(); 
		frameCounter++;
	}

	void next_frame() {
		chk(swapChain->Present(1, 0));
		frameIndex = swapChain->GetCurrentBackBufferIndex();
		signal_queue();
	}

	void signal_queue() {
		currentFenceValue = fenceValue;
		chk(commandQueue->Signal(fence.Get(), fenceValue));
		fenceValue++;
	}

	void wait_for_gpu() {
		if (!fence) {
			chk(device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
			fenceValue++;
			fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
			if (!fenceEvent) chk(HRESULT_FROM_WIN32(GetLastError()));
			const uint64_t fence_w = fenceValue;
			chk(commandQueue->Signal(fence.Get(), fence_w));
			fenceValue++;
			chk(fence->SetEventOnCompletion(fence_w, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		const uint64_t last_comp_fence = fence->GetCompletedValue();

		if (currentFenceValue != 0 && currentFenceValue > last_comp_fence) {
			chk(fence->SetEventOnCompletion(currentFenceValue, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}
	}

	ComPtr<ID3D12GraphicsCommandList> create_command_list(D3D12_COMMAND_LIST_TYPE ty, ComPtr<ID3D12PipelineState> pips = nullptr, ComPtr<ID3D12CommandAllocator> callc = nullptr, UINT nm = 0) {
		ComPtr<ID3D12GraphicsCommandList> cl;
		chk(device->CreateCommandList(nm, ty, callc==nullptr ? commandAllocator.Get() : callc.Get(), pips.Get(), IID_PPV_ARGS(&cl)));
		return cl;
	}

	inline void set_default_viewport(ComPtr<ID3D12GraphicsCommandList> cmdlist) {
		cmdlist->RSSetViewports(1, &viewport);
		cmdlist->RSSetScissorRects(1, &scissorRect);
	}

	inline void start_render_to_backbuffer(ComPtr<ID3D12GraphicsCommandList> cmdlist, bool clearR = true, bool clearD = true) {
		set_default_viewport(cmdlist);

		cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		cmdlist->OMSetRenderTargets(1, &rtvHeap->cpu_handle(frameIndex), false,
			&dsvHeap->cpu_handle());

		if(clearR) cmdlist->ClearRenderTargetView(rtvHeap->cpu_handle(frameIndex), color_black, 0, nullptr);
		if(clearD) cmdlist->ClearDepthStencilView(dsvHeap->cpu_handle(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	}

	inline void finish_render_to_backbuffer(ComPtr<ID3D12GraphicsCommandList> cmdlist) {
		cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	}

	inline void resource_barrier(ComPtr<ID3D12GraphicsCommandList> cmdlist,
		const vector<CD3DX12_RESOURCE_BARRIER>& tr) 
	{
		cmdlist->ResourceBarrier(tr.size(), tr.data());
	}

	void create_depth_stencil(uint32_t width, uint32_t height, D3D12_CPU_DESCRIPTOR_HANDLE hndl, ComPtr<ID3D12Resource>& res, DXGI_FORMAT fmt = DXGI_FORMAT_D32_FLOAT) {
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = fmt;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = fmt;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(fmt, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&res)
			));

		device->CreateDepthStencilView(res.Get(), &depthStencilDesc, 
			hndl);
	}


	template <typename Tcb, size_t Tcb_s = sizeof(Tcb)>
	void create_constant_buffer(D3D12_HEAP_PROPERTIES* uplhepprop, descriptor_heap& h,
		ComPtr<ID3D12Resource>& cbr, Tcb** cb, uint32_t idx, size_t num_bufs = 1, D3D12_RESOURCE_FLAGS additionalFlags = D3D12_RESOURCE_FLAG_NONE) {
		CD3DX12_RESOURCE_DESC cbd = CD3DX12_RESOURCE_DESC::Buffer(Tcb_s * num_bufs, additionalFlags);
		chk(device->CreateCommittedResource(uplhepprop,
			D3D12_HEAP_FLAG_NONE,
			&cbd,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&cbr)));

		for (size_t i = 0; i < num_bufs; ++i) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
			desc.BufferLocation = cbr->GetGPUVirtualAddress() + i*Tcb_s;
			desc.SizeInBytes = Tcb_s;
			device->CreateConstantBufferView(&desc, h.cpu_handle(idx+i));
		}

		chk(cbr->Map(0, nullptr, (void**)cb));
		ZeroMemory(*cb, Tcb_s*num_bufs);
	}


	void create_root_signature(vector<CD3DX12_ROOT_PARAMETER> paras,
		vector<CD3DX12_STATIC_SAMPLER_DESC> static_samps,
		ComPtr<ID3D12RootSignature>& rs, bool free_paras_ptrs = false,
		const wchar_t* name = nullptr,
		D3D12_ROOT_SIGNATURE_FLAGS rsf = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
	{
		D3D12_ROOT_SIGNATURE_DESC rsd;
		rsd.pParameters = paras.data();
		rsd.NumParameters = paras.size();
		rsd.pStaticSamplers = (static_samps.size() > 0 ? static_samps.data() : nullptr);
		rsd.NumStaticSamplers = static_samps.size();
		rsd.Flags = rsf;
		ComPtr<ID3DBlob> sig, err;
		chk(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err));

		chk(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&rs)));

		for (auto& p : paras) {
			if(p.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && p.DescriptorTable.pDescriptorRanges) delete p.DescriptorTable.pDescriptorRanges;
		}

		if (name) rs->SetName(name);
	}

	void execute_command_lists(const vector<ComPtr<ID3D12GraphicsCommandList>>& cmdlsts) {
		ID3D12CommandList** cmdlists = new ID3D12CommandList*[cmdlsts.size()];
		int i = 0;
		for (auto cl : cmdlsts) cmdlists[i++] = cl.Get();
		commandQueue->ExecuteCommandLists(cmdlsts.size(), cmdlists);
	}
	void execute_command_list(ComPtr<ID3D12GraphicsCommandList> cmdl = nullptr) {
		if (cmdl == nullptr) cmdl = commandList;
		ID3D12CommandList* cmdlists[] = { cmdl.Get() };
		commandQueue->ExecuteCommandLists(_countof(cmdlists), cmdlists);
	}

	map<wstring, D3D12_SHADER_BYTECODE> loaded_shaders;

	D3D12_SHADER_BYTECODE load_shader(wstring path) {
		auto als = loaded_shaders.find(path);
		if (als != loaded_shaders.end()) 
			return als->second;
		else {
			uint8_t* data;
			uint32_t len;
			chk(ReadDataFromFile(path.c_str(), &data, &len));
			return loaded_shaders[path] = { data,len };
		}
	}

	void free_shaders() {
		for (auto& sbc : loaded_shaders) {
			if (sbc.second.pShaderBytecode) {
				delete sbc.second.pShaderBytecode;
				sbc.second.pShaderBytecode = nullptr;
			}
		}
	}


	vector<ComPtr<ID3D12Resource>> upload_pool;
	ComPtr<ID3D12Resource> new_upload_resource(D3D12_HEAP_PROPERTIES* heapprop,
		D3D12_HEAP_FLAGS heap_flags, D3D12_RESOURCE_DESC* desc, 
		D3D12_RESOURCE_STATES init_state, D3D12_CLEAR_VALUE* init_clear_value = nullptr) 
	{
		ComPtr<ID3D12Resource> r;
		chk(device->CreateCommittedResource(heapprop, heap_flags, desc, init_state,
			init_clear_value, IID_PPV_ARGS(&r)));
		upload_pool.push_back(r);
		return r;
	}
	void empty_upload_pool() {
		for (auto& u : upload_pool)
			if (u != nullptr) u.Reset();
		upload_pool.clear();
	}

#ifdef SOIL
	map<string, ComPtr<ID3D12Resource>> texture_cashe;
	inline void load_texture(ComPtr<ID3D12GraphicsCommandList> cmdlist,
		const string& path, ComPtr<ID3D12Resource>& tex) {
		auto ctx = texture_cashe.find(path);
		if (ctx != texture_cashe.end()) {
			tex = ctx->second;
			return;
		}

		byte* data; uint32_t size;
		ReadDataFromFile(s2ws(path).c_str(), &data, &size);

		D3D12_RESOURCE_DESC rsd = {}; int c;
		D3D12_SUBRESOURCE_DATA txd = {};
		txd.pData = SOIL_load_image_from_memory(data, size, (int*)&rsd.Width, (int*)&rsd.Height, &c, SOIL_LOAD_RGBA);
		

		rsd.MipLevels = 1;
		/*switch (c) {
		case 1: rsd.Format = DXGI_FORMAT_R8_UINT; break;
		case 2: rsd.Format = DXGI_FORMAT_R8G8_UINT; break;
		case 3: case 4: rsd.Format = DXGI_FORMAT_R8G8B8A8_UINT; break;
		}*/
		rsd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rsd.Flags = D3D12_RESOURCE_FLAG_NONE;
		rsd.DepthOrArraySize = 1;
		rsd.SampleDesc.Count = 1;
		rsd.SampleDesc.Quality = 1;
		rsd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		chk(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rsd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_PPV_ARGS(&tex)));
		const uint32_t subres_cnt = 1;
		const uint64_t uplbuf_siz = GetRequiredIntermediateSize(tex.Get(), 0, subres_cnt);

		auto txupl = new_upload_resource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uplbuf_siz),
			D3D12_RESOURCE_STATE_GENERIC_READ);

		txd.RowPitch = rsd.Width * c;
		txd.SlicePitch = rsd.Width * rsd.Height * c;

		UpdateSubresources(cmdlist.Get(), tex.Get(), txupl.Get(), 0, 0, 1, &txd);
		cmdlist->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(tex.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		texture_cashe[path] = tex;
	}
#endif

};

struct pass {
	ComPtr<ID3D12RootSignature> root_sig;
	ComPtr<ID3D12PipelineState> pipeline;
	bool is_compute;

	pass() : root_sig(nullptr), pipeline(nullptr), is_compute(false) {}
	pass(ComPtr<ID3D12RootSignature> rs, ComPtr<ID3D12PipelineState> ps, bool compute = false)
		: root_sig(rs), pipeline(ps), is_compute(compute) { }

	pass(DXDevice* dv,
		vector<CD3DX12_ROOT_PARAMETER> rs_params, vector<CD3DX12_STATIC_SAMPLER_DESC> stat_smps,
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pdsc,
		wstring name = wstring(),
		D3D12_ROOT_SIGNATURE_FLAGS rsf = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
		: is_compute(false)
	{
		dv->create_root_signature(rs_params, stat_smps, root_sig, true,
			(name + wstring(L" Root Signature")).c_str(), rsf);
		pdsc.pRootSignature = root_sig.Get();
		chk(dv->device->CreateGraphicsPipelineState(&pdsc, IID_PPV_ARGS(&pipeline)));
		pipeline->SetName((name + wstring(L" Pipeline")).c_str());
	}

	pass(DXDevice* dv,
		vector<CD3DX12_ROOT_PARAMETER> rs_params, vector<CD3DX12_STATIC_SAMPLER_DESC> stat_smps,
		D3D12_COMPUTE_PIPELINE_STATE_DESC pdsc,
		wstring name = wstring(),
		D3D12_ROOT_SIGNATURE_FLAGS rsf = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		: is_compute(true)
	{
		dv->create_root_signature(rs_params, stat_smps, root_sig, true,
			(name + wstring(L" Root Signature")).c_str(), rsf);
		pdsc.pRootSignature = root_sig.Get();
		chk(dv->device->CreateComputePipelineState(&pdsc, IID_PPV_ARGS(&pipeline)));
		pipeline->SetName((name + wstring(L" Pipeline")).c_str());
	}

	pass(DXDevice* dv, ComPtr<ID3D12RootSignature> exisitingRS,
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pdsc,
		wstring name = wstring()) :
		root_sig(exisitingRS), is_compute(false)
	{
		pdsc.pRootSignature = exisitingRS.Get();
		chk(dv->device->CreateGraphicsPipelineState(&pdsc, IID_PPV_ARGS(&pipeline)));
		pipeline->SetName((name + wstring(L" Pipeline")).c_str());
	}

	void apply(ComPtr<ID3D12GraphicsCommandList> cmdlist) {
		cmdlist->SetPipelineState(pipeline.Get());
		if (is_compute)
			cmdlist->SetComputeRootSignature(root_sig.Get());
		else cmdlist->SetGraphicsRootSignature(root_sig.Get());
	}
};

struct DXCompute {
	DXDevice* dv;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12GraphicsCommandList> commandList;

	ComPtr<ID3D12Fence> fence;

	DXCompute() {}

	void init(DXDevice* __dv) {
		dv = __dv;
		dv->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
			IID_PPV_ARGS(&commandAllocator));
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

		chk(dv->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

		commandList = dv->create_command_list(D3D12_COMMAND_LIST_TYPE_COMPUTE, nullptr,
			commandAllocator);
		commandList->Close();

		chk(dv->device->CreateFence(
			dv->fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	}

	void destroy() {
		wait_for_gpu();
		dv = nullptr;
		commandQueue.Reset();
		commandAllocator.Reset();
		commandList.Reset();
		fence.Reset();
	}

	void reset(ComPtr<ID3D12PipelineState> ps = nullptr) {
		chk(commandAllocator->Reset());
		chk(commandList->Reset(commandAllocator.Get(), ps.Get()));
	}
	void reset(const pass& p) {
		chk(commandAllocator->Reset());
		chk(commandList->Reset(commandAllocator.Get(), p.pipeline.Get()));
		commandList->SetComputeRootSignature(p.root_sig.Get());
	}

	void execute(ComPtr<ID3D12CommandList> cmdl = nullptr) {
		if(cmdl == nullptr) cmdl = commandList;
		ID3D12CommandList* cmdlists[] = { cmdl.Get() };
		commandQueue->ExecuteCommandLists(_countof(cmdlists), cmdlists);
		chk(commandQueue->Signal(fence.Get(), dv->fenceValue));
	}

	void wait_for_gpu() {
		chk(commandQueue->Wait(fence.Get(), dv->fenceValue));
	}
};

