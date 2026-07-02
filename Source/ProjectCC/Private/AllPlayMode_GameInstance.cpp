// Fill out your copyright notice in the Description page of Project Settings.


#include "AllPlayMode_GameInstance.h"
#include "Sound/AllPlayMode_SoundSubsystem.h"

void UAllPlayMode_GameInstance::Init()
{
	Super::Init();

	//[사운드] 게임 실행 시 오디오 서브시스템을 불러와 데이터 넘겨줌
	if (UAllPlayMode_SoundSubsystem* AudioSub = GetSubsystem<UAllPlayMode_SoundSubsystem>()) {
		AudioSub->InitializeAudioData(AllSoundData);
	}
}

void UAllPlayMode_GameInstance::SetLocalPlayerNickname(const FString& NewNickName)
{
	LocalPlayerNickName = NewNickName;
	//앞뒤 공백 제거
	LocalPlayerNickName.TrimStartAndEndInline();
}

FString UAllPlayMode_GameInstance::GetPlayerLocalNickname()
{
	return LocalPlayerNickName;
}
