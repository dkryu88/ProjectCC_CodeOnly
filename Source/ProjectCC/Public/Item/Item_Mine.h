// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_Mine.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AItem_Mine : public AItem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mine")
	TSubclassOf<class AObjects_Mine> Mine;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mine")
	float ImpulsePower = 300.f;

	virtual bool UseEffect_Implementation(APlayer_Character* Player) override;

};
