#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include <d3dx12.h>

#include <vector>
#include <string>
#include <fstream>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#define DIRECTINPUT_VERSION     0x0800   // DirectInputのバージョン指定
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include <DirectXTex.h>
#include <wrl.h>

#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")

#include "input.h"
#include "WinApp.h"

using namespace DirectX;
using namespace Microsoft::WRL;

LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // メッセージで分岐
    switch (msg) {
        case WM_DESTROY: // ウィンドウが破棄された
            PostQuitMessage(0); // OSに対して、アプリの終了を伝える
            return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam); // 標準の処理を行う
}

// 頂点データ
struct VertexPosUv
{
    XMFLOAT3 pos; // xyz座標
    XMFLOAT2 uv;  // uv座標
};

// 定数バッファ用データ構造体
struct ConstBufferData {
    XMFLOAT4 color; // 色 (RGBA)
    XMMATRIX mat;   // ３Ｄ変換行列
};

// パイプラインセット
struct PipelineSet
{
    // パイプラインステートオブジェクト
    ComPtr<ID3D12PipelineState> pipelinestate;
    // ルートシグネチャ
    ComPtr<ID3D12RootSignature> rootsignature;
};

// テクスチャの最大枚数
const int spriteSRVCount = 512;

// スプライトの共通データ
struct SpriteCommon
{
    // パイプラインセット
    PipelineSet pipelineSet;
    // 射影行列
    XMMATRIX matProjection{};
    // テクスチャ用デスクリプタヒープの生成
    ComPtr<ID3D12DescriptorHeap> descHeap;
    // テクスチャリソース（テクスチャバッファ）の配列
    ComPtr<ID3D12Resource> texBuff[spriteSRVCount];
};

// スプライト1枚分のデータ
struct Sprite
{
    //頂点バッファ;
    ComPtr<ID3D12Resource> vertBuff;
    //頂点バッファビュー;
    D3D12_VERTEX_BUFFER_VIEW vbView{};
    //定数バッファ;
    ComPtr<ID3D12Resource> constBuff;
    // Z軸回りの回転角
    float rotation = 0.0f;
    // 座標
    XMFLOAT3 position = { 0,0,0 };
    // ワールド行列
    XMMATRIX matWorld;
    // 色(RGBA)
    XMFLOAT4 color = { 1, 1, 1, 1 };
    // テクスチャ番号
    UINT texNumber = 0;
    // 大きさ
    XMFLOAT2 size = { 100, 100 };
    // アンカーポイント
    XMFLOAT2 anchorpoint = { 0.5f, 0.5f };
    // 左右反転
    bool isFlipX = false;
    // 上下反転
    bool isFlipY = false;
    // テクスチャ左上座標
    XMFLOAT2 texLeftTop = { 0, 0 };
    // テクスチャ切り出しサイズ
    XMFLOAT2 texSize = { 100, 100 };
    // 非表示
    bool isInvisible = false;
};

// 3Dオブジェクト用パイプライン生成
PipelineSet Object3dCreateGraphicsPipeline(ID3D12Device* dev)
{
    HRESULT result;

    ComPtr<ID3DBlob> vsBlob = nullptr; // 頂点シェーダオブジェクト
    ComPtr<ID3DBlob> psBlob = nullptr; // ピクセルシェーダオブジェクト
    ComPtr<ID3DBlob> errorBlob = nullptr; // エラーオブジェクト

    // 頂点シェーダの読み込みとコンパイル
    result = D3DCompileFromFile(
        L"Resources/shaders/BasicVS.hlsl",  // シェーダファイル名
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
        "main", "vs_5_0", // エントリーポイント名、シェーダーモデル指定
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
        0,
        &vsBlob, &errorBlob);

    if (FAILED(result)) {
        // errorBlobからエラー内容をstring型にコピー
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // エラー内容を出力ウィンドウに表示
        OutputDebugStringA(errstr.c_str());
        exit(1);
    }

    // ピクセルシェーダの読み込みとコンパイル
    result = D3DCompileFromFile(
        L"Resources/shaders/BasicPS.hlsl",   // シェーダファイル名
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
        "main", "ps_5_0", // エントリーポイント名、シェーダーモデル指定
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
        0,
        &psBlob, &errorBlob);

    if (FAILED(result)) {
        // errorBlobからエラー内容をstring型にコピー
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // エラー内容を出力ウィンドウに表示
        OutputDebugStringA(errstr.c_str());
        exit(1);
    }

    // 頂点レイアウト
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // グラフィックスパイプライン設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

    gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

    gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定

    gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    //gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // カリングしない

    // レンダーターゲットのブレンド設定
    D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = gpipeline.BlendState.RenderTarget[0];
    blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // 標準設定
    blenddesc.BlendEnable = true;                   // ブレンドを有効にする
    blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;    // 加算
    blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;      // ソースの値を100% 使う
    blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;    // デストの値を   0% 使う
    blenddesc.BlendOp = D3D12_BLEND_OP_ADD;             // 加算
    blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;         // ソースのアルファ値
    blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;    // 1.0f-ソースのアルファ値

    // デプスステンシルステートの設定
    gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 深度値フォーマット

    gpipeline.InputLayout.pInputElementDescs = inputLayout;
    gpipeline.InputLayout.NumElements = _countof(inputLayout);

    gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    gpipeline.NumRenderTargets = 1; // 描画対象は1つ
    gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0～255指定のRGBA
    gpipeline.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング

    // デスクリプタテーブルの設定
    CD3DX12_DESCRIPTOR_RANGE descRangeSRV;
    descRangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 レジスタ

    // ルートパラメータの設定
    CD3DX12_ROOT_PARAMETER rootparams[2];
    rootparams[0].InitAsConstantBufferView(0);
    rootparams[1].InitAsDescriptorTable(1, &descRangeSRV);

    // スタティックサンプラー
    CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

    // パイプラインとルートシグネチャのセット
    PipelineSet pipelineSet;

    // ルートシグネチャの生成
    // ルートシグネチャの設定
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSigBlob;
    // バージョン自動判定でのシリアライズ
    result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
    // ルートシグネチャの生成
    result = dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&pipelineSet.rootsignature));

    // パイプラインにルートシグネチャをセット
    gpipeline.pRootSignature = pipelineSet.rootsignature.Get();
    
    result = dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelineSet.pipelinestate));

    // パイプラインとルートシグネチャを返す
    return pipelineSet;
}

