#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "WinApp.h"

//DirectX基盤
class DirectXCommon
{

public:
	//namespace省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:	//メンバ関数
	//初期化
	void Initialize(WinApp* win);
	//描画前処理
	void PreDraw();
	//描画後処理
	void PostDraw();
	//デバイスの取得
	ID3D12Device* GetDev() { return dev.Get(); }
	//コマンドリストの取得
	ID3D12GraphicsCommandList* GetCmdList() { return cmdList.Get(); }


private:	//メンバ変数
	//WindowsAPI
	WinApp* winApp = nullptr;

	// 裏表の２つ分について
	std::vector<ComPtr<ID3D12Resource>> backBuffers;

	// フェンスの生成
	UINT64 fenceVal = 0;

	// Direct3D関連
	ComPtr<ID3D12Device> dev;
	ComPtr<IDXGIFactory6> dxgiFactory;
	ComPtr<IDXGISwapChain4> swapchain;
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12DescriptorHeap> rtvHeaps;
	ComPtr<ID3D12Resource> depthBuffer;
	ComPtr<ID3D12Fence> fence;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;

private:	//メンバ関数
	//デバイスの初期化
	void InitializeDevice();

	//コマンド関連の初期化
	void InitializeCommand();

	//スワップチェーンの初期化
	void InitializeSwapchain();

	//レンダーターゲットの初期化
	void InitializeRenderTargetView();

	//深度バッファの初期化
	void InitializeDepthBuffer();

	//フェンスの初期化
	void InitializeFence();
};