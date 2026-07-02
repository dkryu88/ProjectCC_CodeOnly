// Fill out your copyright notice in the Description page of Project Settings.


#include "Sound/AllPlayMode_SoundSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundDataAsset.h"
#include "AllPlayMode_GameInstance.h"


void UAllPlayMode_SoundSubsystem::Deinitialize(){
	StopBGM(0.f);
	Super::Deinitialize();
}

void UAllPlayMode_SoundSubsystem::InitializeAudioData(USoundDataAsset* NewSoundAsset)
{
	AudioData = NewSoundAsset;
}

void UAllPlayMode_SoundSubsystem::PlayBGMByMapName(FName MapName, float FadeInTime)
{
	USoundDataAsset* audioData = GetAudioData();
	if (!audioData) return;
	if (USoundBase** FoundBGM = audioData->MatchBGM_Map.Find(MapName)) {
		PlayBGM(*FoundBGM, FadeInTime);
	}
}

//[추가] 매치 종료 30초 전 사용할 BGM 재생함수
void UAllPlayMode_SoundSubsystem::PlayBGM30SecByMapName(FName MapName, float FadeInTime)
{
	USoundDataAsset* audioData = GetAudioData();
	if (!audioData) return;
	if (USoundBase** FoundBGM30Sec = audioData->MatchBGM30Sec_Map.Find(MapName)) {
		PlayBGM(*FoundBGM30Sec, FadeInTime);
	}

}

void UAllPlayMode_SoundSubsystem::PlayBGM(USoundBase* BGMSound, float FadeInTime)
{
	if (!BGMSound || !GetWorld()) return;
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying() && CurrentBGMComponent->GetSound() == BGMSound) return;

	if (CurrentBGMComponent) {
		CurrentBGMComponent->Stop();
	}
	CurrentBGMComponent = UGameplayStatics::CreateSound2D(GetWorld(), BGMSound, OriginalBGMVolume, 1.f, 0.f, nullptr, true, false);

	if (CurrentBGMComponent) {
		CurrentBGMComponent->FadeIn(FadeInTime, OriginalBGMVolume);
	}
}

void UAllPlayMode_SoundSubsystem::StopBGM(float FadeOutTime)
{
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying()) {
		CurrentBGMComponent->FadeOut(FadeOutTime, 0.f);
		CurrentBGMComponent->bAutoDestroy = true;
	}

	CurrentBGMComponent = nullptr;
}

void UAllPlayMode_SoundSubsystem::SetBGMPitch(float NewPitch)
{
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying()) {
		CurrentBGMComponent->SetPitchMultiplier(NewPitch);
	}
}

void UAllPlayMode_SoundSubsystem::StartDucking(float FadeTime, float TargetVolume)
{
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying()) {
		CurrentBGMComponent->AdjustVolume(FadeTime, TargetVolume);
	}
}

void UAllPlayMode_SoundSubsystem::StopDucking(float FadeTime)
{
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying()) {
		CurrentBGMComponent->AdjustVolume(FadeTime, OriginalBGMVolume);
	}
}

void UAllPlayMode_SoundSubsystem::PlayOneShotSFX(USoundBase* SFXSound, float VolumeMultiplier)
{
	if (!SFXSound || !GetWorld()) return;
	UGameplayStatics::PlaySound2D(GetWorld(), SFXSound, VolumeMultiplier);
}

void UAllPlayMode_SoundSubsystem::PlayManagedSFX(USoundBase* SFXSound, float VolumeMultiplier)
{
	if (!SFXSound || !GetWorld()) return;
	// 중복 재생 방지
	StopManagedSFX(SFXSound);
	// 효과음 재생
	UAudioComponent* NewSFXComponent = UGameplayStatics::SpawnSound2D(GetWorld(), SFXSound, VolumeMultiplier);
	// 맵에 추가
	if (NewSFXComponent) {
		ActiveSFXMap.Add(SFXSound, NewSFXComponent);
		NewSFXComponent->OnAudioFinishedNative.AddWeakLambda(this, [this, SFXSound](UAudioComponent* AudioComp) {
			ActiveSFXMap.Remove(SFXSound);
		});
	}
}

void UAllPlayMode_SoundSubsystem::StopManagedSFX(USoundBase* SFXSound)
{
	if (!SFXSound) return;
	if (UAudioComponent** FoundComponent = ActiveSFXMap.Find(SFXSound)) {
		if (*FoundComponent && (*FoundComponent)->IsPlaying()) {
			(*FoundComponent)->Stop();
		}
		ActiveSFXMap.Remove(SFXSound);
	}
}

USoundDataAsset* UAllPlayMode_SoundSubsystem::GetAudioData() {
	if (AudioData) return AudioData;
	if (UGameInstance* GI = GetGameInstance()) {
		if (UAllPlayMode_GameInstance* AllPlayGI = Cast<UAllPlayMode_GameInstance>(GI)) {
			AudioData = AllPlayGI->AllSoundData;
		}
	}
	return AudioData;
}

//[클릭]
void UAllPlayMode_SoundSubsystem::PlayUIClickSound()
{
	// 안전망을 통해 데이터 에셋을 가져오고, 클릭음이 존재하면 1회성으로 재생합니다.
	if (USoundDataAsset* DataAsset = GetAudioData()) {
		if (DataAsset->UIClickSound) {
			PlayOneShotSFX(DataAsset->UIClickSound);
		}
	}
}
