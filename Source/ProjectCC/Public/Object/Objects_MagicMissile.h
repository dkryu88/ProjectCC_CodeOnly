// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_MagicMissile.generated.h"

/**
 * 
 */
class UPlayerTransformationDataAsset;
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AObjects_MagicMissile : public AObjects
{
	GENERATED_BODY()

public:
	AObjects_MagicMissile(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void Func_HitPlayer_Implementation(APlayer_Character* HitPlayer) override;

	UPROPERTY(EditAnywhere, Category = "Magic")
	TObjectPtr<UPlayerTransformationDataAsset> TransformationData;
};
