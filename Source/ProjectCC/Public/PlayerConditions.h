// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerConditions.generated.h"

/**
 * 플레이어의 상태를 나타냄(기절, 감전, 탈락 등)
 */

class UPlayerConditionEffect;
class APlayer_Character;
class UAnimMontage;

UENUM(BlueprintType)
enum class EPlayerConditionEvent : uint8
{
	Jump,				//점프 시
	Dodge,				//회피 시
	Attack,				//공격 시
	Hit,				//피격 시
	BigHit,				//강한 피격 시
};

UENUM(BlueprintType)
enum class EConditionEventRule : uint8 {
	Keep,				//Condition 효과 유지
	Remove,				//Condition 제거
};

UENUM(BlueprintType)
enum class EPlayerConditionType : uint8 {
	None	UMETA(DisplayName = "None"),
	Buff	UMETA(DisplayName = "Buff"),
	DeBuff	UMETA(DisplayName = "DeBuff"),
	Complex	UMETA(DisplayName = "Complex")
};

USTRUCT(BlueprintType)
struct FPlayerCondition {
	GENERATED_BODY()
	//Condition 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ConditionName = FName(TEXT("None"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPlayerConditionType ConditionType = EPlayerConditionType::None;
	//지속시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration = 0.f;
	//Condition 중 받는 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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
	//Condition 중첩 적용 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanMultiApply = false;
	//우선순위 (높은 우선순위는 모든 낮은 우선순위 상태를 제거)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 0;
	//제거 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanRemove = true;
	//효과 실행 용도--------------------------------
	//다음 효과 적용까지 남은 시간
	UPROPERTY(Transient)
	float NextEffectTimer = 0.f;
	//상태 현재 남은 적용 횟수
	UPROPERTY(Transient)
	int32 RemainingTickCount = 0;
	//효과 적용 주체
	UPROPERTY(Transient)
	TObjectPtr<APlayer_Character> CausePlayer;
	//상태 특수 효과
	UPROPERTY(Transient)
	TObjectPtr<UPlayerConditionEffect> ConditionEffect = nullptr;
	//현재 상태 애니메이션 (필요한 경우만 등록)
	UPROPERTY(Transient)
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

	EConditionEventRule GetRuleByEvent(EPlayerConditionEvent Event) const {
		switch (Event) {
		case EPlayerConditionEvent::Jump: return JumpRule;
		case EPlayerConditionEvent::Dodge: return DodgeRule;
		case EPlayerConditionEvent::Attack: return AttackRule;
		case EPlayerConditionEvent::Hit: return HitRule;
		case EPlayerConditionEvent::BigHit: return BigHitRule;
		default: return EConditionEventRule::Keep;
		}
	}

	bool HasConditionAnimation() const {
		if (ConditionMontage != nullptr) {
			return true;
		}
		return false;
	}
};