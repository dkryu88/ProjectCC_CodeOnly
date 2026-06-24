// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_Burn.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

void UPlayerConditionEffect_Burn::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* causePlayer)
{
	ConditionData.CausePlayer = causePlayer;
}

void UPlayerConditionEffect_Burn::PersistEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, float DeltaTime)
{
	if (!Player) return;

	AController* CausePlayerController = nullptr;

	if (ConditionData.CausePlayer) {
		CausePlayerController = ConditionData.CausePlayer->GetController();
	}

	UGameplayStatics::ApplyDamage(Player, ConditionData.HelthChange, CausePlayerController, ConditionComp ? ConditionComp->GetOwner() : nullptr, UDamageType::StaticClass());
}

void UPlayerConditionEffect_Burn::EndEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData)
{
}


