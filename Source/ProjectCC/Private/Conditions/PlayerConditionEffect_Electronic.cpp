// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_Electronic.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "PlayerConditions.h"

void UPlayerConditionEffect_Electronic::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer)
{
	if (!Player) return;

	Player->AddInputBlockController(FName("Electronic"), true, false, true, false);

	ConditionData.NextEffectTimer = ConditionData.EffectInterval;
	ConditionData.RemainingTickCount = ConditionData.EffectCount;

	if (ConditionData.ConditionMontage) {
		Player->Multicast_PlayOverrideMontage(ConditionData.ConditionMontage);
	}
}

void UPlayerConditionEffect_Electronic::PersistEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, float DeltaTime)
{
	if (!Player) return;
	if (!Player->HasAuthority()) return;

	Player->ApplyDamageInternal(ConditionData.HelthChange, ConditionData.CausePlayer, nullptr, false, false, false);
}

void UPlayerConditionEffect_Electronic::EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect)
{
	if (Player) {
		Player->RemoveInputBlockController(FName("Electronic"));
		if (ConditionData.ConditionMontage) {
			Player->Multicast_StopMontage(ConditionData.ConditionMontage, 0.15f);
		}
	}

	Super::EndFunction(Player, ConditionComp, ConditionData, bUseEndEffect);
}

void UPlayerConditionEffect_Electronic::ResumeEffectVisual(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData)
{
	if (!Player) return;
	if (!ConditionData.ConditionMontage) return;

	Player->Multicast_PlayOverrideMontage(ConditionData.ConditionMontage);
}


