// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerVisualManagerComponent.h"
#include "PlayerTransformations.generated.h"

class UPlayerTransformationEffect;
class APlayer_Character;

UENUM(BlueprintType)
enum class EPlayerInputResult : uint8
{
	CanAction				UMETA(DisplayName = "Can Action"),
	CantAction				UMETA(DisplayName = "Can't Action"),
	StopTransform			UMETA(DisplayName = "Stop Transformation"),
	StopTransformAndKeep	UMETA(DisplayName = "Stop Transformation And Keep Action"),
	StopTransformInst		UMETA(DisplayName = "Stop Short Time"),
	Counting				UMETA(DisplayName = "Count will add -1. Stop When Count is 0")
};

UENUM(BlueprintType)
enum class ETransformationVisibility : uint8
{
	Visible			UMETA(DisplayName = "Visible"),
	Invisible		UMETA(DisplayName = "Invisible"),
	OtherMaterial	UMETA(DisplayName = "SameMaterial")
};

UENUM(BlueprintType)
enum class EPlayerTransformationType : uint8 {
	Buff,
	Debuff,
	Complex,
	None
};

USTRUCT(BlueprintType)
struct FPlayerTransformation {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	FName TransformationName = FName(TEXT("Normal"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transformation")
	TObjectPtr<USkeletalMesh> TransformSkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TSubclassOf<UAnimInstance> TransformAnimClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Effect")
	TSubclassOf<UPlayerTransformationEffect> PlayerTransformationEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TObjectPtr<UStaticMesh> TransformStaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transformation")
	FVector MeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transformation")
	FRotator MeshRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transformation")
	FVector MeshScale = FVector(1.f, 1.f, 1.f);

	//0.f는 자동 해제 X
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transformation")
	float Duration = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	float ShotTimeToStopTransformation = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	int32 TotalCountToStopTransformation = 0;

	//적용 우선 순위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	int32 Priority = 0;

	//변신 중 플레이어 캐릭터 위젯 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	bool bExposureCharacterWidget = true;

	//변신 타입 설정 (버프 / 디버프 / 복합)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPlayerTransformationType TransformationType = EPlayerTransformationType::Buff;

	//VisualManager 사용 여부 (머터리얼 수정 여부)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Visual")
	bool bUseVisualManager = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Visual")
	FName VisualEffectName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Visual")
	FVisualEffectRequest VisualData;

	//변신 행동 규칙-------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transformation Input")
	EPlayerInputResult MoveRule = EPlayerInputResult::CanAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult JumpRule = EPlayerInputResult::CanAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult DodgeRule = EPlayerInputResult::CanAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult AttackRule = EPlayerInputResult::CanAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult InteractionRule = EPlayerInputResult::CanAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult HittedRule = EPlayerInputResult::CanAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult UseItemRule = EPlayerInputResult::CanAction;

	//효과 실행 용도--------------------------------
	// 변신 적용 중인지 확인
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	bool bActive = false;
	//변신 남은 시간
	UPROPERTY(Transient, NotReplicated)
	float RemainingDuration = 0.f;
	//변신 해제 Count
	UPROPERTY(Transient, NotReplicated)
	int32 RemainingCount = 0;
	//효과 적용 주체
	UPROPERTY(Transient, NotReplicated)
	TObjectPtr<APlayer_Character> CausePlayer = nullptr;
	//상태 특수 효과
	UPROPERTY(Transient, NotReplicated)
	TObjectPtr<UPlayerTransformationEffect> TransformationEffect = nullptr;
};