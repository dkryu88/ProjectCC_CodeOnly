// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_BambooSpear.generated.h"

/**
 *
 */
UCLASS()
class PROJECTCC_API AWeapon_BambooSpear : public AWeapon
{
	GENERATED_BODY()

public:
	virtual float OnPreHit(APlayer_Character* Target, bool& bSkipRotation) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	int32 MultiPlyRuleEnemyCoinEnemy = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	int32 MultiPlyRuleMaxCoinOwner = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	float AttackMagnification = 30.f;

};
