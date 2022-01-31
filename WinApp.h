#pragma once
#include <Windows.h>

//WindowsAPI
class WinApp
{
public:	//定数
	//ウィンドウ横幅
	static const int windows_width = 1280;
	//ウィンドウ縦幅
	static const int windows_height = 720;
public:	//静的メンバ関数
	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:	//メンバ関数
	//初期化
	void Initialize();
	//更新
	void Update();
	//終了
	void Finalize();
	//メッセージの処理
	bool ProcessMessage();
	//getter
	HWND GetHwnd() { return hwnd; }
	//getter
	HINSTANCE GetHInstance() { return w.hInstance; }
private:
	//ウィンドウハンドル
	HWND hwnd = nullptr;
	//ウィンドウクラスの設定
	WNDCLASSEX w{};
};