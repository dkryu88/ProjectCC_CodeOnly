// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerConditions.h"
#include "PlayerConditionDataAsset.generated.h"

/**
 * 
 */
class UPlayerConditionEffect;
class UAnimMontage;

UCLASS()
class PROJECTCC_API UPlayerConditionDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	//Condition 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ConditionName = FName(TEXT("None"));
	//Condition 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPlayerConditionType ConditionType = EPlayerConditionType::None;
	//Condition 지속 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Duration = 0.f;
	//Condition 중 받는 데미지
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HelthChange = 0.f;
	//Condition 중 받는 효과 수치
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectValue = 0.f;
	//Condition 중 효과 받는 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EffectCount = 3;
	//Condition 중 효과 받는 간격
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectInterval = 0.f;
	//효과 중첩 적용 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanMultiApply = false;
	//효과 중첩 최소 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bCanMultipApply", ClampMin = "0.0"))
	float MultiApplyInterval = 0.5f;
	//제거 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanRemove = true;
	//우선순위 (높은 우선순위는 모든 낮은 우선순위 상태를 제거)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 0;
	//Condition 특수 효과
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UPlayerConditionEffect> ConditionEffect;
	//Condition 애니메이션
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Animation")
	TObjectPtr<UAnimMontage> ConditionMontage = nullptr;

	//플레이어 행동 별 Condition 진행 Rule---------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Rule")
	EConditionEventRule JumpRule = EConditionEventRule::Keep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Rule")
	EConditionEventRule DodgeRule = EConditionEventRule::Keep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Rule")
	EConditionEventRule AttackRule = EConditionEventRule::Keep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Rule")
	EConditionEventRule HitRule = EConditionEventRule::Keep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Rule")
	EConditionEventRule BigHitRule = EConditionEventRule::Keep;
};
