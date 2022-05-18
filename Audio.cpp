#include "Audio.h"

#include <windows.h>
#include <cassert>
#pragma comment(lib,"xaudio2.lib")

void Audio::Initialize()
{
	HRESULT result;

	IXAudio2MasteringVoice* masterVoice;

	// XAudioエンジンのインスタンスを生成
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));

	// マスターボイスを生成
	result = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));
}

void Audio::Finalize()
{
	// XAudio2解放
	xAudio2.Reset();
	// 音声データ解放
	//SoundUnload(&soundData1);
}
