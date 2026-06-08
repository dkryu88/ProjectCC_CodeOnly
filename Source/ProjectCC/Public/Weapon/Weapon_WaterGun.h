// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_WaterGun.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AWeapon_WaterGun : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual void HitEffect_Implementation(APlayer_Character* Player, AActor* Target) override;
};
