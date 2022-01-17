#include "pch.h"
#include "Renderer.h"
#include "Window.h"

namespace HelloTriangle
{
#pragma region Public
	Renderer::Renderer(
		Window* window,
		uint32_t width,
		uint32_t height,
		bool useWarpDevice
	) : 
		m_window{ window },
		m_width{ width },
		m_height{ height },
		m_aspectRatio{ static_cast<float>(width) / static_cast<float>(height) },
		m_useWarpDevice{ useWarpDevice },
		m_viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) },
		m_scissorRect{ 0, 0, static_cast<long>(width), static_cast<long> (height) }
	{ }

	void Renderer::Initialize()
	{
		LoadPipeline();
		LoadAssets();
	}

	void Renderer::Render()
	{
		// Record all the commands we need to render the scene into the command list.
		PopulateCommandList();

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present the frame.
		ThrowIfFailed(m_swapChain->Present(1, 0));

		WaitForPreviousFrame();
	}

	void Renderer::OnDestroy()
	{
		// Ensure that the GPU is no longer referencing resources that are about to be
		// cleaned up by the destructor.
		WaitForPreviousFrame();

		CloseHandle(m_fenceEvent);
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
		swapChainDesc.Width = m_width;
		swapChainDesc.Height = m_height;
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
		// Create an empty root signature
		// "describes the parameters that are passed to the various programmable shader stages
		// of the rendering pipeline."
		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(
				0,
				nullptr,
				0,
				nullptr,
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			);

			Microsoft::WRL::ComPtr<ID3DBlob> signature;
			Microsoft::WRL::ComPtr<ID3DBlob> error;

			ThrowIfFailed(D3D12SerializeRootSignature(
				&rootSignatureDesc,
				D3D_ROOT_SIGNATURE_VERSION_1,
				&signature,
				&error
			));
			ThrowIfFailed(m_d3dDevice->CreateRootSignature(
				0,
				signature->GetBufferPointer(),
				signature->GetBufferSize(),
				IID_PPV_ARGS(&m_rootSignature)
			));
		}

		// Create the pipeline state, which includes compiling and loading shaders
		{
			Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
			Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;

			uint32_t compileFlags{ 0 };
#ifdef _DEBUG
			compileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
#endif

			ThrowIfFailed(D3DCompileFromFile(
				L"C:\\Users\\Hayden\\Source\\HelloTriangle\\x64\\Debug\\shaders.hlsl",
				nullptr,
				nullptr,
				"VSMain",
				"vs_5_0",
				compileFlags,
				0,
				&vertexShader,
				nullptr
			));

			ThrowIfFailed(D3DCompileFromFile(
				L"C:\\Users\\Hayden\\Source\\HelloTriangle\\x64\\Debug\\shaders.hlsl",
				nullptr,
				nullptr,
				"PSMain",
				"ps_5_0",
				compileFlags,
				0,
				&pixelShader,
				nullptr
			));

			// Define the vertex input layout.
			std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputElementDescs
			{{
				{
					"POSITION",
					0,
					DXGI_FORMAT_R32G32B32_FLOAT,
					0,
					0,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
					0
				},
				{
					"COLOR",
					0,
					DXGI_FORMAT_R32G32B32A32_FLOAT,
					0,
					12,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
					0
				}
			}};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{ 0 };
			psoDesc.InputLayout =
			{
				inputElementDescs.data(),
				static_cast<uint32_t>(inputElementDescs.size())
			};
			psoDesc.pRootSignature = m_rootSignature.Get();
			psoDesc.VS = CD3DX12_SHADER_BYTECODE{ vertexShader.Get() };
			psoDesc.PS = CD3DX12_SHADER_BYTECODE{ pixelShader.Get() };
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC{ D3D12_DEFAULT };
			psoDesc.BlendState = CD3DX12_BLEND_DESC{ D3D12_DEFAULT };
			psoDesc.DepthStencilState.DepthEnable = false;
			psoDesc.DepthStencilState.StencilEnable = false;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(
				&psoDesc,
				IID_PPV_ARGS(&m_pipelineState)
			));
		}

		// Create the command list
		ThrowIfFailed(m_d3dDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_commandAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&m_commandList)
		));

		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ThrowIfFailed(m_commandList->Close());

		// Create the vertex buffer
		{
			// Define the geometry for a triangle
			std::array<Vertex, 3> triangleVertices
			{ {
				{ { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
				{ { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
				{ { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
			} };
			const uint32_t vertexBufferSize = 
				static_cast<uint32_t>(triangleVertices.size() * sizeof(Vertex));

			// Note: using upload heaps to transfer static data like vert buffers is not 
			// recommended. Every time the GPU needs it, the upload heap will be marshalled 
			// over. Please read up on Default Heap usage. An upload heap is used here for 
			// code simplicity and because there are very few verts to actually transfer.
			CD3DX12_HEAP_PROPERTIES vertexHeap{ D3D12_HEAP_TYPE_UPLOAD };
			CD3DX12_RESOURCE_DESC bufferResource{ CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize) };
			ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
				&vertexHeap,
				D3D12_HEAP_FLAG_NONE,
				&bufferResource,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_vertexBuffer)
			));

			// Copy triangle data to vertex buffer
			uint8_t* vertexDataBegin;
			CD3DX12_RANGE readRange{ 0, 0 }; // No intention to read on CPU
			ThrowIfFailed(m_vertexBuffer->Map(
				0,
				&readRange,
				reinterpret_cast<void**>(&vertexDataBegin)
			));
			memcpy(
				vertexDataBegin,
				triangleVertices.data(),
				(triangleVertices.size() * sizeof(Vertex))
			);
			m_vertexBuffer->Unmap(0, nullptr);

			// Initialize vertex buffer view
			m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof(Vertex);
			m_vertexBufferView.SizeInBytes = vertexBufferSize;
		}

		// Create synchronization objects and wait until assets are uploaded to GPU
		{
			ThrowIfFailed(m_d3dDevice->CreateFence(
				0,
				D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(&m_fence)
			));
			m_fenceValue = 1;

			// Create an event handle for frame synchronization
			m_fenceEvent = CreateEventW(nullptr, false, false, nullptr);
			if (m_fenceEvent == nullptr)
			{
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}

			WaitForPreviousFrame();
		}

	}

	void Renderer::PopulateCommandList()
	{
		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(m_commandAllocator->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

		// Set necessary state
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		// Indicate that the back buffer will be used as a render target.
		CD3DX12_RESOURCE_BARRIER renderTransition{
			CD3DX12_RESOURCE_BARRIER::Transition(
				m_renderTargets[m_frameIndex].Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			)
		};
		m_commandList->ResourceBarrier(
			1,
			&renderTransition
		);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{
			m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			static_cast<int32_t>(m_frameIndex),
			m_rtvDescriptorSize
		};
		m_commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// Record commands.
		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_commandList->DrawInstanced(3, 1, 0, 0);

		// Indicate that the back buffer will now be used to present.
		CD3DX12_RESOURCE_BARRIER presentTransition
		{
			CD3DX12_RESOURCE_BARRIER::Transition(
				m_renderTargets[m_frameIndex].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT
			)
		};
		m_commandList->ResourceBarrier(
			1,
			&presentTransition
		);

		ThrowIfFailed(m_commandList->Close());
	}

	void Renderer::WaitForPreviousFrame()
	{
		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
		// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
		// sample illustrates how to use fences for efficient resource usage and to
		// maximize GPU utilization.

		// Signal and increment the fence value.
		const UINT64 fence = m_fenceValue;
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
		m_fenceValue++;

		// Wait until the previous frame is finished.
		if (m_fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
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