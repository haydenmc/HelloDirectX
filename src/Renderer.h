#pragma once
#include "pch.h"
#include <DirectXMath.h>
#include <memory>

namespace HelloTriangle
{
	class Window;

	class Renderer
	{
	public:
		Renderer(
			Window* window,
			uint32_t width,
			uint32_t height,
			bool useWarpDevice = false
		);

		void Initialize();
		void Render();
		void OnDestroy();

	private:
		static constexpr int NUM_FRAMES = 2;

		struct Vertex
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT4 color;
		};

		Window* const m_window;
		const bool m_useWarpDevice;

		// Viewport dimensions
		uint32_t m_width{ 0 };
		uint32_t m_height{ 0 };
		float m_aspectRatio{ 0.0f };

		// DX Pipeline
		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
		Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, NUM_FRAMES> m_renderTargets;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
		uint32_t m_rtvDescriptorSize{ 0 };

		// Resources
		Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{ 0 };

		// Synchronization
		uint32_t m_frameIndex{ 0 };
		HANDLE m_fenceEvent{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		uint64_t m_fenceValue{ 0 };

		void LoadPipeline();
		void LoadAssets();
		void PopulateCommandList();
		void WaitForPreviousFrame();

		void GetHardwareAdapter(
			IDXGIFactory1* pFactory,
			IDXGIAdapter1** ppAdapter,
			bool requestHighPerformanceAdapter = false);
	};
}