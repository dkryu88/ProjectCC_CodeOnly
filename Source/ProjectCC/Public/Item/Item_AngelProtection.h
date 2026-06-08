// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_AngelProtection.generated.h"

/**
 * 
 */
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AItem_AngelProtection : public AItem
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Condition")
	float DamageImmunityDuration = 5.f;

	UPROPERTY(EditAnywhere, Category="ConditionData")
	TObjectPtr<UPlayerConditionDataAsset> InvincibleDataAsset;

public:
	virtual bool UseEffect_Implementation(class APlayer_Character* Player) override;
};
