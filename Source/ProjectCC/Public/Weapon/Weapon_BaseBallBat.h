// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_BaseBallBat.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AWeapon_BaseBallBat : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual bool UseEffect_Implementation(APlayer_Character* Player) override;
	virtual void HitEffect_Implementation(APlayer_Character* Player, AActor* Target) override;

	UPROPERTY(EditDefaultsOnly, Category = "Bat")
	float KnockBackForce_Coin = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Bat")
	float KnockBackForce_OtherNotPlayer = 800.f;
};
