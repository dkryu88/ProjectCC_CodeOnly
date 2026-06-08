// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponStats.generated.h"

/**
 * 무기들이 가져야하는 공통 스탯을 지정
 */

//공격 대상을 지정
UENUM(BlueprintType)
enum class EAttackTargetType :uint8
{
	//SingleTarget:단일
	SingleTarget UMETA(DisplayName="SingleTarget"),
	//MultiTarget:범위
	MultiTarget UMETA(DisplayName="MultiTarget")
};

UENUM(BlueprintType)
enum class EAttackType :uint8
{
	//근접 공격
	Melee		UMETA(DisplayName = "Melee"),
	//원거리 공격
	Shoot		UMETA(DisplayName = "Shoot"),
	//투척 공격
	Throw		UMETA(DisplayName = "Throw"),
	//원거리 공격(히트 스캔)
	Shoot_HS	UMETA(DisplayName = "Shoot(HitScan)")
};

UENUM(BlueprintType)
enum class EWeaponAttackInputType : uint8
{
	Single		UMETA(DisplayName = "Single Attack"),
	Continuous	UMETA(DisplayName = "Continuous Attack"),
	Repeat		UMETA(DisplayName = "Repeat Attack"),
	Burst		UMETA(DisplayName = "Burst Attack")
};

USTRUCT(BlueprintType)
struct FWeaponStats 
{
	GENERATED_BODY()
	//무기 이름
	UPROPERTY(EditAnywhere)
	FName AttackName = "Default";
	//무기 공격력
	UPROPERTY(EditAnywhere)
	float Attack = 10.f;
	//무기 공격 속도
	UPROPERTY(EditAnywhere)
	float AttackRate = 1.f;
	//무기 공격 사거리
	UPROPERTY(EditAnywhere)
	float AttackRange = 1.f;
	//무기 공격 범위 크기
	UPROPERTY(EditAnywhere)
	float AttackRadius = 20.f;
	//무기 공격 범위 넓이(0~360)
	UPROPERTY(EditAnywhere)
	float AttackDegree = 180.f;
	//무기 공격 대상 넉백 수치
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KnockBackStrength = 750.f;
	//공격 대상 넉백 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KnockBackScale = 1.f;
	//무기 공격 대상 수 
	UPROPERTY(EditAnywhere)
	EAttackTargetType AttackTargetType;
	//무기 공격 선딜레이
	UPROPERTY(EditAnywhere)
	float AttackEarlierDelay;
	//무기 공격 후딜레이 <미구현>
	UPROPERTY(EditAnywhere)
	float AttackAfterDelay;
	//무기 공격 방식
	UPROPERTY(EditAnywhere)
	EAttackType AttackType;
	//무기 공격 기동 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	EWeaponAttackInputType AttackInputType = EWeaponAttackInputType::Single;
	//무기 점사 수 (점사 공격 방식) <미구현>
	UPROPERTY(EditAnywhere)
	int32 BurstCount = 3;
	//연속 공격 간견 (지속, 점사 공격 방식)
	UPROPERTY(EditAnywhere)
	float ContinuousAttackInterval = 0.1f;
};
