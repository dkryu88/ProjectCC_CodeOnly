// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Match_Event.h"
#include "Match_Event_Meteor.generated.h"

/**
 *
 */
class AMapObjects_Meteor;

UCLASS()
class PROJECTCC_API AMatch_Event_Meteor : public AMatch_Event
{
	GENERATED_BODY()

public:
	virtual void StartEvent_Implementation(AMapConstructor* Map, APlayMode_Match* InMatchMode, float duration) override;
	virtual void StopEvent_Implementation() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Meteor")
	TSubclassOf<AMapObjects_Meteor> MapObjects_Meteor;

	UPROPERTY(EditDefaultsOnly, Category = "Meteor|Warning")
	TSubclassOf<AActor> MeteorWarningClass;

	UPROPERTY(EditDefaultsOnly, Category = "Meteor|Warning")
	float WarningDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Meteor|Warning")
	float WarningZOffset = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Meteor")
	float MeteorSpawnHeightByBlock = 30.f;

	bool bMeteorEventRunning = false;

	//스폰 간격마다 소환할 운석 수
	UPROPERTY(EditDefaultsOnly, Category = "Meteor")
	int32 SpawnCountPerInterval = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Meteor")
	float SpawnInterval = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Metero")
	float NowDuration = 30.f;

	FTimerHandle EventDurationTimerHandle;
	FTimerHandle MeteorTimerHandle;

	void SpawnMeteor();
	void SpawnMeteorWarningActor(const FVector& TargetTopLocation);
	void SpawnMeteorAtTargetTop(const FVector& TargetTopLocation);
};
