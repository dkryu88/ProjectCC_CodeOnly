// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameAudioDataAsset.generated.h"

class USoundBase;
/**
 * 
 */
UCLASS()
class PROJECTCC_API UGameAudioDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	USoundBase* TitleBGM;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	USoundBase* ReadyBGM;
	
	// 맵마다 다른 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	TMap<FName, USoundBase*> MatchBGM_Map;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	USoundBase* ResultBGM;

	// 이벤트 시작 전 경고음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX")
	USoundBase* EventWarningSound;
	// 매치 시작 시 카운트다운
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX")
	USoundBase* CountDownSound;
};
