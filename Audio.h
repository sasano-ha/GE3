#pragma once
#include <cstdint>
#include <xaudio2.h>
#include <wrl.h>



/// <summary>
/// オーディオ
/// </summary>

class Audio
{
public:

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

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
};

