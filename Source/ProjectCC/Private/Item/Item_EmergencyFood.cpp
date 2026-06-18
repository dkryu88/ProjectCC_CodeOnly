// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_EmergencyFood.h"
#include "Player_Character.h"
#include "ItemDataAsset.h"
#include "Player_State.h"

bool AItem_EmergencyFood::UseEffect_Implementation(APlayer_Character* Player) {
	if (!Player || !ItemData) return false;

	Player->HPChange(HealAmount);

	return true;
}
