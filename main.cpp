#include <Windows.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <vector>
#include <string>
#include <fstream>
#include <DirectXMath.h>


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
#include "DirectXCommon.h"
#include "Object3d.h"
#include "Model.h"
#include "SpriteCommon.h"
#include "Sprite.h"

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


// スプライト単体更新
//void SpriteUpdate(Sprite& sprite, const SpriteCommon& spriteCommon)
//{
//	// ワールド行列の更新
//	sprite.matWorld_ = XMMatrixIdentity();
//	// Z軸回転
//	sprite.matWorld_ *= XMMatrixRotationZ(XMConvertToRadians(sprite.rotation_));
//	// 平行移動
//	sprite.matWorld_ *= XMMatrixTranslation(sprite.position_.x, sprite.position_.y, sprite.position_.z);
//
//	// 定数バッファの転送
//	ConstBufferData* constMap = nullptr;
//	HRESULT result = sprite.constBuff->Map(0, nullptr, (void**)&constMap);
//	constMap->mat = sprite.matWorld_ * spriteCommon.matProjection;
//	constMap->color = sprite.color;
//	sprite.constBuff->Unmap(0, nullptr);
//}

// スプライト単体描画
//void SpriteDraw(const Sprite& sprite, ID3D12GraphicsCommandList* cmdList, const SpriteCommon& spriteCommon, ID3D12Device* dev)
//{
//	if (sprite.isInvisible) {
//		return;
//	}
//
//	// 頂点バッファをセット
//	cmdList->IASetVertexBuffers(0, 1, &sprite.vbView_);
//
//	// 定数バッファをセット
//	cmdList->SetGraphicsRootConstantBufferView(0, sprite.constBuff->GetGPUVirtualAddress());
//
//	// シェーダリソースビューをセット
//	cmdList->SetGraphicsRootDescriptorTable(1,
//		CD3DX12_GPU_DESCRIPTOR_HANDLE(
//			spriteCommon.descHeap_->GetGPUDescriptorHandleForHeapStart(),
//			sprite.texNumber,
//			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
//
//	// ポリゴンの描画（4頂点で四角形）
//	cmdList->DrawInstanced(4, 1, 0, 0);
//}

// デバッグ文字列クラスの定義
//class DebugText
//{
//public: // 定数の宣言    
//	static const int maxCharCount = 256;    // 最大文字数
//	static const int fontWidth = 9;         // フォント画像内1文字分の横幅
//	static const int fontHeight = 18;       // フォント画像内1文字分の縦幅
//	static const int fontLineCount = 14;    // フォント画像内1行分の文字数
//
//public: // メンバ関数
//	void Initialize(ID3D12Device* dev, int window_width, int window_height, UINT texnumber, const SpriteCommon& spriteCommon);
//
//	void Print(const SpriteCommon& spriteCommon, const std::string& text, float x, float y, float scale_ = 1.0f);
//
//	void DrawAll(ID3D12GraphicsCommandList* cmdList, const SpriteCommon& spriteCommon, ID3D12Device* dev);
//
//private: // メンバ変数     
//	// スプライトデータの配列
//	Sprite sprites[maxCharCount];
//	// スプライトデータ配列の添え字番号
//	int spriteIndex = 0;
//};
//
//void DebugText::Initialize(ID3D12Device* dev, int window_width, int window_height, UINT texnumber, const SpriteCommon& spriteCommon)
//{
//	// 全てのスプライトデータについて
//	for (int i = 0; i < _countof(sprites); i++)
//	{
//		// スプライトを生成する
//		sprites[i] = SpriteCreate(dev, window_width, window_height, texnumber, spriteCommon, { 0,0 });
//	}
//}
//
//void DebugText::Print(const SpriteCommon& spriteCommon, const std::string& text, float x, float y, float scale_)
//{
//	// 全ての文字について
//	for (int i = 0; i < text.size(); i++)
//	{
//		// 最大文字数超過
//		if (spriteIndex >= maxCharCount) {
//			break;
//		}
//
//		// 1文字取り出す(※ASCIIコードでしか成り立たない)
//		const unsigned char& character = text[i];
//
//		// ASCIIコードの2段分飛ばした番号を計算
//		int fontIndex = character - 32;
//		if (character >= 0x7f) {
//			fontIndex = 0;
//		}
//
//		int fontIndexY = fontIndex / fontLineCount;
//		int fontIndexX = fontIndex % fontLineCount;
//
//		// 座標計算
//		sprites[spriteIndex].position_ = { x + fontWidth * scale_ * i, y, 0 };
//		sprites[spriteIndex].texLeftTop = { (float)fontIndexX * fontWidth, (float)fontIndexY * fontHeight };
//		sprites[spriteIndex].texSize = { fontWidth, fontHeight };
//		sprites[spriteIndex].size = { fontWidth * scale_, fontHeight * scale_ };
//		// 頂点バッファ転送
//		SpriteTransferVertexBuffer(sprites[spriteIndex], spriteCommon);
//		// 更新
//		SpriteUpdate(sprites[spriteIndex], spriteCommon);
//
//		// 文字を１つ進める
//		spriteIndex++;
//	}
//}
//
//// まとめて描画
//void DebugText::DrawAll(ID3D12GraphicsCommandList* cmdList, const SpriteCommon& spriteCommon, ID3D12Device* dev)
//{
//	// 全ての文字のスプライトについて
//	for (int i = 0; i < spriteIndex; i++)
//	{
//		// スプライト描画
//		SpriteDraw(sprites[i], cmdList, spriteCommon, dev);
//	}
//
//	spriteIndex = 0;
//}

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
	DirectXCommon* dxCommon = nullptr;

