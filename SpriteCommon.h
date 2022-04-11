#pragma once

#include "PipelineSet.h"

#include <DirectXTex.h>

/// <summary>
/// �X�v���C�g���ʕ���
/// </summary>

class SpriteCommon
{
public:	//�v���W�F�N�g�S�̂Ŏg����

	// �e�N�X�`���̍ő喇��
	static const int kSpriteSRVCount = 512;

	/// <summary>
	/// ������
	/// </summary>
	void Initialize(ID3D12Device* device);

private:	//�錾�݂̂Ŏg����
	// �p�C�v���C���Z�b�g
	PipelineSet pipelineSet_;
	// �ˉe�s��
	DirectX::XMMATRIX matProjection_{};
	// �e�N�X�`���p�f�X�N���v�^�q�[�v�̐���
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descHeap_;
	// �e�N�X�`�����\�[�X�i�e�N�X�`���o�b�t�@�j�̔z��
	Microsoft::WRL::ComPtr<ID3D12Resource> texBuff_[kSpriteSRVCount];
	//�f�o�C�X�i�؂�Ă���j
	ID3D12Device* device_ = nullptr;
private:
	// �p�C�v���C������
	void CreateGraphicsPipeline();
};

