// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AllPlayMode_SoundSubsystem.generated.h"

/**
 * 
 */
class USoundBase;
class UAudioComponent;
class USoundDataAsset;

UCLASS()
class PROJECTCC_API UAllPlayMode_SoundSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	void InitializeAudioData(USoundDataAsset* NewSoundAsset);
	void PlayBGMByMapName(FName MapName, float FadeInTime = 1.f);
	//[추가]
	void PlayBGM30SecByMapName(FName MapName, float FadeInTime = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void PlayBGM(USoundBase* BGMSound, float FadeInTime = 1.f);
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void StopBGM(float FadeOutTime = 1.f);
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void SetBGMPitch(float NewPitch);
	UFUNCTION(BlueprintCallable, Category = "Audio|Ducking")
	void StartDucking(float FadeTime = 0.5f, float TargetVolume = 0.3f);
	UFUNCTION(BlueprintCallable, Category = "Audio|Ducking")
	void StopDucking(float FadeTime = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayOneShotSFX(USoundBase* SFXSound, float VolumeMultiplier = 1.f);
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayManagedSFX(USoundBase* SFXSound, float VolumeMultiplier = 1.f);
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void StopManagedSFX(USoundBase* SFXSound);

	USoundDataAsset* GetAudioData();

	//[클릭]
	UFUNCTION(BlueprintCallable, Category = "Audio|UI")
	void PlayUIClickSound();

protected:
	UPROPERTY()
	TObjectPtr<USoundDataAsset> AudioData;

	// 현재 재생중인 BGM컴포넌트
	UPROPERTY()
	TObjectPtr<UAudioComponent> CurrentBGMComponent;

	// 관리형 SFX Map
	UPROPERTY()
	TMap<USoundBase*, UAudioComponent*> ActiveSFXMap;

	// 더킹 후 복구 할 볼륨값
	float OriginalBGMVolume = 1.f;
};