#pragma region WindowsAPI初期化

	//windowsAPIの初期化
	winApp = new WinApp();
	winApp->Initialize();


	MSG msg{};  // メッセージ
#pragma endregion WindowsAPI初期化

#pragma region DirectX初期化処理
	//DirectXの初期化
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

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


	// スプライト共通部分の初期化
	SpriteCommon* spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon->GetDev(), dxCommon->GetCmdList(), winApp->windows_width, winApp->windows_height);



	/*const int SPRITES_NUM = 1;
	Sprite sprites[SPRITES_NUM];*/


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

	//3Dオブジェクト静的初期化
	Object3d::StaticInitialize(dxCommon->GetDev(), WinApp::windows_width, WinApp::windows_height);


	// DirectX初期化処理　ここまで
#pragma endregion DirectX初期化処理

#pragma region 描画初期化処理

	//OBJからモデルデータを読み込む
	Model* model_1 = Model::LoadFromOBJ("ground");
	Model* model_2 = Model::LoadFromOBJ("triangle_mat");
	//3Dオブジェクト生成
	Object3d* object3d_1 = Object3d::Create();
	Object3d* object3d_2 = Object3d::Create();
	Object3d* object3d_3 = Object3d::Create();
	//3Dオブジェクトに3Dモデルをひもづけ
	object3d_1->SetModel(model_1);
	object3d_2->SetModel(model_2);
	object3d_3->SetModel(model_2);

	object3d_2->SetScale({ 20.0f, 20.0f, 20.0f });
	object3d_3->SetScale({ 30.0f, 30.0f, 30.0f });


	object3d_2->SetPosition({ -5, 0, -5 });
	object3d_3->SetPosition({ +5, 0, +5 });

	// スプライト共通データ生成
	//spriteCommon = SpriteCommonCreate(dxCommon->GetDev(), WinApp::windows_width, WinApp::windows_height);
	// スプライト共通テクスチャ読み込み
	spriteCommon->LoadTexture(0, L"Resources/texture.png");
	spriteCommon->LoadTexture(1, L"Resources/house.png");

	Sprite* sprite = new Sprite();
	sprite->Initialize(spriteCommon, 0);

	// スプライトの生成
	//for (int i = 0; i < _countof(sprites); i++)
	//{
	//	int texNumber = rand() % 2;
	//	sprites[i] = SpriteCreate(dxCommon->GetDev(), WinApp::windows_width, WinApp::windows_height, texNumber, spriteCommon, { 0,0 }, false, false);

	//	// スプライトの座標変更
	//	sprites[i].position_.x = 1280 / 2;
	//	sprites[i].position_.y = 720 / 2;
	//	//sprites[i].isInvisible = true;
	//	//sprites[i].position.x = (float)(rand() % 1280);
	//	//sprites[i].position.y = (float)(rand() % 720);

	//	//sprites[i].rotation = (float)(rand() % 360);
	//	//sprites[i].rotation = 0;

	//	//sprites[i].size.x = 400.0f;
	//	//sprites[i].size.y = 100.0f;
	//	// 頂点バッファに反映
	//	SpriteTransferVertexBuffer(sprites[i], spriteCommon);
	//}

	//// デバッグテキスト
	//DebugText debugText;

	//// デバッグテキスト用のテクスチャ番号を指定
	//const int debugTextTexNumber = 2;
	//// デバッグテキスト用のテクスチャ読み込み
	//SpriteCommonLoadTexture(spriteCommon, debugTextTexNumber, L"Resources/debugfont.png", dxCommon->GetDev());
	//// デバッグテキスト初期化
	//debugText.Initialize(dxCommon->GetDev(), WinApp::windows_width, WinApp::windows_height, debugTextTexNumber, spriteCommon);


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

		float clearColor[] = { 0.1f,0.25f, 0.5f,0.0f }; // 青っぽい色

		if (input->PushKey(DIK_SPACE))     // スペースキーが押されていたら
		{
			// 画面クリアカラーの数値を書き換える
			clearColor[1] = 1.0f;
		}

		// 座標操作
		if (input->PushKey(DIK_UP) || input->PushKey(DIK_DOWN) || input->PushKey(DIK_RIGHT) || input->PushKey(DIK_LEFT))
		{
			
		}


		if (input->PushKey(DIK_D) || input->PushKey(DIK_A))
		{

		}

		//3Dオブジェクト更新
		object3d_1->Update();
		object3d_2->Update();
		object3d_3->Update();

		// DirectX毎フレーム処理　ここまで