// スプライト用パイプライン生成
PipelineSet SpriteCreateGraphicsPipeline(ID3D12Device* dev)
{
    HRESULT result;

    ComPtr<ID3DBlob> vsBlob = nullptr; // 頂点シェーダオブジェクト
    ComPtr<ID3DBlob> psBlob = nullptr; // ピクセルシェーダオブジェクト
    ComPtr<ID3DBlob> errorBlob = nullptr; // エラーオブジェクト

    // 頂点シェーダの読み込みとコンパイル
    result = D3DCompileFromFile(
        L"Resources/shaders/SpriteVS.hlsl",  // シェーダファイル名
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
        "main", "vs_5_0", // エントリーポイント名、シェーダーモデル指定
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
        0,
        &vsBlob, &errorBlob);

    if (FAILED(result)) {
        // errorBlobからエラー内容をstring型にコピー
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // エラー内容を出力ウィンドウに表示
        OutputDebugStringA(errstr.c_str());
        exit(1);
    }

    // ピクセルシェーダの読み込みとコンパイル
    result = D3DCompileFromFile(
        L"Resources/shaders/SpritePS.hlsl",   // シェーダファイル名
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
        "main", "ps_5_0", // エントリーポイント名、シェーダーモデル指定
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
        0,
        &psBlob, &errorBlob);

    if (FAILED(result)) {
        // errorBlobからエラー内容をstring型にコピー
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // エラー内容を出力ウィンドウに表示
        OutputDebugStringA(errstr.c_str());
        exit(1);
    }

    // 頂点レイアウト
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // グラフィックスパイプライン設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

    gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

    gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定

    gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;              // 背面カリングをしない

    // レンダーターゲットのブレンド設定
    D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = gpipeline.BlendState.RenderTarget[0];
    blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // 標準設定
    blenddesc.BlendEnable = true;                   // ブレンドを有効にする
    blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;    // 加算
    blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;      // ソースの値を100% 使う
    blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;    // デストの値を   0% 使う
    blenddesc.BlendOp = D3D12_BLEND_OP_ADD;             // 加算
    blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;         // ソースのアルファ値
    blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;    // 1.0f-ソースのアルファ値

    // デプスステンシルステートの設定
    gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    gpipeline.DepthStencilState.DepthEnable = false;    // 深度テストをしない
    //gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;       // 常に上書きルール
    gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 深度値フォーマット

    gpipeline.InputLayout.pInputElementDescs = inputLayout;
    gpipeline.InputLayout.NumElements = _countof(inputLayout);

    gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    gpipeline.NumRenderTargets = 1; // 描画対象は1つ
    gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0～255指定のRGBA
    gpipeline.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング

    // デスクリプタテーブルの設定
    CD3DX12_DESCRIPTOR_RANGE descRangeSRV;
    descRangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 レジスタ

    // ルートパラメータの設定
    CD3DX12_ROOT_PARAMETER rootparams[2];
    rootparams[0].InitAsConstantBufferView(0); // 定数バッファビューとして初期化(b0レジスタ)
    rootparams[1].InitAsDescriptorTable(1, &descRangeSRV);

    // スタティックサンプラー
    CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

    // パイプラインとルートシグネチャのセット
    PipelineSet pipelineSet;

    // ルートシグネチャの生成
    // ルートシグネチャの設定
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSigBlob;
    // バージョン自動判定でのシリアライズ
    result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
    // ルートシグネチャの生成
    result = dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&pipelineSet.rootsignature));

    // パイプラインにルートシグネチャをセット
    gpipeline.pRootSignature = pipelineSet.rootsignature.Get();

    result = dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelineSet.pipelinestate));

    // パイプラインとルートシグネチャを返す
    return pipelineSet;
}

// スプライト共通データ生成
SpriteCommon SpriteCommonCreate(ID3D12Device* dev, int window_width, int window_height)
{
    HRESULT result = S_FALSE;

    SpriteCommon spriteCommon{};

    // スプライト用パイプライン生成
    spriteCommon.pipelineSet = SpriteCreateGraphicsPipeline(dev);

    // 並行投影の射影行列生成
    spriteCommon.matProjection = XMMatrixOrthographicOffCenterLH(
        0.0f, (float)WinApp::windows_width, (float)window_height, 0.0f, 0.0f, 1.0f);

    // デスクリプタヒープを生成 
    D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
    descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descHeapDesc.NumDescriptors = spriteSRVCount;
    result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&spriteCommon.descHeap));

    return spriteCommon;
}

// スプライト共通テクスチャ読み込み
void SpriteCommonLoadTexture(SpriteCommon& spriteCommon, UINT texnumber, const wchar_t* filename, ID3D12Device* dev)
{
    // 異常な番号の指定を検出
    assert(texnumber <= spriteSRVCount-1);

    HRESULT result = S_FALSE;

    // WICテクスチャのロード
    TexMetadata metadata{};
    ScratchImage scratchImg{};

    result = LoadFromWICFile(
        filename,
        WIC_FLAGS_NONE,
        &metadata, scratchImg);

    const Image* img = scratchImg.GetImage(0, 0, 0); // 生データ抽出

    // リソース設定
    CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        metadata.format,
        metadata.width,
        (UINT)metadata.height,
        (UINT16)metadata.arraySize,
        (UINT16)metadata.mipLevels);

    // テクスチャ用バッファの生成
    result = dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0),
        D3D12_HEAP_FLAG_NONE,
        &texresDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, // テクスチャ用指定
        nullptr,
        IID_PPV_ARGS(&spriteCommon.texBuff[texnumber]));

    // テクスチャバッファにデータ転送
    result = spriteCommon.texBuff[texnumber]->WriteToSubresource(
        0,
        nullptr, // 全領域へコピー
        img->pixels,    // 元データアドレス
        (UINT)img->rowPitch,    // 1ラインサイズ
        (UINT)img->slicePitch   // 1枚サイズ
    );

    // シェーダリソースビュー設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // 設定構造体
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
    srvDesc.Texture2D.MipLevels = 1;

    // ヒープのtexnumber番目にシェーダーリソースビュー作成
    dev->CreateShaderResourceView(
        spriteCommon.texBuff[texnumber].Get(), //ビューと関連付けるバッファ
        &srvDesc, //テクスチャ設定情報
        CD3DX12_CPU_DESCRIPTOR_HANDLE(spriteCommon.descHeap->GetCPUDescriptorHandleForHeapStart(), texnumber, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
    );
}

