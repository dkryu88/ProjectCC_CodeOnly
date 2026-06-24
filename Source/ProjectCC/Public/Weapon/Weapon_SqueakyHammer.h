// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_SqueakyHammer.generated.h"

/**
 * 
 */

class APlayer_Character;
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AWeapon_SqueakyHammer : public AWeapon
{
	GENERATED_BODY()
	
protected:
	virtual void HitEffect_Implementation(APlayer_Character* Player, AActor* Target)override;

	// 스턴 적용 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConditionDataAsset")
	float StunDuration = 2.f;

	// 콤보 초기화 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConditionDataAsset")
	float ComboResetTime = 10.0f;

private:
	// 마지막 공격 대상 기억
	UPROPERTY()
	APlayer_Character* LastHitPlayer = nullptr;

	// 현재 콤보 스택
	int32 ComboStack = 0;

	// 마지막 타격 시간 저장 변수
	float LastHitTime = 0.0f;

	UPROPERTY(EditAnywhere, Category="ConditionDataAsset")
	TObjectPtr<UPlayerConditionDataAsset> StunDataAsset = nullptr;

};
