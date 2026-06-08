// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimation.generated.h"

/**
 * 
 */

UCLASS()
class PROJECTCC_API UPlayerAnimation : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
public:
	UPROPERTY(BlueprintReadWrite, Category=PlayerAnim)
	float Move_Speed = 0.f;

	UPROPERTY(BlueprintReadWrite, Category=PlayerAnim)
	float Jump_Velocity = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = PlayerAnim)
	float Move_ForwardDir = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = PlayerAnim)
	float Move_SideDir = 0.f;

	UPROPERTY(BlueprintReadWrite, Category=PlayerAnim)
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadWrite, Category=PlayerAnim)
	bool bIsDodging = false;
	
	UPROPERTY(BlueprintReadWrite, Category = PlayerAnim)
	TObjectPtr<UAnimSequenceBase> CurrentGripSequence;

	UPROPERTY(BlueprintReadWrite, Category = PlayerAnim)
	bool bHasGrip = false;

	UPROPERTY(BlueprintReadWrite, Category = PlayerAnim)
	float GripAlpha = 0.f;
};
