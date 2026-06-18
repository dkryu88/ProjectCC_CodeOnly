// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AudioManagerSubsystem.generated.h"

class USoundBase;
class UAudioComponent;
class UGameAudioDataAsset;
	/**
	 *
	 */
	UCLASS()
	class PROJECTCC_API UAudioManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ============================시스템============================
	//게임인스턴스로부터 데이터를 념겨받는 함수
	void InitializeAudioData(UGameAudioDataAsset* NewDataAsset);

	// 서브시스템이 파괴될 때 실행(메모리 정리)
	virtual void Deinitialize() override;


	// ============================BGM============================
	// BGM 재생(Match_Level에서 맵이름을 찾아서 해당하는 BGM 재생)
	void PlayBGMByMapName(FName MapName, float FadeInTime = 1.f);

	// BGM 재생
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void PlayBGM(USoundBase* BGMSound, float FadeInTime = 1.f);

	// BGM 정지
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void StopBGM(float FadeOutTime = 1.f);

	// BGM 재생속도 조절
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void SetBGMPitch(float NewPitch);

	// BGM 더킹 시작
	UFUNCTION(BlueprintCallable, Category = "Audio|Ducking")
	void StartDucking(float FadeTime = 0.5f, float TargetVolume = 0.3f);

	// BGM 더킹 종료
	UFUNCTION(BlueprintCallable, Category = "Audio|Ducking")
	void StopDucking(float FadeTime = 0.5f);


	// ============================SFX============================
	// 재생을 끊을 필요가 없는 1회성 재생 SFX 재생 함수(매치카운트다운 효과음 등)
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayOneShotSFX(USoundBase* SFXSound, float VolumeMultiplier = 1.f);

	//TMap에 등록되어 멈출 수 있는 관리형 효과음 재생함수(이벤트경고음 등)
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayManagedSFX(USoundBase* SFXSound, float VolumeMultiplier = 1.f);
	//TMap에 등록된 SFX를 정지하는 함수
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void StopManagedSFX(USoundBase* SFXSound);

	//효과음 함수들에서 에셋을 편하게 쓸 수 있도록 헬퍼 함수 추가
	//UGameAudioDataAsset* GetAudioData() const { return AudioData; }	//기존코드->타이밍이슈로 InitializeAudioData함수에서 AduioData를 받아오지 못하는 버그 발생
	UGameAudioDataAsset* GetAudioData();								//변경코드->지연초기화를 통해 무조건 데이터에셋을 받아오도록 변경

private:
	UPROPERTY()
	TObjectPtr<UGameAudioDataAsset> AudioData;

	// 현재 재생중인 BGM컴포넌트
	UPROPERTY()
	TObjectPtr<UAudioComponent> CurrentBGMComponent;

	// 관리형 SFX Map
	UPROPERTY()
	TMap<USoundBase*, UAudioComponent*> ActiveSFXMap;

	// 더킹 후 복구 할 볼륨값
	float OriginalBGMVolume = 1.f;
};