// スプライト単体頂点バッファの転送
void SpriteTransferVertexBuffer(const Sprite& sprite, const SpriteCommon& spriteCommon)
{
    HRESULT result = S_FALSE;

    // 頂点データ
    VertexPosUv vertices[] = {
        //     u     v
        {{}, {0.0f, 1.0f}}, // 左下
        {{}, {0.0f, 0.0f}}, // 左上
        {{}, {1.0f, 1.0f}}, // 右下
        {{}, {1.0f, 0.0f}}, // 右上
    };

    // 左下、左上、右下、右上
    enum { LB, LT, RB, RT };

    float left = (0.0f - sprite.anchorpoint.x) * sprite.size.x;
    float right = (1.0f - sprite.anchorpoint.x) * sprite.size.x;
    float top = (0.0f - sprite.anchorpoint.y) * sprite.size.y;
    float bottom = (1.0f - sprite.anchorpoint.y) * sprite.size.y;

    if (sprite.isFlipX)
    {// 左右入れ替え
        left = -left;
        right = -right;
    }

    if (sprite.isFlipY)
    {// 左右入れ替え
        top = -top;
        bottom = -bottom;
    }

    vertices[LB].pos = { left, bottom,  0.0f }; // 左下
    vertices[LT].pos = { left, top,     0.0f }; // 左上
    vertices[RB].pos = { right, bottom, 0.0f }; // 右下
    vertices[RT].pos = { right, top,    0.0f }; // 右上

    // 指定番号の画像が読み込み済みなら
    if (spriteCommon.texBuff[sprite.texNumber]) {
        // テクスチャ情報取得
        D3D12_RESOURCE_DESC resDesc = spriteCommon.texBuff[sprite.texNumber]->GetDesc();

        float tex_left = sprite.texLeftTop.x / resDesc.Width;
        float tex_right = (sprite.texLeftTop.x + sprite.texSize.x) / resDesc.Width;
        float tex_top = sprite.texLeftTop.y / resDesc.Height;
        float tex_bottom = (sprite.texLeftTop.y + sprite.texSize.y) / resDesc.Height;

        vertices[LB].uv = { tex_left,   tex_bottom   }; // 左下
        vertices[LT].uv = { tex_left,   tex_top      }; // 左上
        vertices[RB].uv = { tex_right,  tex_bottom   }; // 右下
        vertices[RT].uv = { tex_right,  tex_top      }; // 右上
    }

    // 頂点バッファへのデータ転送
    VertexPosUv* vertMap = nullptr;
    result = sprite.vertBuff->Map(0, nullptr, (void**)&vertMap);
    memcpy(vertMap, vertices, sizeof(vertices));
    sprite.vertBuff->Unmap(0, nullptr);
}

// スプライト生成
Sprite SpriteCreate(ID3D12Device* dev, int window_width, int window_height, UINT texNumber, const SpriteCommon& spriteCommon, XMFLOAT2 anchorpoint = { 0.5f,0.5f }, bool isFlipX = false, bool isFlipY = false)
{
    HRESULT result = S_FALSE;

    // 新しいスプライトを作る
    Sprite sprite{};

    // テクスチャ番号をコピー
    sprite.texNumber = texNumber;

    // アンカーポイントをコピー
    sprite.anchorpoint = anchorpoint;

    // 反転フラグをコピー
    sprite.isFlipX = isFlipX;
    sprite.isFlipY = isFlipY;

    // 頂点データ
    VertexPosUv vertices[4];

    // 指定番号の画像が読み込み済みなら
    if (spriteCommon.texBuff[sprite.texNumber]) {
        // テクスチャ情報取得
        D3D12_RESOURCE_DESC resDesc = spriteCommon.texBuff[sprite.texNumber]->GetDesc();

        // スプライトの大きさを画像の解像度に合わせる
        sprite.size = { (float)resDesc.Width, (float)resDesc.Height };
        sprite.texSize = { (float)resDesc.Width, (float)resDesc.Height };
    }

    // 頂点バッファ生成
    result = dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sprite.vertBuff));

    // 頂点バッファデータ転送
    SpriteTransferVertexBuffer(sprite, spriteCommon);

    // 頂点バッファビューの作成
    sprite.vbView.BufferLocation = sprite.vertBuff->GetGPUVirtualAddress();
    sprite.vbView.SizeInBytes = sizeof(vertices);
    sprite.vbView.StrideInBytes = sizeof(vertices[0]);

    // 定数バッファの生成
    result = dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&sprite.constBuff));

    // 定数バッファにデータ転送
    ConstBufferData* constMap = nullptr;
    result = sprite.constBuff->Map(0, nullptr, (void**)&constMap);
    constMap->color = XMFLOAT4(1, 1, 1, 1); // 色指定（RGBA）
    constMap->mat = XMMatrixOrthographicOffCenterLH(
        0.0f, (float)window_width, (float)window_height, 0.0f, 0.0f, 1.0f);   // 平行投影行列の合成    
    sprite.constBuff->Unmap(0, nullptr);    

    // 初期化した、新しいスプライトを返す
    return sprite;
}

