// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_FireExtinguish.h"
#include "PlayerConditionDataAsset.h"
#include "PlayerConditionComponent.h"
#include "Player_Character.h"

void AWeapon_FireExtinguish::HitEffect_Implementation(APlayer_Character* Player, AActor* Target)
{
	Super::HitEffect_Implementation(EquippedPlayer, Target);

	if (!HasAuthority()) return;
	if (!Target) return;
	if (!Player) return;
	if (!ConditionData) return;

	APlayer_Character* player = Cast<APlayer_Character>(Target);
	if (!player) return;

	UPlayerConditionComponent* ConditionComp = player->FindComponentByClass<UPlayerConditionComponent>();
	if (!ConditionComp) return;

	ConditionComp->ApplyCondition(ConditionData, player, 0.f);

	UE_LOG(LogTemp, Warning, TEXT("Apply SightOut"));
}
