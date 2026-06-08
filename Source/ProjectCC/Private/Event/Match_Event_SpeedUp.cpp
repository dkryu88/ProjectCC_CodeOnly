// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/Match_Event_SpeedUp.h"
#include "Player_Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void AMatch_Event_SpeedUp::StartEvent_Implementation(AMapConstructor* Map, APlayMode_Match* InMatchMode, float duration)
{
	Super::StartEvent_Implementation(Map, InMatchMode, duration);
	ApplySpeedBoost(true);
}

void AMatch_Event_SpeedUp::StopEvent_Implementation()
{
	ApplySpeedBoost(false);
	Super::StopEvent_Implementation();
}

void AMatch_Event_SpeedUp::ApplyEventToPlayer_Implementation(APlayer_Character* Player) {
	if (!bEventRunning) {
		ApplySpeedBoost(false);
		return;
	}

	ApplySpeedBoostToPlayer(Player, true);
}

void AMatch_Event_SpeedUp::ApplySpeedBoostToPlayer(APlayer_Character* Player, bool bApply)
{
	if (!Player || Player->IsOut()) return;

	if (bApply) {
		Player->AddSpeedController(FName("Event_SpeedUp"), SpeedMultiplier, 0.f, true, 2);
		Player->SetMaintainMoveOnNotInput(true, 0.5f);
	}
	else {
		Player->RemoveSpeedControllerByName(FName("Event_SpeedUp"));
		Player->SetMaintainMoveOnNotInput(false);
	}
}

void AMatch_Event_SpeedUp::ApplySpeedBoost(bool bApply) {
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		APlayerController* PC = IT->Get();
		if (!PC) continue;

		APlayer_Character* Player = Cast<APlayer_Character>(PC->GetPawn());
		ApplySpeedBoostToPlayer(Player, bApply);
	}
}
