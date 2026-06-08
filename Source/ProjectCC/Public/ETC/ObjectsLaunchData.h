// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ObjectsLaunchData.generated.h"
/**
 * 
 */

USTRUCT(BlueprintType)
struct PROJECTCC_API FObjectLaunchData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector LaunchVelocity = FVector::ZeroVector;

	UPROPERTY()
	bool bUseGravity = false;

	UPROPERTY()
	float AttackRange = 0.f;
};
