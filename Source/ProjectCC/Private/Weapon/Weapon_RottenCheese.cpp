// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_RottenCheese.h"
#include "MapConstructor.h"
#include "PlayMode_Match.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "Kismet/GameplayStatics.h"

bool AWeapon_RottenCheese::UseEffect_Implementation(APlayer_Character* OwnPlayer)
{
	Super::UseEffect_Implementation(OwnPlayer);
	return true;
}
