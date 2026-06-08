// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_Spoon.h"
#include "Player_Character.h"
#include "Kismet/GameplayStatics.h"

void AWeapon_Spoon::HitEffect_Implementation(APlayer_Character* Player, AActor* Target)
{
	if (!HasAuthority() || !Player || !Target) return;

	APlayer_Character* CurrentTarget = Cast<APlayer_Character>(Target);
	if (!CurrentTarget) return;

	//같은 대상 공격 시 ComboStack만큼 데이지 증가
	if (LastHitPlayer == CurrentTarget) {
		ComboStack++;
		float ExtraDamage = ComboStack * StackBonusDamage;

		UGameplayStatics::ApplyDamage(CurrentTarget, ExtraDamage, Player->GetController(), Player, nullptr);
	}
	else {
		ComboStack = 0;
		LastHitPlayer = CurrentTarget;
	}
}
