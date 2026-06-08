// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_Dagger.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AWeapon_Dagger : public AWeapon
{
	GENERATED_BODY()
	

protected:
	virtual float OnPreHit(APlayer_Character* Target, bool& bSkipRotation) override;

private:
	const float BackAttackThreshold = 0.707f;
	const float BackAttackMuliplier = 2.0f;
};
