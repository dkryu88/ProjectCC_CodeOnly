// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_WaterGun.h"
#include "PlayerConditionComponent.h"
#include "Player_Character.h"

void AWeapon_WaterGun::HitEffect_Implementation(APlayer_Character* Player, AActor* Target)
{
	Super::HitEffect_Implementation(EquippedPlayer, Target);

	if (!HasAuthority()) return;
	if (!Target) return;
	if (!Player) return;

	APlayer_Character* player = Cast<APlayer_Character>(Target);
	if (!player) return;

	UPlayerConditionComponent* ConditionComp = player->FindComponentByClass<UPlayerConditionComponent>();
	if (!ConditionComp) return;

	ConditionComp->RemoveSameNameCondition("Burn", false);
}
