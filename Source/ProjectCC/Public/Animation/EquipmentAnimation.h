// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EquipmentAnimation.generated.h"

class UAnimSequence;
class UAnimMontage;

USTRUCT(BlueprintType)
struct FEquipmentActionAnimation
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> Sequence = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> MontageOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bForceFullBody = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BlendInTime = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BlendOutTime = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PlayRate = 1.0f;


	bool IsValid() {
		return Sequence != nullptr || MontageOverride != nullptr;
	}
};