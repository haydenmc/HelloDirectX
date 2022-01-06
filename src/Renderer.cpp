#include "pch.h"
#include "Renderer.h"

namespace HelloTriangle
{
#pragma region Public
	Renderer::Renderer(
		RenderWindow* window,
		bool useWarpDevice
	) : 
		m_window(window),
		m_useWarpDevice(useWarpDevice)
	{ }

	void Renderer::OnInit()
	{
		LoadPipeline();
		LoadAssets();
	}

	void Renderer::OnUpdate()
	{

	}

	void Renderer::OnRender()
	{

	}

	void Renderer::OnDestroy()
	{

	}
#pragma endregion Public

#pragma region Private
	void Renderer::LoadPipeline()
	{
		uint32_t dxgiFactoryFlags{ 0 };

#ifdef _DEBUG
		// Enable the debug layer
		MWRL::ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif

		MWRL::ComPtr<IDXGIFactory4> factory;
		ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

		if (m_useWarpDevice)
		{
			MWRL::ComPtr<IDXGIAdapter> warpAdapter;
			ThrowIfFailed(D3D12CreateDevice(
				warpAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_d3dDevice)
			));
		}
		else
		{
			MWRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
			GetHardwareAdapter(factory.Get(), &hardwareAdapter);
			ThrowIfFailed(D3D12CreateDevice(
				hardwareAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_d3dDevice)
			));
		}

		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.BufferCount = NUM_FRAMES;
		swapChainDesc.Width = m_window->GetWidth();
		swapChainDesc.Height = m_window->GetHeight();
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;
		MWRL::ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(
			m_commandQueue.Get(),
			m_window->GetHwnd(),
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain
		));

		// Disable fullscreen transitions
		ThrowIfFailed(factory->MakeWindowAssociation(m_window->GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swapChain.As(&m_swapChain));
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		// Create descriptor heaps
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
			rtvHeapDesc.NumDescriptors = NUM_FRAMES;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
				&rtvHeapDesc,
				IID_PPV_ARGS(&m_rtvHeap)
			));
			m_rtvDescriptorSize = 
				m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		// Create frame resources
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle
			{
				m_rtvHeap->GetCPUDescriptorHandleForHeapStart()
			};
			for (uint32_t n = 0; n < NUM_FRAMES; ++n)
			{
				ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
				m_d3dDevice->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(1, m_rtvDescriptorSize);
			}
		}

		ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_commandAllocator)
		));
	}

	void Renderer::LoadAssets()
	{

	}

	void Renderer::PopulateCommandList()
	{

	}

	void Renderer::WaitForPreviousFrame()
	{

	}
	
	void Renderer::GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter)
	{
		*ppAdapter = nullptr;

		MWRL::ComPtr<IDXGIAdapter1> adapter;

		MWRL::ComPtr<IDXGIFactory6> factory6;
		if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
		{
			for (
				UINT adapterIndex = 0;
				SUCCEEDED(factory6->EnumAdapterByGpuPreference(
					adapterIndex,
					requestHighPerformanceAdapter == true ? 
						DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
					IID_PPV_ARGS(&adapter)));
				++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(
					adapter.Get(),
					D3D_FEATURE_LEVEL_11_0,
					_uuidof(ID3D12Device),
					nullptr)))
				{
					break;
				}
			}
		}

		if (adapter.Get() == nullptr)
		{
			for (
				UINT adapterIndex = 0;
				SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter));
				++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(
					adapter.Get(),
					D3D_FEATURE_LEVEL_11_0,
					_uuidof(ID3D12Device),
					nullptr)))
				{
					break;
				}
			}
		}

		*ppAdapter = adapter.Detach();
	}
#pragma endregion Private
}