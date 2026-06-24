// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_Stun.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "PlayerConditions.h"

void UPlayerConditionEffect_Stun::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer) {
	if (Player) {
		Player->AddInputBlockController(FName("Stun"), true, false, true, false);
		if (ConditionData.ConditionAnimation) {
			FName SlotName = Player->GetAnimationSlot(Player);
			Player->Multicast_PlayAnimationDynamic(ConditionData.ConditionAnimation, SlotName, 0.05f, 0.05f, 1.f, 999999, 0);
			UE_LOG(LogTemp, Warning, TEXT("Playing Stun Animation"));
		}

		UE_LOG(LogTemp, Warning, TEXT("Stunned"));
	}
}

void UPlayerConditionEffect_Stun::EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect)
{
	if (Player) {
		Player->RemoveInputBlockController(FName("Stun"));
		if (ConditionData.ConditionAnimation) {
			FName SlotName = Player->GetAnimationSlot(Player);
			Player->Multicast_StopSlotAnimation(SlotName, 0.15f);
		}
	}

	Super::EndFunction(Player, ConditionComp, ConditionData, bUseEndEffect);
}