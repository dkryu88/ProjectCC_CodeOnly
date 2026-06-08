// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_RailGun.generated.h"

/**
 * 
 */

class APlayer_Character;

UCLASS()
class PROJECTCC_API AWeapon_RailGun : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual bool BeforeAttackWeaponFunction() override;
	virtual bool InteractionWeaponFunction(EFunctionInterActionReason Reason) override;
	virtual void ReleaseAttackWeaponFunction() override;
	virtual void AdditionalUnEquipWeaponFunction() override;

	void FireRailGunBeam(float Damage, float Radius, float GaugePercent);
	float GetDamageByGauge(float Gauge, float MaxDamage);
	float GetRadiusByGauge(float Gauge);
	void ApplyRailGunMoveSpeed();
	void ResetRailGunMoveSpeed();

	void ExecuteRailGunContinuousAttack();
	void ReleaseRailGunAttack();
	void CancelRailGunAttack();

public:
	// 발사 가능 최소 게이지 — 이 값 미만에서 Release하면 발사 안 함
	UPROPERTY(EditDefaultsOnly, Category = "RailGun")
	float MinGauge = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "RailGun")
	float ChargeSpeedPerSecond = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "RailGun")
	float ChargeAmountPerSecond = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "RailGun")
	float MaxChargeHoldDuration = 3.f;

	// 최소 충전(MinGauge) 시 데미지 비율 (DataAsset Attack 기준, 0~1)
	UPROPERTY(EditDefaultsOnly, Category = "RailGun")
	float MinDamageRatio = 0.1f;

	// 게이지 MinGauge일 때 빔 반경
	UPROPERTY(EditDefaultsOnly, Category = "RailGun")
	float MinBeamRadius = 10.f;

	// 게이지 75 이상에서 고정되는 최대 빔 반경
	UPROPERTY(EditDefaultsOnly, Category = "RailGun")
	float MaxBeamRadius = 80.f;
	
	//레일건의 현재 차징 중 여부
	UPROPERTY()
	bool bRailGunCharging = false;

	// 이번 틱에 사용할 빔 반경
	float PendingRadius = 0.f;

	// 이번 발사에 사용할 충전 데미지
	float PendingDamage = 0.f;

	// 현재 충전 게이지 (0~100)
	float ChargeGauge = 0.f;

	// 최근 충전 시간 (충전 간격을 맞추기 위함)
	float LastChargeTime = -1.f;

	// 최대 게이지 도달 시간
	float MaxChargeStartTime = -1.f;

	// 게이지 100 도달로 자동발사된 상태 — 다음 틱 홀드 차단, Release 시 UseCount 1 소모 후 해제
	bool bMaxChargeFired = false;

	// UseCount 소진 후 홀드 자동 재개 방지 플래그
	bool bWaitingRepress = false;
};
