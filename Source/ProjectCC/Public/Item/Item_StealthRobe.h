// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_StealthRobe.generated.h"

/**
 * 
 */

class UPlayerTransformationDataAsset;


UCLASS()
class PROJECTCC_API AItem_StealthRobe : public AItem
{
	GENERATED_BODY()
	

protected:
	virtual bool UseEffect_Implementation(APlayer_Character* Player) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stealth")
	TObjectPtr<UPlayerTransformationDataAsset> StealthTransformationData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stealth")
	float StealthDuration = 10.f;
};
