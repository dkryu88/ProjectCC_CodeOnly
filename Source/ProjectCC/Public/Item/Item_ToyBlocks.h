// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_ToyBlocks.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AItem_ToyBlocks : public AItem
{
	GENERATED_BODY()

protected:
	virtual bool UseEffect_Implementation(class APlayer_Character* Player) override;

	UPROPERTY(EditAnywhere, Category="ToyBlocks")
	TSubclassOf<class AArea_ToyBlocks> ToyBlocks;
};
