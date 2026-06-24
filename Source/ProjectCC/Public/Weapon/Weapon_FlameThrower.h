// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_FlameThrower.generated.h"

/**
 * 
 */
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AWeapon_FlameThrower : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual void HitEffect_Implementation(APlayer_Character* Player, AActor* Target) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Condition")
	TObjectPtr<UPlayerConditionDataAsset> ConditionData;

};
