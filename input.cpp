#include "input.h"
#include <wrl.h>
#include <dinput.h>

#define DIRECTINPUT_VERSION 0x0800  //DirectInput�̃o�[�W�����w��

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Microsoft::WRL;

void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
    HRESULT result;

    //DirectInput�̃C���X�^���X����
    ComPtr<IDirectInput8> dinput = nullptr;
    result = DirectInput8Create(
        hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&dinput, nullptr);

    //�L�[�{�[�h�f�o�C�X����
    ComPtr<IDirectInputDevice8> devkeyboard = nullptr;
    result = dinput->CreateDevice(GUID_SysKeyboard, &devkeyboard, NULL);

    //���̓f�[�^�`���Z�b�g
    result = devkeyboard->SetDataFormat(&c_dfDIKeyboard); // �W���`��

    //�r���I���䃌�x���̃Z�b�g
    result = devkeyboard->SetCooperativeLevel(
        hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
}

void Input::Update()
{
}
