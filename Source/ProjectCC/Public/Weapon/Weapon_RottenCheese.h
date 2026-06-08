// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_RottenCheese.generated.h"

/**
 * 
 */
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AWeapon_RottenCheese : public AWeapon
{
	GENERATED_BODY()

public:
	virtual bool UseEffect_Implementation(APlayer_Character* OwnPlayer) override;
};
