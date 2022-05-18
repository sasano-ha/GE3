#pragma once
#include <cstdint>
#include <xaudio2.h>
#include <wrl.h>



/// <summary>
/// �I�[�f�B�I
/// </summary>

class Audio
{
public:

	// �`�����N�w�b�_
	struct ChunkHeader
	{
		char id[4]; // �`�����N����ID
		int32_t size;  // �`�����N�T�C�Y
	};

	// RIFF�w�b�_�`�����N
	struct RiffHeader
	{
		ChunkHeader chunk;   // "RIFF"
		char type[4]; // "WAVE"
	};

	// FMT�`�����N
	struct FormatChunk
	{
		ChunkHeader chunk; // "fmt "
		WAVEFORMATEX fmt; // �g�`�t�H�[�}�b�g
	};


	// �����f�[�^
	struct SoundData
	{
		// �g�`�t�H�[�}�b�g
		WAVEFORMATEX wfex;
		// �o�b�t�@�̐擪�A�h���X
		BYTE* pBuffer;
		// �o�b�t�@�̃T�C�Y
		unsigned int bufferSize;
	};

	/// <summary>
	/// ������
	/// </summary>
	void Initialize();

	/// <summary>
	/// �I������
	/// </summary>
	void Finalize();

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
};

