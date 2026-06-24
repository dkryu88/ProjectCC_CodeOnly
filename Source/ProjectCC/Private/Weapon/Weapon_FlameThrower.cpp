// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_FlameThrower.h"
#include "PlayerConditionDataAsset.h"
#include "PlayerConditionComponent.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

void AWeapon_FlameThrower::HitEffect_Implementation(APlayer_Character* Player, AActor* Target)
{
	Super::HitEffect_Implementation(Player, Target);

	if (!HasAuthority()) return;
	if (!Target || !Player || !ConditionData) return;

	APlayer_Character* player = Cast<APlayer_Character>(Target);
	if (!player) return;

	UPlayerConditionComponent* ConditionComp = player->FindComponentByClass<UPlayerConditionComponent>();
	if (!ConditionComp) return;

	//화염 방사기의 화상 Condition은 2초만 적용
	ConditionComp->ApplyCondition(ConditionData, player, 2.1f);
}
