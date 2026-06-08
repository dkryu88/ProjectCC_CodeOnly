// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_StealthRobe.h"
#include "Player_Character.h"
#include "PlayerTransformationComponent.h"
#include "PlayerTransformationDataAsset.h"

bool AItem_StealthRobe::UseEffect_Implementation(APlayer_Character* Player)
{
	if (!HasAuthority()) {
		return false;
	}

	if (!Player || !Player->TransformationComp) {
		return false;
	}

	if (!StealthTransformationData) {
		return false;
	}

	return Player->TransformationComp->StartTransformation(StealthTransformationData, Player, StealthDuration);
}
