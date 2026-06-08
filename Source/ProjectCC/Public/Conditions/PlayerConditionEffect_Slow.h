// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerConditionEffect.h"
#include "PlayerConditionEffect_Slow.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API UPlayerConditionEffect_Slow : public UPlayerConditionEffect
{
	GENERATED_BODY()
	
public:

	virtual void StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer)override;
	virtual void EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect) override;
};