// スプライト共通グラフィックコマンドのセット
void SpriteCommonBeginDraw(const SpriteCommon& spriteCommon, ID3D12GraphicsCommandList* cmdList)
{
    // パイプラインステートの設定
    cmdList->SetPipelineState(spriteCommon.pipelineSet.pipelinestate.Get());
    // ルートシグネチャの設定
    cmdList->SetGraphicsRootSignature(spriteCommon.pipelineSet.rootsignature.Get());
    // プリミティブ形状を設定
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // テクスチャ用デスクリプタヒープの設定
    ID3D12DescriptorHeap* ppHeaps[] = { spriteCommon.descHeap.Get() };
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

// スプライト単体更新
void SpriteUpdate(Sprite& sprite, const SpriteCommon& spriteCommon)
{
    // ワールド行列の更新
    sprite.matWorld = XMMatrixIdentity();
    // Z軸回転
    sprite.matWorld *= XMMatrixRotationZ(XMConvertToRadians(sprite.rotation));
    // 平行移動
    sprite.matWorld *= XMMatrixTranslation(sprite.position.x, sprite.position.y, sprite.position.z);

    // 定数バッファの転送
    ConstBufferData* constMap = nullptr;
    HRESULT result = sprite.constBuff->Map(0, nullptr, (void**)&constMap);
    constMap->mat = sprite.matWorld * spriteCommon.matProjection;
    constMap->color = sprite.color;
    sprite.constBuff->Unmap(0, nullptr);
}

// スプライト単体描画
void SpriteDraw(const Sprite& sprite, ID3D12GraphicsCommandList* cmdList, const SpriteCommon&spriteCommon, ID3D12Device* dev)
{
    if (sprite.isInvisible) {
        return;
    }

    // 頂点バッファをセット
    cmdList->IASetVertexBuffers(0, 1, &sprite.vbView);

    // 定数バッファをセット
    cmdList->SetGraphicsRootConstantBufferView(0, sprite.constBuff->GetGPUVirtualAddress());

    // シェーダリソースビューをセット
    cmdList->SetGraphicsRootDescriptorTable(1,
        CD3DX12_GPU_DESCRIPTOR_HANDLE(
            spriteCommon.descHeap->GetGPUDescriptorHandleForHeapStart(),
            sprite.texNumber,
            dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

    // ポリゴンの描画（4頂点で四角形）
    cmdList->DrawInstanced(4, 1, 0, 0);
}

// デバッグ文字列クラスの定義
class DebugText
{
public: // 定数の宣言    
    static const int maxCharCount = 256;    // 最大文字数
    static const int fontWidth = 9;         // フォント画像内1文字分の横幅
    static const int fontHeight = 18;       // フォント画像内1文字分の縦幅
    static const int fontLineCount = 14;    // フォント画像内1行分の文字数

public: // メンバ関数
    void Initialize(ID3D12Device* dev, int window_width, int window_height, UINT texnumber, const SpriteCommon& spriteCommon);

    void Print(const SpriteCommon& spriteCommon, const std::string& text, float x, float y, float scale = 1.0f);

    void DrawAll(ID3D12GraphicsCommandList* cmdList, const SpriteCommon& spriteCommon, ID3D12Device* dev);

private: // メンバ変数     
    // スプライトデータの配列
    Sprite sprites[maxCharCount];
    // スプライトデータ配列の添え字番号
    int spriteIndex = 0;
};

void DebugText::Initialize(ID3D12Device* dev, int window_width, int window_height, UINT texnumber, const SpriteCommon& spriteCommon)
{
    // 全てのスプライトデータについて
    for (int i = 0; i < _countof(sprites); i++)
    {
        // スプライトを生成する
        sprites[i] = SpriteCreate(dev, window_width, window_height, texnumber, spriteCommon, { 0,0 });
    }
}

void DebugText::Print(const SpriteCommon& spriteCommon, const std::string& text, float x, float y, float scale)
{
    // 全ての文字について
    for (int i = 0; i < text.size(); i++)
    {
        // 最大文字数超過
        if (spriteIndex >= maxCharCount) {
            break;
        }

        // 1文字取り出す(※ASCIIコードでしか成り立たない)
        const unsigned char& character = text[i];

        // ASCIIコードの2段分飛ばした番号を計算
        int fontIndex = character - 32;
        if (character >= 0x7f) {
            fontIndex = 0;
        }

        int fontIndexY = fontIndex / fontLineCount;
        int fontIndexX = fontIndex % fontLineCount;

        // 座標計算
        sprites[spriteIndex].position = { x + fontWidth * scale * i, y, 0 };
        sprites[spriteIndex].texLeftTop = { (float)fontIndexX * fontWidth, (float)fontIndexY * fontHeight };
        sprites[spriteIndex].texSize = { fontWidth, fontHeight };
        sprites[spriteIndex].size = { fontWidth * scale, fontHeight * scale };
        // 頂点バッファ転送
        SpriteTransferVertexBuffer(sprites[spriteIndex], spriteCommon);
        // 更新
        SpriteUpdate(sprites[spriteIndex], spriteCommon);

        // 文字を１つ進める
        spriteIndex++;
    }
}

// まとめて描画
void DebugText::DrawAll(ID3D12GraphicsCommandList* cmdList, const SpriteCommon& spriteCommon, ID3D12Device* dev)
{
    // 全ての文字のスプライトについて
    for (int i = 0; i < spriteIndex; i++)
    {
        // スプライト描画
        SpriteDraw(sprites[i], cmdList, spriteCommon, dev);
    }

    spriteIndex = 0;
}

// 3Dオブジェクト型
struct Object3d
{
    // 定数バッファ
    ComPtr<ID3D12Resource> constBuff;
    // アフィン変換情報
    XMFLOAT3 scale = { 1,1,1 };
    XMFLOAT3 rotation = { 0,0,0 };
    XMFLOAT3 position = { 0,0,0 };
    // ワールド変換行列
    XMMATRIX matWorld;
    // 親オブジェクトへのポインタ
    Object3d* parent = nullptr;
};

// 3Dオブジェクト初期化
void InitializeObject3d(Object3d* object, int index, ID3D12Device* dev, ID3D12DescriptorHeap* descHeap)
{
    HRESULT result;
        
    // 定数バッファの生成
    result = dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),   // アップロード可能
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&object->constBuff));
}

void UpdateObject3d(Object3d* object, XMMATRIX& matView, XMMATRIX& matProjection)
{
    XMMATRIX matScale, matRot, matTrans;

    // スケール、回転、平行移動行列の計算
    matScale = XMMatrixScaling( object->scale.x, object->scale.y, object->scale.z);
    matRot = XMMatrixIdentity();
    matRot *= XMMatrixRotationZ(XMConvertToRadians(object->rotation.z));
    matRot *= XMMatrixRotationX(XMConvertToRadians(object->rotation.x));
    matRot *= XMMatrixRotationY(XMConvertToRadians(object->rotation.y));
    matTrans = XMMatrixTranslation( object->position.x, object->position.y, object->position.z);

    // ワールド行列の合成
    object->matWorld = XMMatrixIdentity(); // 変形をリセット
    object->matWorld *= matScale; // ワールド行列にスケーリングを反映
    object->matWorld *= matRot; // ワールド行列に回転を反映
    object->matWorld *= matTrans; // ワールド行列に平行移動を反映

    // 親オブジェクトがあれば
    if (object->parent != nullptr) {
        // 親オブジェクトのワールド行列を掛ける
        object->matWorld *= object->parent->matWorld;
    }

    // 定数バッファへデータ転送
    ConstBufferData* constMap = nullptr;
    if (SUCCEEDED(object->constBuff->Map(0, nullptr, (void**)&constMap))) {
        constMap->color = XMFLOAT4(1, 1, 1, 1); // RGBA
        constMap->mat = object->matWorld * matView * matProjection;
        object->constBuff->Unmap(0, nullptr);
    }
}

void DrawObject3d(Object3d* object, ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* descHeap, D3D12_VERTEX_BUFFER_VIEW& vbView, D3D12_INDEX_BUFFER_VIEW& ibView, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandleSRV, unsigned short* indices, UINT numIndices )
{
    // デスクリプタヒープの配列
    ID3D12DescriptorHeap* ppHeaps[] = { descHeap };
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // 頂点バッファの設定
    cmdList->IASetVertexBuffers(0, 1, &vbView);
    // インデックスバッファの設定
    cmdList->IASetIndexBuffer(&ibView);

    // 定数バッファビューをセット
    cmdList->SetGraphicsRootConstantBufferView(0, object->constBuff->GetGPUVirtualAddress());
    // シェーダリソースビューをセット
    cmdList->SetGraphicsRootDescriptorTable(1, gpuDescHandleSRV);
    // 描画コマンド
    cmdList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
}

// チャンクヘッダ
struct ChunkHeader
{
    char id[4]; // チャンク毎のID
    int32_t size;  // チャンクサイズ
};

// RIFFヘッダチャンク
struct RiffHeader
{
    ChunkHeader chunk;   // "RIFF"
    char type[4]; // "WAVE"
};

// FMTチャンク
struct FormatChunk
{
    ChunkHeader chunk; // "fmt "
    WAVEFORMATEX fmt; // 波形フォーマット
};

class XAudio2VoiceCallback : public IXAudio2VoiceCallback
{
public:
    // ボイス処理パスの開始時
    STDMETHOD_(void, OnVoiceProcessingPassStart) (THIS_ UINT32 BytesRequired) {};
    // ボイス処理パスの終了時
    STDMETHOD_(void, OnVoiceProcessingPassEnd) (THIS) {};
    // バッファストリームの再生が終了した時
    STDMETHOD_(void, OnStreamEnd) (THIS) {};
    // バッファの使用開始時
    STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext) {};
    // バッファの末尾に達した時
    STDMETHOD_(void, OnBufferEnd) (THIS_ void* pBufferContext) {};
    // 再生がループ位置に達した時
    STDMETHOD_(void, OnLoopEnd) (THIS_ void* pBufferContext) {};
    // ボイスの実行エラー時
    STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error) {};
};

