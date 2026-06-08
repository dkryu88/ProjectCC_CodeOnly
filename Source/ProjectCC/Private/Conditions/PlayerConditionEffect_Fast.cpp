// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_Fast.h"
#include "Player_Character.h"
#include "PlayerConditionDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"

void UPlayerConditionEffect_Fast::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer)
{
	if (Player) Player->AddSpeedController(TEXT("Fast"), ConditionData.EffectValue, 0.f);
}

void UPlayerConditionEffect_Fast::EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect)
{
	if (Player) Player->RemoveSpeedControllerByName(TEXT("Fast"));

	Super::EndFunction(Player, ConditionComp, ConditionData, bUseEndEffect);
}
