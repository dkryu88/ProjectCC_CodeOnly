// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerConditionEffect.h"
#include "PlayerConditionEffect_Magnetic.generated.h"

/**
 * 
 */

class ACoin;

UCLASS()
class PROJECTCC_API UPlayerConditionEffect_Magnetic : public UPlayerConditionEffect
{
	GENERATED_BODY()
	
protected:
	virtual void StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer) override;
	virtual void PersistEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, float DeltaTime) override;
	virtual void EndEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData) override;
	virtual void EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect) override;
private:
	void MagneticSweep(APlayer_Character* Player);

	TSet<TWeakObjectPtr<ACoin>> CapturedCoins;
	TMap<TWeakObjectPtr<ACoin>, float>CoinCaptureTimes;

	float ScanTimer = 0.f;
	float MagneticRadius = 500.f;
};
