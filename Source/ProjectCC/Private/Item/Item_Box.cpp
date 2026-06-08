// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_Box.h"
#include "Player_Character.h"
#include "PlayerTransformations.h"
#include "PlayerTransformationComponent.h"

bool AItem_Box::UseEffect_Implementation(APlayer_Character* Player) {
	if (!HasAuthority()) return false;
	if (!Player || !Player->TransformationComp) return false;
	if (!TransformationData) return false;

	Player->TransformationComp->StartTransformation(TransformationData, Player, -1.f);

	return true;
}


