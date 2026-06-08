// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_RottenCheese.generated.h"

/**
 * 
 */
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AObjects_RottenCheese : public AObjects
{
	GENERATED_BODY()

public:
	AObjects_RottenCheese(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void Func_Destroy_Implementation() override;
	virtual void Func_HitPlayer_Implementation(APlayer_Character* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConditionDataAsset")
	TObjectPtr<UPlayerConditionDataAsset> SlowConditionData;

	UPROPERTY(EditDefaultsOnly, Category = "Area")
	TSubclassOf<class AArea_RottenCheeseSlowArea> RottenCheeseSlowArea;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConditionDataAsset")
	float SlowDuration = 2.f;

	bool bEffectApplied = false;
};
