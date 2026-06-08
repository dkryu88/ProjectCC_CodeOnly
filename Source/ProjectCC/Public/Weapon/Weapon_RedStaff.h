// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_RedStaff.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AWeapon_RedStaff : public AWeapon
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, Category = "HealData")
	float HealAmount = 10.f;

	virtual void HitEffect_Implementation(APlayer_Character* Player, AActor* Target) override;
};
