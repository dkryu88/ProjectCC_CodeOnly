// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_AngelProtection.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "PlayerConditionDataAsset.h"

bool AItem_AngelProtection::UseEffect_Implementation(APlayer_Character* Player)
{
	if (!HasAuthority() || !Player || !InvincibleDataAsset) return false;

	if (Player->ConditionComp) {
		Player->ConditionComp->ApplyCondition(InvincibleDataAsset, Player, DamageImmunityDuration);
		return true;
	}

	return false;
}
