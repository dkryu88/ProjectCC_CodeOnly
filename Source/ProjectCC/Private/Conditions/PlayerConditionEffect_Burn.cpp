// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_Burn.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

void UPlayerConditionEffect_Burn::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* causePlayer)
{
	ConditionData.NextEffectTimer = ConditionData.EffectInterval;
	ConditionData.CausePlayer = causePlayer;

	if (ConditionData.EffectCount > 0) {
		ConditionData.RemainingTickCount = ConditionData.EffectCount;
	}
	else if (ConditionData.EffectInterval > 0.f) {
		ConditionData.RemainingTickCount = FMath::Max(1, FMath::FloorToInt(ConditionData.Duration / ConditionData.EffectInterval));
	}
}

void UPlayerConditionEffect_Burn::PersistEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, float DeltaTime)
{
	if (!Player) return;
	if (ConditionData.EffectInterval <= 0.f) return;
	if (ConditionData.RemainingTickCount <= 0) return;

	ConditionData.NextEffectTimer -= DeltaTime;

	while (ConditionData.NextEffectTimer <= 0.f && ConditionData.RemainingTickCount > 0) {
		AController* CausePlayerController = nullptr;
		if (ConditionData.CausePlayer) {
			CausePlayerController = ConditionData.CausePlayer->GetController();
		}
		UGameplayStatics::ApplyDamage(Player, ConditionData.HelthChange, CausePlayerController, ConditionComp ? ConditionComp->GetOwner() : nullptr, UDamageType::StaticClass());

		ConditionData.RemainingTickCount--;
		ConditionData.NextEffectTimer += ConditionData.EffectInterval;
	}

}

void UPlayerConditionEffect_Burn::EndEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData)
{
}


