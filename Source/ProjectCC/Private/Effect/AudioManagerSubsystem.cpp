// Fill out your copyright notice in the Description page of Project Settings.


#include "Effect/AudioManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Effect/GameAudioDataAsset.h"
#include "AllPlayMode_GameInstance.h"

void UAudioManagerSubsystem::InitializeAudioData(UGameAudioDataAsset* NewDataAsset)
{// 서브시스템이 게임인스턴스에 참조해놓은 데이터에셋을 받아와 직접 소유
	AudioData = NewDataAsset;
}

void UAudioManagerSubsystem::Deinitialize()
{
	StopBGM(0.f);
	Super::Deinitialize();
}

UGameAudioDataAsset* UAudioManagerSubsystem::GetAudioData()
{
	// 1. 만약 데이터가 이미 잘 들어있다면 그대로 반환
	if (AudioData) return AudioData;

	// 2. 만약 빈손(nullptr)이라면? GameInstance로 찾아가서 직접 가져옴 (지연 초기화 안전망)
	if (UGameInstance* GI = GetGameInstance()) {
		if (UAllPlayMode_GameInstance* AllPlayGI = Cast<UAllPlayMode_GameInstance>(GI)) {
			AudioData = AllPlayGI->AllAudioData;
		}
	}
	return AudioData;
}

void UAudioManagerSubsystem::PlayBGMByMapName(FName MapName, float FadeInTime)
{
	UGameAudioDataAsset* _AudioData = GetAudioData();
	if (!_AudioData) return;
	if (USoundBase** FoundBGM = _AudioData->MatchBGM_Map.Find(MapName)) {
		PlayBGM(*FoundBGM, FadeInTime);
	}
}


void UAudioManagerSubsystem::PlayBGM(USoundBase* BGMSound, float FadeInTime)
{
	if (!BGMSound || !GetWorld()) return;
	// 같은 BGM을 재생중이면 리턴
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying() && CurrentBGMComponent->GetSound() == BGMSound) return;

	// 기존 BGM 끄기
	if (CurrentBGMComponent) {
		CurrentBGMComponent->Stop();
	}

	// BGM 플레이어 생성(재생 아직 안함)
	CurrentBGMComponent = UGameplayStatics::CreateSound2D(GetWorld(), BGMSound, OriginalBGMVolume, 1.f, 0.f, nullptr, true, false);

	// FadeIn으로 재생시작(실질적인 재생 함수)
	if (CurrentBGMComponent) {
		CurrentBGMComponent->FadeIn(FadeInTime, OriginalBGMVolume);
	}
}

void UAudioManagerSubsystem::StopBGM(float FadeOutTime)
{//FadeOut으로 재생 종료
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying()) {
		CurrentBGMComponent->FadeOut(FadeOutTime, 0.f);
		CurrentBGMComponent->bAutoDestroy = true;
	}
	CurrentBGMComponent = nullptr;
}


void UAudioManagerSubsystem::SetBGMPitch(float NewPitch)
{//재생속도 조절
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying()) {
		CurrentBGMComponent->SetPitchMultiplier(NewPitch);
	}
}

void UAudioManagerSubsystem::StartDucking(float FadeTime, float TargetVolume)
{//볼륨 조절
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())	{
		CurrentBGMComponent->AdjustVolume(FadeTime, TargetVolume);
	}
}

void UAudioManagerSubsystem::StopDucking(float FadeTime)
{//볼륨 원복
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())	{
		CurrentBGMComponent->AdjustVolume(FadeTime, OriginalBGMVolume);
	}
}

void UAudioManagerSubsystem::PlayOneShotSFX(USoundBase* SFXSound, float VolumeMultiplier)
{
	if (!SFXSound || !GetWorld()) return;
	UGameplayStatics::PlaySound2D(GetWorld(), SFXSound, VolumeMultiplier);
}

void UAudioManagerSubsystem::PlayManagedSFX(USoundBase* SFXSound, float VolumeMultiplier)
{
	if (!SFXSound || !GetWorld()) return;
	// 중복 재생 방지
	StopManagedSFX(SFXSound);

	// 효과음 재생
	UAudioComponent* NewSFXComponent = UGameplayStatics::SpawnSound2D(GetWorld(), SFXSound, VolumeMultiplier);
	
	// 맵에 추가
	if (NewSFXComponent) {
		ActiveSFXMap.Add(SFXSound, NewSFXComponent);
		//메모리 누수 방지 - 재생이 끝났을 때 스스로 TMap에서 지우는 람다함수 예약
		NewSFXComponent->OnAudioFinishedNative.AddWeakLambda(this, [this, SFXSound](UAudioComponent* AudioComp) {
			ActiveSFXMap.Remove(SFXSound);
			});
	}
}

void UAudioManagerSubsystem::StopManagedSFX(USoundBase* SFXSound)
{
	if (!SFXSound) return;
	if (UAudioComponent** FoundComponent = ActiveSFXMap.Find(SFXSound)) {
		if (*FoundComponent && (*FoundComponent)->IsPlaying()) {
			(*FoundComponent)->Stop();
		}
		ActiveSFXMap.Remove(SFXSound);
	}
}

