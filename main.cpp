#include <vector>
#include <string>

#include "input.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "Object3d.h"
#include "Model.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "DebugText.h"
#include "Audio.h"

#include "FbxLoader.h"

using namespace Microsoft::WRL;


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//ポインタ置き場
	Input* input = nullptr;
	WinApp* winApp = nullptr;
	DirectXCommon* dxCommon = nullptr;
	DebugText* debugText = nullptr;
	Audio* audio = nullptr;

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


	// スプライト共通部分の初期化
	SpriteCommon* spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon->GetDev(), dxCommon->GetCmdList(), winApp->windows_width, winApp->windows_height);

	//デバッグテキスト
	debugText = new DebugText();
	// デバッグテキスト用のテクスチャ番号を指定
	const int debugTextTexNumber = 2;
	// デバッグテキスト用のテクスチャ読み込み
	spriteCommon->LoadTexture(debugTextTexNumber, L"Resources/debugfont.png");
	// デバッグテキスト初期化
	debugText->Initialize(spriteCommon, debugTextTexNumber);


	//入力の初期化
	input = new Input();
	input->Initialize(winApp);

	//オーディオの初期化
	audio = new Audio();
	audio->Initialize();

	// 音声読み込み
	audio->LoadWave("Alarm01.wav");
	// 音声再生
	audio->PlayWave("Alarm01.wav");

	//3Dオブジェクト静的初期化
	Object3d::StaticInitialize(dxCommon->GetDev(), WinApp::windows_width, WinApp::windows_height);

	FbxLoader::GetInstance()->Initialize(dxCommon->GetDev());

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

	//// 音声読み込み
	//audio->LoadWave("Alarm01.wav");
	//// 音声再生
	//audio->PlayWave("Alarm01.wav");

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

	//// デバッグテキスト
	//debugText = new DebugText();


	// デバッグテキスト用のテクスチャ番号を指定
	const int debugTextTexNumber = 2;
	// デバッグテキスト用のテクスチャ読み込み
	spriteCommon->LoadTexture(debugTextTexNumber, L"Resources/debugfont.png");
	// デバッグテキスト初期化
	debugText->Initialize(spriteCommon, debugTextTexNumber);
	// モデル名指定してファイル読み込み
	FbxLoader::GetInstance()->LoadModelFromFile("cube");

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
		/*object3d_1->Draw();
		object3d_2->Draw();
		object3d_3->Draw();*/

		//3Dオブジェクトの描画後処理
		Object3d::PostDraw();

		// スプライト共通コマンド
		spriteCommon->PreDraw();

		for (auto& sprite : sprites) {
			/*sprite->Draw();*/
		}

		// デバッグテキスト描画
		/*debugText->DrawAll();*/

		// ４．描画コマンドここまで

		//描画後処理
		dxCommon->PostDraw();
	}

	FbxLoader::GetInstance()->Finalize();

	// デバッグテキスト解放
	debugText->Finalize();
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
	//オーディオ解放
	audio->Finalize();
	delete audio;

	//windowsAPIの終了処理
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