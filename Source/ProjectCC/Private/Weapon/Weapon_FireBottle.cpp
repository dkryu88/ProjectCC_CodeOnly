// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_FireBottle.h"
#include "Player_Character.h"
#include "WeaponDataAsset.h"
#include "Objects.h"

bool AWeapon_FireBottle::UseEffect_Implementation(APlayer_Character* OwnPlayer)
{
	Super::UseEffect_Implementation(OwnPlayer);
	return true;
}
