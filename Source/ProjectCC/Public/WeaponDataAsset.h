// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/EquipmentAnimation.h"
#include "Objects.h"
#include "WeaponStats.h"
#include "Effect/FGameEffectData.h"
#include "Player_FunctionInterActionReason.h"
#include "WeaponDataAsset.generated.h"

/**
 * 각각의 무기들 Stat을 관리 (Stat의 종류는 WeaponTypes.h에서 지정받음)
 */


UENUM(BlueprintType)
enum class EWeaponGrade : uint8
{
	None UMETA(DisplayName = "Default"),
	B    UMETA(DisplayName = "B Grade"),
	A    UMETA(DisplayName = "A Grade"),
	S    UMETA(DisplayName = "S Grade")
};

UENUM(BlueprintType)
enum class EWeaponAttackDirection : uint8
{
	Horizontal,
	Vertical,
	Round
};

UCLASS()
class PROJECTCC_API UWeaponDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	//무기 스탯
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FWeaponStats Stats;

	//원거리/투척 무기 발사체 지정
	UPROPERTY(EditAnywhere, BlueprintReadONly, Category = "WeaponBullet")
	TSubclassOf<class AObjects> Bullet;

	//무기 사용 가능 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 MaxUseCount = 1;

	//무기 무게 (0 ~ 2)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 Weight = 0;

	//무기 공격이 플레이어를 회전 시키는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bApplyRotation = true;

	//무기 공격이 플레이어를 넉백 시키는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bApplyKnockBack = true;

	//무기 조준 회전 속도 (0:기본, 1:느림, 2:완전 느림)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 Aim_TurnSpeed = 0;

	//무기 비적중 시 카운트 줄이기 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bNotCountWhenUnHit = false;

	//무기 등급
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponGrade WeaponGrade = EWeaponGrade::B;

	//무기 공격 방향
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponAttackDirection AttackDirection = EWeaponAttackDirection::Horizontal;

	//무기 공격 방향 Round일때 세로 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bHorizonEqualVertical = false;

	//무기 행동별 애니메이션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TMap<EFunctionInterActionReason, FEquipmentActionAnimation> AdditionalAnimation;

	//무기 대기 애니메이션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> WeaponGripSequence;

	//무기 애니메이션 재생 기준 (True면 손은 다른 애니메이션에 영향을 받지 않음, False면 손도 다른 애니메이션에 영향을 받음)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bUsingSpecialPose = false;

	//무기 적중 액션 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bUsingHitAction = true;

	//무기 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UTexture2D> WeaponIcon = nullptr;

	//무기 조준 Preview-------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	bool bUseAimPreview = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	bool bOverrideAimPreviewRange = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview", meta=(EditCondition="bOverrideAimPreviewRange"))
	float AimPreviewRangeOverride = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	bool bOverrideAimPreviewDegree = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview", meta=(EditCondition="bOverrideAimPreviewDegree"))
	float AimPreviewDegreeOverride = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	float ShootPathPreviewRadius = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	FLinearColor AimFillColor = FLinearColor(0.1f, 0.4f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	FLinearColor AimEdgeColor = FLinearColor(0.3f, 0.8f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	float AimFillOpacity = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	float AimEdgeOpacity = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	float AimEdgeWidth = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	float AimEdgeSoftness = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	float AimPatternLength = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	float AimFlowSpeed = 0.1f;
	
	//무기 이펙트--------------------------------------------------------
	//공격(무기 사용) 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	FGameEffectData AttackEffect;
	//타격 이펙트 (무기 던지기/무기 적중)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	FGameEffectData HitEffect;
	//소멸 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	FGameEffectData DestroyEffect;
	//기타 상황 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TMap<FName, FGameEffectData> CustomEffects;
};
