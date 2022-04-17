#include <vector>
#include <string>
#include <fstream>
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")

#include "input.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "Object3d.h"
#include "Model.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "DebugText.h"

using namespace Microsoft::WRL;

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

	// スプライト共通テクスチャ読み込み
	spriteCommon->LoadTexture(0, L"Resources/texture.png");
	spriteCommon->LoadTexture(1, L"Resources/house.png");

	std::vector<Sprite*> sprites;

	// スプライトの生成
	for (int i = 0; i < 20; i++)
	{
		int texNumber = 0;
		 Sprite* sprite = Sprite::Create(spriteCommon, texNumber, { 0,0 }, false, false);

		// スプライトの座標変更
		sprite->SetPosition({ (float)(rand() % 1280), (float)(rand() % 720), 0 });
		sprite->SetRotation((float)(rand() % 360));
		sprite->SetSize({ (float)(rand() % 400), (float)(rand() % 400) });
		//sprites[i].isInvisible = true;
		//sprites[i].size.x = 400.0f;
		//sprites[i].size.y = 100.0f;
		// 頂点バッファに反映
		sprite->TransferVertexBuffer();

		sprites.push_back(sprite);
	}

	// デバッグテキスト
	DebugText* debugText = nullptr;
	debugText = new DebugText();

	// デバッグテキスト用のテクスチャ番号を指定
	const int debugTextTexNumber = 2;
	// デバッグテキスト用のテクスチャ読み込み
	spriteCommon->LoadTexture(debugTextTexNumber, L"Resources/debugfont.png");
	// デバッグテキスト初期化
	debugText->Initialize(spriteCommon, debugTextTexNumber);


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

		debugText->Print("Hello,DirectX!!", 200, 100);
		debugText->Print("Nihon Kogakuin", 200, 200, 2.0f);

		//3Dオブジェクト更新
		object3d_1->Update();
		object3d_2->Update();
		object3d_3->Update();

		//スプライト更新
		for (auto& sprite : sprites) {
			sprite->Update();
		}
	
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

		for (auto& sprite : sprites) {
			sprite->Draw();
		}

		// デバッグテキスト描画
		debugText->DrawAll();

		// ４．描画コマンドここまで

		//描画後処理
		dxCommon->PostDraw();
	}

	// デバッグテキスト解放
	delete debugText;

	// スプライト解放
	for (auto& sprite : sprites) {
		delete sprite;
	}
	sprites.clear();

	// スプライト共通部解放
	delete spriteCommon;

	// 3Dオブジェクト解放
	delete model_1;
	delete model_2;

	// 3Dオブジェクト解放
	delete object3d_1;
	delete object3d_2;
	delete object3d_3;

	// XAudio2解放
	xAudio2.Reset();
	// 音声データ解放
	SoundUnload(&soundData1);
	// windowsAPIの終了処理
	winApp->Finalize();

	//DirectX解放
	delete dxCommon;

	// 入力解放
	delete input;

	// WindowsAPI解放
	delete winApp;
	winApp = nullptr;

	return 0;
}