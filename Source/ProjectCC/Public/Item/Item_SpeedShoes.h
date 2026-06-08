// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_SpeedShoes.generated.h"

/**
 * 
 */

class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AItem_SpeedShoes : public AItem
{
	GENERATED_BODY()
	
protected:
	virtual bool UseEffect_Implementation(APlayer_Character* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConditionDataAsset")
	TObjectPtr<UPlayerConditionDataAsset> FastConditionData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConditionDataAsset")
	float SpeedDuration = 15.f;

};
