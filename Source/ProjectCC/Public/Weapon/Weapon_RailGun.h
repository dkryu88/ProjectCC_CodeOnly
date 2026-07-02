// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Animation/AnimSequence.h"
#include "Weapon_RailGun.generated.h"

/**
 * 
 */

class APlayer_Character;

enum class ERailGunAnimState : uint8 {
	None,
	Charging,
	Firing
};

UCLASS()
class PROJECTCC_API AWeapon_RailGun : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool BeforeAttackWeaponFunction() override;
	virtual bool InteractionWeaponFunction(EFunctionInterActionReason Reason) override;
	virtual void ReleaseAttackWeaponFunction() override;
	virtual void AdditionalUnEquipWeaponFunction() override;
	virtual bool BuildAimPreviewData(APlayer_Character* Player, FAimPreviewVisualData& PreviewData) override;

	void FireRailGunBeam(float Damage, float Radius, float GaugePercent);
	float GetDamageByGauge(float Gauge, float MaxDamage);
	float GetRadiusByGauge(float Gauge);
	void ApplyRailGunMoveSpeed();
	void ResetRailGunMoveSpeed();

	void ExecuteRailGunContinuousAttack();
	void ReleaseRailGunAttack();
	void CancelRailGunAttack();

	void InstantHideRailGunPreview(float Duration);

	void PlayRailGunDynamicAnimation(UAnimSequence* Sequence, int32 LoopCount);
	void StopRailGunAnimation();
public:
	//레일건 충전 애니메이션 
	UPROPERTY(EditAnywhere, Category="RainGun | Animation")
	TObjectPtr<UAnimSequence> RailGunChargeAnimation = nullptr;
	//레일건 공격 중 애니메이션
	UPROPERTY(EditAnywhere, Category = "RainGun | Animation")
	TObjectPtr<UAnimSequence> RailGunAttackAnimation = nullptr;
	//레일건 공격 완료 애니메이션
	UPROPERTY(EditAnywhere, Category = "RainGun | Animation")
	TObjectPtr<UAnimSequence> RailGunCompleteAnimation = nullptr;
	//레일건 공격 이펙트 (지속)
	UPROPERTY(EditDefaultsOnly, Category = "RailGun")
	UNiagaraSystem* RailGunBeamEffect;
	//레일건 공격 이펙트 재생 나이아가라 컴포넌트 (기존의 EffectComp는 일시 효과만 적용하므로 따로 생성)
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveRailGunEffectComp;
	//레일건 발사 이펙트 파라미터 갱신
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_UpdateRailGunBeamEffect(FVector_NetQuantize10 BeamStart, FVector_NetQuantize10 BeamEnd, float ChargeRate, float Radius, bool bBeamActive);
	//레일건 발사 이펙트 중지
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StopRailGunBeamEffect();
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
	float MinBeamRadius = 20.f;

	// 게이지 75 이상에서 고정되는 최대 빔 반경
	UPROPERTY(EditDefaultsOnly, Category = "RailGun")
	float MaxBeamRadius = 50.f;
	
	//레일건의 현재 차징 중 여부
	UPROPERTY(ReplicatedUsing = OnRep_RailGunCharging)
	bool bRailGunCharging = false;

	UFUNCTION()
	void OnRep_RailGunCharging();

	// 현재 충전 게이지 (0~100)
	UPROPERTY(Replicated)
	float ChargeGauge = 0.f;

	// 현재 프리뷰를 보여주고 있는지 여부
	UPROPERTY(ReplicatedUsing = OnRep_NoShowingRailGunPreview)
	bool bNoShowingRailGunPreview = false;

	bool bLocalNoShowingRailGunPreview = false;

	UFUNCTION()
	void OnRep_NoShowingRailGunPreview();

	//프리뷰 일시 숨김 타이머
	FTimerHandle InstantHideRailGunPreviewTimerHandle;

	// 이번 틱에 사용할 빔 반경
	float PendingRadius = 0.f;

	// 이번 발사에 사용할 충전 데미지
	float PendingDamage = 0.f;

	//레일건 발사 애니메이션 상태
	ERailGunAnimState RailGunAnimState = ERailGunAnimState::None;

	// 최근 충전 시간 (충전 간격을 맞추기 위함)
	float LastChargeTime = -1.f;

	// 최대 게이지 도달 시간
	float MaxChargeStartTime = -1.f;

	// 게이지 100 도달로 자동발사된 상태 — 다음 틱 홀드 차단, Release 시 UseCount 1 소모 후 해제
	bool bMaxChargeFired = false;

	// UseCount 소진 후 홀드 자동 재개 방지 플래그
	bool bWaitingRepress = false;

	//현재 재생중인 애니메이션 Sequence의 슬롯 이름
	FName CurrentAnimSlot = NAME_None;
	//재생할 애니메이션 Sequence의 슬롯 이름 획득
	FName GetAnimationSlotName();
	//재생중인 애니메이션 슬롯 전환
	void RefreshRailGunAnimationSlot();
};
