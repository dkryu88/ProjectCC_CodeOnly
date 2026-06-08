// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Match_Event.h"
#include "Match_Event_SpeedUp.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AMatch_Event_SpeedUp : public AMatch_Event
{
	GENERATED_BODY()
	
public:
	virtual void StartEvent_Implementation(AMapConstructor* Map, APlayMode_Match* InMatchMode, float duration) override;
	virtual void StopEvent_Implementation() override;
	virtual void ApplyEventToPlayer_Implementation(APlayer_Character* Player) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Speed")
	float SpeedMultiplier = 1.5f;

	FTimerHandle EventDurationTimerHandle;

	void ApplySpeedBoostToPlayer(APlayer_Character* Player, bool bApply);
	void ApplySpeedBoost(bool bApply);
};