// 音声データ
struct SoundData
{
    // 波形フォーマット
    WAVEFORMATEX wfex;
    // バッファの先頭アドレス
    BYTE* pBuffer;
    // バッファのサイズ
    unsigned int bufferSize;
};

// 音声読み込み
SoundData SoundLoadWave(const char* filename)
{
    // ファイル入力ストリームのインスタンス
    std::ifstream file;
    // .wavファイルをバイナリモードで開く
    file.open(filename, std::ios_base::binary);
    // ファイルオープン失敗を検出する
    assert(file.is_open());

    // RIFFヘッダーの読み込み
    RiffHeader riff;
    file.read((char*)&riff, sizeof(riff));
    // ファイルがRIFFかチェック
    if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
        assert(0);
    }
    // タイプがWAVEかチェック
    if (strncmp(riff.type, "WAVE", 4) != 0) {
        assert(0);
    }

    // Formatチャンクの読み込み
    FormatChunk format = {};
    // チャンクヘッダーの確認
    file.read((char*)&format, sizeof(ChunkHeader));
    if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
        assert(0);
    }
    // チャンク本体の読み込み
    assert(format.chunk.size <= sizeof(format.fmt));
    file.read((char*)&format.fmt, format.chunk.size);

    // Dataチャンクの読み込み
    ChunkHeader data;
    file.read((char*)&data, sizeof(data));
    // JUNKチャンクを検出した場合
    if (strncmp(data.id, "JUNK ", 4) == 0) {
        // 読み取り位置をJUNKチャンクの終わりまで進める
        file.seekg(data.size, std::ios_base::cur);
        // 再読み込み
        file.read((char*)&data, sizeof(data));
    }

    if (strncmp(data.id, "data ", 4) != 0) {
        assert(0);
    }

    // Dataチャンクのデータ部（波形データ）の読み込み
    char* pBuffer = new char[data.size];
    file.read(pBuffer, data.size);

    // Waveファイルを閉じる
    file.close();

    // returnする為の音声データ
    SoundData soundData = {};

    soundData.wfex = format.fmt;
    soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
    soundData.bufferSize = data.size;

    return soundData;
}

// 音声データ解放
void SoundUnload(SoundData* soundData)
{
    // バッファのメモリを解放
    delete[] soundData->pBuffer;

    soundData->pBuffer = 0;
    soundData->bufferSize = 0;
    soundData->wfex = {};
}

// 音声再生
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData) {

    HRESULT result;

    // 波形フォーマットを元にSourceVoiceの生成
    IXAudio2SourceVoice* pSourceVoice = nullptr;
    result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
    assert(SUCCEEDED(result));

    // 再生する波形データの設定
    XAUDIO2_BUFFER buf{};
    buf.pAudioData = soundData.pBuffer;
    buf.AudioBytes = soundData.bufferSize;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    // 波形データの再生
    result = pSourceVoice->SubmitSourceBuffer(&buf);
    result = pSourceVoice->Start();
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    //ポインタ置き場
    Input* input = nullptr;
    WinApp* winApp = nullptr;

#pragma region WindowsAPI初期化

    //windowsAPIの初期化
    winApp = new WinApp();
    winApp->Initialize();


    MSG msg{};  // メッセージ
#pragma endregion WindowsAPI初期化

#pragma region DirectX初期化処理
    // DirectX初期化処理　ここから
#ifdef _DEBUG
    //デバッグレイヤーをオンに
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif

    HRESULT result;
    ComPtr<ID3D12Device> dev;
    ComPtr<IDXGIFactory6> dxgiFactory;
    ComPtr<IDXGISwapChain4> swapchain;
    ComPtr<ID3D12CommandAllocator> cmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ComPtr<ID3D12CommandQueue> cmdQueue;
    ComPtr<ID3D12DescriptorHeap> rtvHeaps;    

    // スプライト
    SpriteCommon spriteCommon;
    const int SPRITES_NUM = 1;
    Sprite sprites[SPRITES_NUM];

    // DXGIファクトリーの生成
    result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    // アダプターの列挙用
    std::vector<ComPtr<IDXGIAdapter1>> adapters;
    // ここに特定の名前を持つアダプターオブジェクトが入る
    ComPtr<IDXGIAdapter1> tmpAdapter = nullptr;
    for (int i = 0;
        dxgiFactory->EnumAdapters1(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
        i++)
    {
        adapters.push_back(tmpAdapter); // 動的配列に追加する
    }

    for (int i = 0; i < adapters.size(); i++)
    {
        DXGI_ADAPTER_DESC1 adesc;
        adapters[i]->GetDesc1(&adesc);  // アダプターの情報を取得

        // ソフトウェアデバイスを回避
        if (adesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        std::wstring strDesc = adesc.Description;   // アダプター名
        // Intel UHD Graphics（オンボードグラフィック）を回避
        if (strDesc.find(L"Intel") == std::wstring::npos)
        {
            tmpAdapter = adapters[i];   // 採用
            break;
        }
    }

    // 対応レベルの配列
    D3D_FEATURE_LEVEL levels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D_FEATURE_LEVEL featureLevel;

    for (int i = 0; i < _countof(levels); i++)
    {
        // 採用したアダプターでデバイスを生成
        result = D3D12CreateDevice(tmpAdapter.Get(), levels[i], IID_PPV_ARGS(&dev));
        if (result == S_OK)
        {
            // デバイスを生成できた時点でループを抜ける
            featureLevel = levels[i];
            break;
        }
    }

    // コマンドアロケータを生成
    result = dev->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&cmdAllocator));

    // コマンドリストを生成
    result = dev->CreateCommandList(0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        cmdAllocator.Get(), nullptr,
        IID_PPV_ARGS(&cmdList));

    // 標準設定でコマンドキューを生成
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};

    dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));

    // 各種設定をしてスワップチェーンを生成
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
    swapchainDesc.Width = 1280;
    swapchainDesc.Height = 720;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 色情報の書式
    swapchainDesc.SampleDesc.Count = 1; // マルチサンプルしない
    swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER; // バックバッファ用
    swapchainDesc.BufferCount = 2;  // バッファ数を２つに設定
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップ後は破棄
    swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain1> swapchain1;

    // スワップチェーンの生成
    dxgiFactory->CreateSwapChainForHwnd(
        cmdQueue.Get(),
        winApp->GetHwnd(),
        &swapchainDesc,
        nullptr,
        nullptr,
        &swapchain1);

    // 生成したIDXGISwapChain1のオブジェクトをIDXGISwapChain4に変換する
    swapchain1.As(&swapchain);

    // 各種設定をしてデスクリプタヒープを生成
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Type =
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー
    heapDesc.NumDescriptors = 2;    // 裏表の２つ
    dev->CreateDescriptorHeap(&heapDesc,
        IID_PPV_ARGS(&rtvHeaps));
    // 裏表の２つ分について
    std::vector<ComPtr<ID3D12Resource>> backBuffers(2);

    for (int i = 0; i < 2; i++)
    {
        // スワップチェーンからバッファを取得
        result = swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));

        // レンダーターゲットビューの生成
        dev->CreateRenderTargetView(
            backBuffers[i].Get(),
            nullptr,
            CD3DX12_CPU_DESCRIPTOR_HANDLE(
                rtvHeaps->GetCPUDescriptorHandleForHeapStart(), 
                i, 
                dev->GetDescriptorHandleIncrementSize(heapDesc.Type)
            )
        );
    }

    ComPtr<ID3D12Resource> depthBuffer;
    // 深度バッファリソース設定
    CD3DX12_RESOURCE_DESC depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT,
        WinApp::windows_width,
        WinApp::windows_height,
        1, 0,
        1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    // 深度バッファの生成
    result = dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthResDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値書き込みに使用
        &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
        IID_PPV_ARGS(&depthBuffer));

    // 深度ビュー用デスクリプタヒープ作成
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1; // 深度ビューは1つ
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // デプスステンシルビュー
    ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;
    result = dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

    // 深度ビュー作成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値フォーマット
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dev->CreateDepthStencilView(
        depthBuffer.Get(),
        &dsvDesc,
        dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // フェンスの生成
    ComPtr<ID3D12Fence> fence;
    UINT64 fenceVal = 0;

    result = dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

   
    //入力の初期化
    input = new Input();
    input->Initialize(winApp);


    ComPtr<IXAudio2> xAudio2;
    IXAudio2MasteringVoice* masterVoice;
    XAudio2VoiceCallback voiceCallback;

    // XAudioエンジンのインスタンスを生成
    result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(result));

    // マスターボイスを生成
    result = xAudio2->CreateMasteringVoice(&masterVoice);
    assert(SUCCEEDED(result));

    // 音声読み込み
    SoundData soundData1 = SoundLoadWave("Resources/Alarm01.wav");

    // 音声再生
    //SoundPlayWave(xAudio2.Get(), soundData1);

    // DirectX初期化処理　ここまで
