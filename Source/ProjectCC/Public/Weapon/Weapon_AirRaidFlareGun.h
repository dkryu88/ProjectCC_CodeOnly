// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_AirRaidFlareGun.generated.h"

/**
 * 
 */
class UNiagaraSystem;

UCLASS()
class PROJECTCC_API AWeapon_AirRaidFlareGun : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual bool InteractionWeaponFunction(EFunctionInterActionReason Reason) override;

	int32 CurrentRow = 0;
	FTimerHandle RowTimerHandle;

	//줄 간 폭발 간격
	UPROPERTY(EditDefaultsOnly, Category="AirRaid")
	float RowInterval = 0.15f;
	//공격 입력 후 첫 폭발까지 대기 시간
	UPROPERTY(EditDefaultsOnly, Category="AirRaid")
	float ExplosionDelay = 1.25f;
	//한 칸당 폭발 데미지
	UPROPERTY(EditDefaultsOnly, Category="AirRaid")
	float ExplosionDamage = 100.f;
	//폭발 가로 범위 (블록 단위)
	UPROPERTY(EditDefaultsOnly, Category = "AirRaid")
	int32 RangeInBlocks = 7;
	//폭발 총 횟수
	UPROPERTY(EditDefaultsOnly, Category = "AirRaid")
	int32 RowCount = 7;

	UPROPERTY(EditDefaultsOnly, Category = "AirRaid|Flare")
	float FlareSpawnZOffset = 100.f;

protected:
	FTimerHandle ExplosionTimerHandle;
	bool bPendingExplosion = false;
};
