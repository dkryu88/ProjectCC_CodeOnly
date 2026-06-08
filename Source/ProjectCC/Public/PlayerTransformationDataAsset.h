// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerTransformations.h"
#include "PlayerTransformationEffect.h"
#include "PlayerTransformationDataAsset.generated.h"

/**
 * 
 */
class UAnimeInstance;
class UMaterialInterface;

UCLASS()
class PROJECTCC_API UPlayerTransformationDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	FName TransformationName = FName(TEXT("Default"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TObjectPtr<USkeletalMesh> TransformSkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TSubclassOf<UAnimInstance> TransformAnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TObjectPtr<UStaticMesh> TransformStaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TSubclassOf<UPlayerTransformationEffect> TransformationEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	FVector MeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	FRotator MeshRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	FVector MeshScale = FVector(1.f, 1.f, 1.f);

	//0.f는 자동 해제 X
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	float Duration = 5.f;

	//변신 일시 중지 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	float ShortTimeToStopTransformation = 1.f;

	//변신 중단 카운트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	int32 TotalCountToStopTransformation = 3;

	//적용 우선 순위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	int32 Priority = 0;

	//변신 중 플레이어 캐릭터 위젯 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transformation")
	bool bExposureCharacterWidget = true;

	//변신 타입 설정 (버프 / 디버프 / 복합)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPlayerTransformationType TransformationType = EPlayerTransformationType::Buff;

	//VisualManager 사용 여부 (머터리얼 수정 여부)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Visual")
	bool bUseVisualManager = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Visual", meta = (EditCondition = "bUseVisualManager"))
	FName VisualEffectName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Visual", meta = (EditCondition = "bUseVisualManager"))
	FVisualEffectRequest VisualData;

	//변신 중 플레이어 Input에 따른 결과 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult MoveRule = EPlayerInputResult::CanAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult JumpRule = EPlayerInputResult::CantAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult DodgeRule = EPlayerInputResult::CantAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult AttackRule = EPlayerInputResult::StopTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult InteractionRule = EPlayerInputResult::CanAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult HittedRule = EPlayerInputResult::StopTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation Input")
	EPlayerInputResult UseItemRule = EPlayerInputResult::StopTransform;
};
