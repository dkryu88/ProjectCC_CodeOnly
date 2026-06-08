// Fill out your copyright notice in the Description page of Project Settings.


#include "AllPlayMode_GameInstance.h"

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