#pragma endregion DirectX初期化処理

#pragma region 描画初期化処理

    // 頂点データ構造体
    struct Vertex
    {
        XMFLOAT3 pos;       // xyz座標
        XMFLOAT3 normal;    // 法線ベクトル
        XMFLOAT2 uv;        // uv座標
    };
    
    const float topHeight = 10.0f;   // 天井の高さ
    const float radius = 10.0f;
    const int DIV = 3;    

    Vertex vertices[DIV + 1 + 1] = {};

    vertices[0].pos.x = radius * sinf(XM_2PI * 0 / DIV); vertices[0].pos.y = radius * cosf(XM_2PI * 0 / DIV); vertices[0].pos.z = 0.0f;
    vertices[1].pos.x = radius * sinf(XM_2PI * 1 / DIV); vertices[1].pos.y = radius * cosf(XM_2PI * 1 / DIV); vertices[1].pos.z = 0.0f;
    vertices[2].pos.x = radius * sinf(XM_2PI * 2 / DIV); vertices[2].pos.y = radius * cosf(XM_2PI * 2 / DIV); vertices[2].pos.z = 0.0f;
    vertices[3].pos = { 0,0,0 };
    vertices[4].pos = { 0,0,-topHeight };

    // インデックスデータ
    unsigned short indices[3 * DIV * 2];

    indices[0] = 1;
    indices[1] = 0;
    indices[2] = 3;

    indices[3] = 2;
    indices[4] = 1;
    indices[5] = 3;

    indices[6] = 0;
    indices[7] = 2;
    indices[8] = 3;

    indices[9] = 0;
    indices[10] = 1;
    indices[11] = 4;

    indices[12] = 1;
    indices[13] = 2;
    indices[14] = 4;

    indices[15] = 2;
    indices[16] = 0;
    indices[17] = 4;

    for (int i = 0; i < _countof(indices) / 3; i++)
    {// 三角形１つごとに計算していく
        // 三角形のインデックスを取り出して、一時的な変数に入れる
        unsigned short index0 = indices[i * 3 + 0];
        unsigned short index1 = indices[i * 3 + 1];
        unsigned short index2 = indices[i * 3 + 2];
        // 三角形を構成する頂点座標をベクトルに代入
        XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
        XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
        XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
        // p0→p1ベクトル、p0→p2ベクトルを計算　（ベクトルの減算）
        XMVECTOR v1 = XMVectorSubtract(p1, p0);
        XMVECTOR v2 = XMVectorSubtract(p2, p0);
        // 外積は両方から垂直なベクトル
        XMVECTOR normal = XMVector3Cross(v1, v2);
        // 正規化（長さを1にする)
        normal = XMVector3Normalize(normal);
        // 求めた法線を頂点データに代入
        XMStoreFloat3(&vertices[index0].normal, normal);
        XMStoreFloat3(&vertices[index1].normal, normal);
        XMStoreFloat3(&vertices[index2].normal, normal);
    }

    // 頂点バッファのサイズ = 頂点データ一つ分のサイズ * 頂点データの要素数
    UINT sizeVB = static_cast<UINT>(sizeof(Vertex) * _countof(vertices));

    // 頂点バッファの生成
    ComPtr<ID3D12Resource> vertBuff;
    result = dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),   // アップロード可能
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeVB),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertBuff));

    // GPU上のバッファに対応した仮想メモリを取得
    Vertex* vertMap = nullptr;
    result = vertBuff->Map(0, nullptr, (void**)&vertMap);
    // 全頂点に対して
    for (int i = 0; i < _countof(vertices); i++)
    {
        vertMap[i] = vertices[i];   // 座標をコピー
    }
    // マップを解除
    vertBuff->Unmap(0, nullptr);

    // 頂点バッファビューの作成
    D3D12_VERTEX_BUFFER_VIEW vbView{};
    vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
    vbView.SizeInBytes = sizeVB;
    vbView.StrideInBytes = sizeof(Vertex);

    // インデックスデータ全体のサイズ
    UINT sizeIB = static_cast<UINT>(sizeof(unsigned short) * _countof(indices));

    // インデックスバッファの生成
    ComPtr<ID3D12Resource> indexBuff;
    result = dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),   // アップロード可能
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeIB),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indexBuff));

    // インデックスバッファビューの作成
    D3D12_INDEX_BUFFER_VIEW ibView{};
    ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R16_UINT;
    ibView.SizeInBytes = sizeIB;

    // GPU上のバッファに対応した仮想メモリを取得
    unsigned short* indexMap = nullptr;
    result = indexBuff->Map(0, nullptr, (void**)&indexMap);

    // 全インデックスに対して
    for (int i = 0; i < _countof(indices); i++)
    {
        indexMap[i] = indices[i];   // インデックスをコピー
    }
    // 繋がりを解除
    indexBuff->Unmap(0, nullptr);      

    //// 射影変換行列(並行投影)
    //XMMATRIX matProjection = XMMatrixOrthographicOffCenterLH(
    //    0, window_width,
    //    window_height, 0,
    //    0, 1000.0f
    //);

    // 射影変換行列(透視投影)
    XMMATRIX matProjection = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(60.0f),              // 画角60度
        (float)WinApp::windows_width / WinApp::windows_height,    // アスペクト比（画面横幅/画面縦幅）
        0.1f, 1000.0f                           // 前端、奥端
    );

    // ビュー変換行列
    XMMATRIX matView;
    XMFLOAT3 eye(0, 0, -100);   // 視点座標
    XMFLOAT3 target(0, 0, 0);   // 注視点座標
    XMFLOAT3 up(0, 1, 0);       // 上方向ベクトル
    matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));    

    float angle = 0.0f; // カメラの回転角    

    const int constantBufferNum = 128; // 定数バッファの最大数

    // 定数バッファ用デスクリプタヒープの生成
    ComPtr<ID3D12DescriptorHeap> basicDescHeap = nullptr;
    // 設定構造体
    D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
    descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーから見える
    descHeapDesc.NumDescriptors = constantBufferNum + 1;
    // 生成
    result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));

    // 3Dオブジェクトの数
    const int OBJECT_NUM = 2;

    Object3d object3ds[OBJECT_NUM];

    // 配列内の全オブジェクトに対して
    for (int i = 0; i < _countof(object3ds); i++)
    {
        // 初期化
        InitializeObject3d(&object3ds[i], i, dev.Get(), basicDescHeap.Get());

        // ここから↓は親子構造のサンプル
        // 先頭以外なら
        //if (i > 0) {
        //    // 一つ前のオブジェクトを親オブジェクトとする
        //    //object3ds[i].parent = &object3ds[i - 1];
        //    // 親オブジェクトの9割の大きさ
        //    object3ds[i].scale = { 0.9f,0.9f,0.9f };
        //    // 親オブジェクトに対してZ軸まわりに30度回転
        //    object3ds[i].rotation = { 0.0f,0.0f,30.0f };
        //    // 親オブジェクトに対してZ方向-8.0ずらす
        //    object3ds[i].position = { 0.0f,0.0f,-8.0f };
        //}
    }

    object3ds[0].scale = { 0.5f, 0.5f, 0.5f };
    object3ds[1].scale = { 0.5f, 0.5f, 0.5f };

    // スプライト共通データ生成
    spriteCommon = SpriteCommonCreate(dev.Get(), WinApp::windows_width, WinApp::windows_height);
    // スプライト共通テクスチャ読み込み
    SpriteCommonLoadTexture(spriteCommon, 0, L"Resources/texture.png", dev.Get());
    SpriteCommonLoadTexture(spriteCommon, 1, L"Resources/house.png", dev.Get());

    // スプライトの生成
    for (int i = 0; i < _countof(sprites); i++)
    {
        int texNumber = rand() % 2;
        sprites[i] = SpriteCreate(dev.Get(), WinApp::windows_width, WinApp::windows_height, texNumber, spriteCommon, { 0,0 }, false, false);

        // スプライトの座標変更
        sprites[i].position.x = 1280/2;
        sprites[i].position.y = 720/2;
        //sprites[i].isInvisible = true;
        //sprites[i].position.x = (float)(rand() % 1280);
        //sprites[i].position.y = (float)(rand() % 720);

        //sprites[i].rotation = (float)(rand() % 360);
        //sprites[i].rotation = 0;

        //sprites[i].size.x = 400.0f;
        //sprites[i].size.y = 100.0f;
        // 頂点バッファに反映
        SpriteTransferVertexBuffer(sprites[i], spriteCommon);
    }

    // デバッグテキスト
    DebugText debugText;

    // デバッグテキスト用のテクスチャ番号を指定
    const int debugTextTexNumber = 2;
    // デバッグテキスト用のテクスチャ読み込み
    SpriteCommonLoadTexture(spriteCommon, debugTextTexNumber, L"Resources/debugfont.png", dev.Get());
    // デバッグテキスト初期化
    debugText.Initialize(dev.Get(), WinApp::windows_width, WinApp::windows_height, debugTextTexNumber, spriteCommon);

    // WICテクスチャのロード
    TexMetadata metadata{};
    ScratchImage scratchImg{};

    result = LoadFromWICFile(
        L"Resources/house.png",   //「Resources」フォルダの「texture.png」
        WIC_FLAGS_NONE,
        &metadata, scratchImg);

    const Image* img = scratchImg.GetImage(0, 0, 0); // 生データ抽出

    // リソース設定
    CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        metadata.format,
        metadata.width,
        (UINT)metadata.height,
        (UINT16)metadata.arraySize,
        (UINT16)metadata.mipLevels);

    // テクスチャ用バッファの生成
    ComPtr<ID3D12Resource> texbuff = nullptr;
    result = dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0),
        D3D12_HEAP_FLAG_NONE,
        &texresDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, // テクスチャ用指定
        nullptr,
        IID_PPV_ARGS(&texbuff));

    // テクスチャバッファにデータ転送
    result = texbuff->WriteToSubresource(
        0,
        nullptr, // 全領域へコピー
        img->pixels,    // 元データアドレス
        (UINT)img->rowPitch,    // 1ラインサイズ
        (UINT)img->slicePitch   // 1枚サイズ
    );

    // シェーダリソースビュー設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // 設定構造体
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
    srvDesc.Texture2D.MipLevels = 1;

    // ヒープの2番目にシェーダーリソースビュー作成
    dev->CreateShaderResourceView(texbuff.Get(), //ビューと関連付けるバッファ
        &srvDesc, //テクスチャ設定情報
        CD3DX12_CPU_DESCRIPTOR_HANDLE(basicDescHeap->GetCPUDescriptorHandleForHeapStart(), constantBufferNum, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
    );

    // 3Dオブジェクト用パイプライン生成
    PipelineSet object3dPipelineSet = Object3dCreateGraphicsPipeline(dev.Get());    

#pragma endregion 描画初期化処理

    int counter = 0; // アニメーションの経過時間カウンター

    while (true)  // ゲームループ
    {
        //windowsのメッセージ処理
        if (winApp->ProcessMessage()) {
            //ゲームループを抜ける
            break;
        }

#pragma region DirectX毎フレーム処理
        // DirectX毎フレーム処理　ここから

        input->Update();

        const int cycle = 540; // 繰り返しの周期
        counter++;
        counter %= cycle; // 周期を超えたら0に戻る
        float scale = (float)counter / cycle; // [0,1]の数値

        //const int cycle = 60; // 繰り返しの周期
        //counter++;
        //float scale = sinf(XM_2PI * (float)counter / cycle); // [-1,+1]の数値
        //scale += 1.0f; // [0,+2]の数値
        //scale /= 2.0f; // [0,+1]の数値
        scale *= 360.0f;

        object3ds[0].rotation = { 0, 0, scale };
        object3ds[1].rotation = { 0, 0, -scale };

        float offset = sinf(XM_2PI * (float)counter / cycle);
        offset *= 10.0f;
        object3ds[0].position = { offset, 0, 0 };
        object3ds[1].position = { -offset, 0, 0 };

        if (input->PushKey(DIK_0)) // 数字の0キーが押されていたら
        {
            OutputDebugStringA("Hit 0\n");  // 出力ウィンドウに「Hit 0」と表示
        }        

        float clearColor[] = { 0.1f,0.25f, 0.5f,0.0f }; // 青っぽい色

        if (input->PushKey(DIK_SPACE))     // スペースキーが押されていたら
        {
            // 画面クリアカラーの数値を書き換える
            clearColor[1] = 1.0f;
        }

        // 座標操作
        if (input->PushKey(DIK_UP) || input->PushKey(DIK_DOWN) || input->PushKey(DIK_RIGHT) || input->PushKey(DIK_LEFT))
        {
            if (input->PushKey(DIK_UP)) { object3ds[0].position.y += 1.0f; }
            else if (input->PushKey(DIK_DOWN)) { object3ds[0].position.y -= 1.0f; }
            if (input->PushKey(DIK_RIGHT)) { object3ds[0].position.x += 1.0f; }
            else if (input->PushKey(DIK_LEFT)) { object3ds[0].position.x -= 1.0f; }
        }


        if (input->PushKey(DIK_D) || input->PushKey(DIK_A))
        {
            if (input->PushKey(DIK_D)) { angle += XMConvertToRadians(1.0f); }
            else if (input->PushKey(DIK_A)) { angle -= XMConvertToRadians(1.0f); }

            // angleラジアンだけY軸まわりに回転。半径は-100
            eye.x = -100 * sinf(angle);
            eye.z = -100 * cosf(angle);
            matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
        }

        for (int i = 0; i < _countof(object3ds); i++)
        {
            UpdateObject3d(&object3ds[i], matView, matProjection);
        }

        // ワールド行列の最下段がワールド座標
        const XMVECTOR& position1 = object3ds[0].matWorld.r[3];
        const XMVECTOR& position2 = object3ds[1].matWorld.r[3];

        // ２つの座標の差
        XMVECTOR position_sub = position1 - position2;

        // ２つの座標の距離
        position_sub = XMVector3Length(position_sub);
        float distance = position_sub.m128_f32[0];

        // 各オブジェクトの半径
        const float radius1 = 3.0f;
        const float radisu2 = 3.0f;

        // 距離が半径の和以下なら当たっている
        if (distance <= radius1 + radisu2) {
            debugText.Print(spriteCommon, "Hit", 100, 100, 10.0f);
        }

        //// X座標,Y座標を指定して表示
        //debugText.Print(spriteCommon, "Hello,DirectX!!", 200, 100);
        //// X座標,Y座標,縮尺を指定して表示
        //debugText.Print(spriteCommon, "Nihon Kogakuin", 200, 200, 2.0f);

        //sprite.rotation = 45;
        //sprite.position = {1280/2, 720/2, 0};
        //sprite.color = {0, 0, 1, 1};

        // GPU上のバッファに対応した仮想メモリを取得
        Vertex* vertMap = nullptr;
        result = vertBuff->Map(0, nullptr, (void**)&vertMap);
        // 全頂点に対して
        for (int i = 0; i < _countof(vertices); i++)
        {
            vertMap[i] = vertices[i];   // 座標をコピー
        }
        // マップを解除
        vertBuff->Unmap(0, nullptr);
        
        // DirectX毎フレーム処理　ここまで
#pragma endregion DirectX毎フレーム処理

#pragma region グラフィックスコマンド
        // バックバッファの番号を取得（2つなので0番か1番）
        UINT bbIndex = swapchain->GetCurrentBackBufferIndex();

        // １．リソースバリアで書き込み可能に変更
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[bbIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

        // ２．描画先指定
        // レンダーターゲットビュー用ディスクリプタヒープのハンドルを取得
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvH = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeaps->GetCPUDescriptorHandleForHeapStart(), bbIndex, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
        // 深度ステンシルビュー用デスクリプタヒープのハンドルを取得
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvH = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeap->GetCPUDescriptorHandleForHeapStart());
        cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

        // ３．画面クリア           R     G     B    A
        //float clearColor[] = { 0.1f,0.25f, 0.5f,0.0f }; // 青っぽい色
        cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
        cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // ４．描画コマンドここから
        // パイプラインステートとルートシグネチャの設定
        cmdList->SetPipelineState(object3dPipelineSet.pipelinestate.Get());
        cmdList->SetGraphicsRootSignature(object3dPipelineSet.rootsignature.Get());
        
        // ビューポート領域の設定
        cmdList->RSSetViewports(1, &CD3DX12_VIEWPORT(0.0f, 0.0f, WinApp::windows_width, WinApp::windows_height));
        // シザー矩形の設定
        cmdList->RSSetScissorRects(1, &CD3DX12_RECT(0, 0, WinApp::windows_width, WinApp::windows_height));

        //cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

        for (int i = 0; i < _countof(object3ds); i++)
        {
            DrawObject3d(&object3ds[i], cmdList.Get(), basicDescHeap.Get(), vbView, ibView, 
                CD3DX12_GPU_DESCRIPTOR_HANDLE(basicDescHeap->GetGPUDescriptorHandleForHeapStart(), constantBufferNum, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
                indices, _countof(indices));
        }

        // スプライト共通コマンド
        SpriteCommonBeginDraw(spriteCommon, cmdList.Get());
        // スプライト描画
        for (int i = 0; i < _countof(sprites); i++)
        {
            SpriteDraw(sprites[i], cmdList.Get(), spriteCommon, dev.Get());
        }
        // デバッグテキスト描画
        debugText.DrawAll(cmdList.Get(), spriteCommon, dev.Get());

        // ４．描画コマンドここまで

        // ５．リソースバリアを戻す
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[bbIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

        // 命令のクローズ
        cmdList->Close();
        // コマンドリストの実行
        ID3D12CommandList* cmdLists[] = { cmdList.Get() }; // コマンドリストの配列
        cmdQueue->ExecuteCommandLists(1, cmdLists);
        // コマンドリストの実行完了を待つ
        cmdQueue->Signal(fence.Get(), ++fenceVal);
        if (fence->GetCompletedValue() != fenceVal) {
            HANDLE event = CreateEvent(nullptr, false, false, nullptr);
            fence->SetEventOnCompletion(fenceVal, event);
            WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }

        cmdAllocator->Reset(); // キューをクリア
        cmdList->Reset(cmdAllocator.Get(), nullptr);  // 再びコマンドリストを貯める準備
#pragma endregion グラフィックスコマンド

        // バッファをフリップ（裏表の入替え）
        swapchain->Present(1, 0);

    }

    //入力解放
    delete input;

    //windowsAPIの終了処理
    winApp->Finalize();

    //WindowsAPI解放
    delete winApp;
    winApp = nullptr;

    // XAudio2解放
    xAudio2.Reset();
    // 音声データ解放
    SoundUnload(&soundData1);


	return 0;
}