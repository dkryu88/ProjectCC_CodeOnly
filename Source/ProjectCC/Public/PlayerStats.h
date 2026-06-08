// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerStats.generated.h"

/**
 * 플레이어 스탯 구성
 */
UENUM(BlueprintType)
enum class PAttackTargetType :uint8
{
	//SingleTarget:단일
	SingleTarget UMETA(DisplayName = "SingleTarget"),
	//MultiTarget:범위
	MultiTarget UMETA(DisplayName = "MultiTarget")
};

UENUM(BlueprintType)
enum class PAttackType :uint8
{
	//근접 공격
	Melee UMETA(DisplayName = "Melee"),
	//원거리 공격
	Shoot UMETA(DisplayName = "Shoot"),
	//투척 공격
	Throw UMETA(DisplayName = "Throw")
};

USTRUCT(BlueprintType)
struct FPlayerStats {
	GENERATED_BODY()

	//공격력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Attack = 5.f;
	//최대 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Max_HP = 100.f;
	//최초 무게
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Default_Weight = 1;
	//최초 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Default_Speed = 400.f;
	//공격 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRate = 2.f;
	//공격 사거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRange = 0.85f;
	//공격 범위 각도 (0 ~ 360)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackDegree = 20.f;
	//공격 범위 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRadius = 20.f;
	//공격 선딜레이
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackEarlierDelay = 0.05f;
	//공격 대상 넉백 기본값
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KnockBackStrength = 750.f;
	//공격 대상 넉백 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KnockBackScale = 1.f;
	//공격 대상
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PAttackTargetType AttackTargetType = PAttackTargetType::SingleTarget;
	//공격 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PAttackType AttackType = PAttackType::Melee;
};
