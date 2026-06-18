// Fill out your copyright notice in the Description page of Project Settings.


#include "AllPlayMode_GameInstance.h"
#include "Effect/AudioManagerSubsystem.h"	//[사운드]


void UAllPlayMode_GameInstance::Init()
{
	Super::Init();

	//[사운드] 게임 실행 시 오디오 서브시스템을 불러와 데이터 넘겨줌
	if (UAudioManagerSubsystem* AudioSub = GetSubsystem<UAudioManagerSubsystem>()) {
		AudioSub->InitializeAudioData(AllAudioData);
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
