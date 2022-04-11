#pragma once

#include "PipelineSet.h"

#include <DirectXTex.h>

/// <summary>
/// スプライト共通部分
/// </summary>

class SpriteCommon
{
public:	//プロジェクト全体で使える

	// テクスチャの最大枚数
	static const int kSpriteSRVCount = 512;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(ID3D12Device* device);

private:	//宣言のみで使える
	// パイプラインセット
	PipelineSet pipelineSet_;
	// 射影行列
	DirectX::XMMATRIX matProjection_{};
	// テクスチャ用デスクリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descHeap_;
	// テクスチャリソース（テクスチャバッファ）の配列
	Microsoft::WRL::ComPtr<ID3D12Resource> texBuff_[kSpriteSRVCount];
	//デバイス（借りてくる）
	ID3D12Device* device_ = nullptr;
private:
	// パイプライン生成
	void CreateGraphicsPipeline();
};

