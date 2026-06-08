// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_Stun.h"
#include "Player_Character.h"
#include "PlayerConditions.h"

void UPlayerConditionEffect_Stun::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer) {
	if (Player) {
		Player->AddInputBlockController(FName("Stun"), true, false, true, false);
		if (ConditionData.ConditionMontage) {
			Player->Multicast_PlayOverrideMontage(ConditionData.ConditionMontage);
		}

		UE_LOG(LogTemp, Warning, TEXT("Stunned"));
	}
}

void UPlayerConditionEffect_Stun::EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect)
{
	if (Player) {
		Player->RemoveInputBlockController(FName("Stun"));
		if (ConditionData.ConditionMontage) {
			Player->Multicast_StopMontage(ConditionData.ConditionMontage, 0.15f);
		}
	}

	Super::EndFunction(Player, ConditionComp, ConditionData, bUseEndEffect);
}