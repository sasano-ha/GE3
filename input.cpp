#include "input.h"
#include <wrl.h>
#include <dinput.h>

#define DIRECTINPUT_VERSION 0x0800  //DirectInputのバージョン指定

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Microsoft::WRL;

void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
    HRESULT result;

    //DirectInputのインスタンス生成
    ComPtr<IDirectInput8> dinput = nullptr;
    result = DirectInput8Create(
        hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&dinput, nullptr);

    //キーボードデバイス生成
    ComPtr<IDirectInputDevice8> devkeyboard = nullptr;
    result = dinput->CreateDevice(GUID_SysKeyboard, &devkeyboard, NULL);

    //入力データ形式セット
    result = devkeyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式

    //排他的制御レベルのセット
    result = devkeyboard->SetCooperativeLevel(
        hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
}

void Input::Update()
{
}
