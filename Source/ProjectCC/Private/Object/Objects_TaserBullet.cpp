// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_TaserBullet.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"

AObjects_TaserBullet::AObjects_TaserBullet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AObjects_TaserBullet::Func_HitPlayer_Implementation(APlayer_Character* Player) {
	Super::Func_HitPlayer_Implementation(Player);

	if (!HasAuthority() || !Player) return;
	if (Player && !Player->IsOut() && ElectronicConditionDataAsset && Player->ConditionComp) {
		Player->ConditionComp->ApplyCondition(ElectronicConditionDataAsset, OwnPlayer, ElectronicDuration);
	}
}