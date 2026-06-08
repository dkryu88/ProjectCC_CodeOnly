// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_Slow.h"
#include "Player_Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UPlayerConditionEffect_Slow::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer)
{
	if (Player) Player->AddSpeedController(TEXT("Slow"), ConditionData.EffectValue, 0.f);
}

void UPlayerConditionEffect_Slow::EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect)
{
	if (Player) Player->RemoveSpeedControllerByName(TEXT("Slow"));

	Super::EndFunction(Player, ConditionComp, ConditionData, bUseEndEffect);
}
