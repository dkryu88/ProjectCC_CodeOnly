// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_ToyHammer.generated.h"

/**
 * 
 */
class APlayer_Character;
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AWeapon_ToyHammer : public AWeapon
{
	GENERATED_BODY()
	
protected:
	AWeapon_ToyHammer(const FObjectInitializer& ObjectInitializer);
	virtual void HitEffect_Implementation(APlayer_Character* Player, AActor* Target) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ToyHammer")
	float StunDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ToyHammer")
	float ComboResetTime = 3.f;

	UPROPERTY()
	APlayer_Character* LastHitPlayer = nullptr;

	int32 ComboStack = 0;

	float LastHitTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="StunCondition")
	TObjectPtr<UPlayerConditionDataAsset> StunData = nullptr;
};
