// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerConditionEffect.h"

//각 상태이상이 상속받아 override
void UPlayerConditionEffect::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer)
{
}

void UPlayerConditionEffect::PersistEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, float DeltaTime)
{
}

void UPlayerConditionEffect::EndEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData)
{
}

void UPlayerConditionEffect::ResumeEffectVisual(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData)
{
}

void UPlayerConditionEffect::EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect)
{
	if (bUseEndEffect) {
		EndEffect(Player, ConditionComp, ConditionData);
	}
}
