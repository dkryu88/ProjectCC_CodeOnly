// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_Box.generated.h"

/**
 * 
 */
class UPlayerTransformationDataAsset;

UCLASS()
class PROJECTCC_API AItem_Box : public AItem
{
	GENERATED_BODY()
	
protected:
	virtual bool UseEffect_Implementation(APlayer_Character* Player) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Transformation")
	TObjectPtr<UPlayerTransformationDataAsset> TransformationData;

};
