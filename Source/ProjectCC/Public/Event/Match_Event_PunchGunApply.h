// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Match_Event.h"
#include "Match_Event_PunchGunApply.generated.h"

/**
 * 
 */
class AWeapon_PunchGun;

UCLASS()
class PROJECTCC_API AMatch_Event_PunchGunApply : public AMatch_Event
{
	GENERATED_BODY()
public:
	virtual void StartEvent_Implementation(AMapConstructor* Map, APlayMode_Match* InMatchMode, float Duration) override;
	virtual void StopEvent_Implementation() override;
	virtual void ApplyEventToPlayer_Implementation(APlayer_Character* Player) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "HomeRun")
	TSubclassOf<AWeapon_PunchGun> PunchGunClass;

	void ApplyPunchGun(bool bApply);
};
