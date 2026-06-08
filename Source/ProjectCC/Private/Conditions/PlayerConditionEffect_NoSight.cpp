// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_NoSight.h"
#include "Player_Character.h"

void UPlayerConditionEffect_NoSight::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer)
{
	if (!Player) return;
	Player->Client_StartAdditionalImage(1);
}

void UPlayerConditionEffect_NoSight::EndEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData)
{
	
}

void UPlayerConditionEffect_NoSight::EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect)
{
	if (!Player) return;
	Player->Client_EndAdditionalImage();

	Super::EndFunction(Player, ConditionComp, ConditionData, bUseEndEffect);
}

