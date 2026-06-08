// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_TaserBullet.generated.h"

/**
 * 
 */
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AObjects_TaserBullet : public AObjects
{
	GENERATED_BODY()
	
public:
	AObjects_TaserBullet(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void Func_HitPlayer_Implementation(APlayer_Character* Player) override;

	UPROPERTY(EditAnywhere, Category="ConditionDataAsset")
	TObjectPtr<UPlayerConditionDataAsset> ElectronicConditionDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConditionDataAsset")
	float ElectronicDuration = 3.f;
};
