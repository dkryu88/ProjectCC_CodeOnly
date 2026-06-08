// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_RedStaff.h"
#include "Player_Character.h"

void AWeapon_RedStaff::HitEffect_Implementation(APlayer_Character* Player, AActor* Target) {
	if (!HasAuthority() || !Player)return;

	APlayer_Character* TargetPlayer = Cast<APlayer_Character>(Target);
	if (TargetPlayer && !TargetPlayer->IsOut()) {
		Player->HPChange(HealAmount);
	}
}