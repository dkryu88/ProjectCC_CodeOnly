// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Match_Event.h"
#include "Match_Event_RandomTeleport.generated.h"

/**
 * 
 */
class UPlayerTransformationDataAsset;
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AMatch_Event_RandomTeleport : public AMatch_Event
{
	GENERATED_BODY()
	
public:
	virtual void StartEvent_Implementation(AMapConstructor* Map, APlayMode_Match* InMatchMode, float duration) override;
	virtual void StopEvent_Implementation() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	float TeleportInterval = 15.f;

	//은신 데이터 에셋
	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	TObjectPtr<UPlayerTransformationDataAsset> StealthTransformationData;

	//무적 효과 데이터 에셋
	UPROPERTY(EditAnywhere, Category = "ConditionData")
	TObjectPtr<UPlayerConditionDataAsset> InvincibilityDataAsset;

	FTimerHandle TeleportTimerHandle;
	FTimerHandle EventDurationTimerHandle;

	void TeleportAllPlayers();
	bool FindRandomLocation(FVector& OutLocation);
};
