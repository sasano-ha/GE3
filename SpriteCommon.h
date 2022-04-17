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
	/// <param name="device">デバイス</param>
	/// <param name="commandList">コマンドリスト</param>
	/// <param name="windows_width">画面横幅</param>
	/// <param name="window_height">画面縦幅</param>
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, int windows_width, int window_height);

	/// <summary>
	/// 描画前処理
	/// </summary>
	void PreDraw();

	/// <summary>
	/// ルートデスクリプタテーブルの設定
	/// </summary>
	/// <param name="rootParameterIndex">ルートパラメーター番号</param>
	/// <param name="texNumber">テクスチャ番号</param>
	void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, UINT texNumber);

	/// <summary>
	/// テクスチャ読み込み
	/// </summary>
	/// <param name="texNumber"></param>
	/// <param name="filename"></param>
	void LoadTexture(UINT texnumber, const wchar_t* filename);

	/// <summary>
	/// テクスチャ取得
	/// </summary>
	/// <param name="texnumber">テクスチャ番号</param>
	/// <returns>テクスチャリソース</returns>
	ID3D12Resource* GetTexBuff(int texNumber);


	ID3D12Device* GetDevice() { return device_; }

	const DirectX::XMMATRIX& GetMatProjection() { return matProjection_; }

	ID3D12GraphicsCommandList* GetCommandList() { return commandList_; }

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
	//コマンドリスト（借りてくる）
	ID3D12GraphicsCommandList* commandList_ = nullptr;

private:
	// パイプライン生成
	void CreateGraphicsPipeline();
};

