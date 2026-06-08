// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_Spoon.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AWeapon_Spoon : public AWeapon
{
	GENERATED_BODY()
	
protected:
	virtual void HitEffect_Implementation(APlayer_Character* Player, AActor* Target)override;

private:
	// 마지막 공격 대상 기억
	UPROPERTY()
	AActor* LastHitPlayer = nullptr;

	// 현재 콤보 스택
	int32 ComboStack = 0;

	// 스택 당 추가 데미지
	const float StackBonusDamage = 2.f;
};
