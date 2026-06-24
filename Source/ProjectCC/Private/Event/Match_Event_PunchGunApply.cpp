// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/Match_Event_PunchGunApply.h"
#include "Weapon/Weapon_PunchGun.h"
#include "Player_Character.h"
#include "GameFramework/PlayerController.h"
#include "PlayMode_Match.h"
#include "EngineUtils.h"

void AMatch_Event_PunchGunApply::StartEvent_Implementation(AMapConstructor* Map, APlayMode_Match* InMatchMode, float Duration)
{
	Super::StartEvent_Implementation(Map, InMatchMode, Duration);
	ApplyPunchGun(true);
}

void AMatch_Event_PunchGunApply::StopEvent_Implementation()
{
	ApplyPunchGun(false);
	Super::StopEvent_Implementation();
}

void AMatch_Event_PunchGunApply::ApplyEventToPlayer_Implementation(APlayer_Character* Player)
{
	if (!Player || !PunchGunClass) return;
	Player->EquippmentLockActivateForEvent(PunchGunClass, true);
}

void AMatch_Event_PunchGunApply::ApplyPunchGun(bool bApply)
{
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		APlayerController* PC = IT->Get();
		if (!PC) continue;

		APlayer_Character* Player = Cast<APlayer_Character>(PC->GetPawn());
		if (!Player) continue;

		if (bApply) {
			if (PunchGunClass) {
				Player->EquippmentLockActivateForEvent(PunchGunClass, true);
			}
		}
		else {
			Player->EquippmentLockActivateForEvent(nullptr, false);
		}
	}
	if (MatchMode) MatchMode->bCanSupplyWeaponSpawn = bApply;
}