#pragma endregion DirectX毎フレーム処理

#pragma region グラフィックスコマンド
		//描画前処理
		dxCommon->PreDraw();

		//3Dオブジェクト描画前処理
		Object3d::PreDraw(dxCommon->GetCmdList());

		//3Dオブジェクトの描画
		object3d_1->Draw();
		object3d_2->Draw();
		object3d_3->Draw();

		//3Dオブジェクトの描画後処理
		Object3d::PostDraw();

		// スプライト共通コマンド
		spriteCommon->PreDraw();

		//// スプライト描画
		//for (int i = 0; i < _countof(sprites); i++)
		//{
		//	SpriteDraw(sprites[i], dxCommon->GetCmdList(), spriteCommon, dxCommon->GetDev());
		//}
		//// デバッグテキスト描画
		//debugText.DrawAll(dxCommon->GetCmdList(), spriteCommon, dxCommon->GetDev());

		// ４．描画コマンドここまで

		//描画後処理
		dxCommon->PostDraw();
	}
	//スプライト共通部解放
	delete spriteCommon;

	//3Dオブジェクト解放
	delete model_1;
	delete model_2;

	//3Dオブジェクト解放
	delete object3d_1;
	delete object3d_2;
	delete object3d_3;

	// XAudio2解放
	xAudio2.Reset();
	// 音声データ解放
	SoundUnload(&soundData1);
	//windowsAPIの終了処理
	winApp->Finalize();

	//DirectX解放
	delete dxCommon;

	//入力解放
	delete input;

	//WindowsAPI解放
	delete winApp;
	winApp = nullptr;

	return 0;
}