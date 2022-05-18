#include "Audio.h"

#include <windows.h>
#include <cassert>
#pragma comment(lib,"xaudio2.lib")

void Audio::Initialize()
{
	HRESULT result;

	IXAudio2MasteringVoice* masterVoice;

	// XAudio�G���W���̃C���X�^���X�𐶐�
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));

	// �}�X�^�[�{�C�X�𐶐�
	result = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));
}

void Audio::Finalize()
{
	// XAudio2���
	xAudio2.Reset();
	// �����f�[�^���
	//SoundUnload(&soundData1);
}
