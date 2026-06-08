// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_FireBottle.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AWeapon_FireBottle : public AWeapon
{
	GENERATED_BODY()

public:
	virtual bool UseEffect_Implementation(APlayer_Character* OwnPlayer) override;
};
