// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_FakeCoin.generated.h"

/**
 * 
 */


UCLASS()
class PROJECTCC_API AItem_FakeCoin : public AItem
{
	GENERATED_BODY()
	

public:
	UPROPERTY(EditAnywhere, Category="Item")
	TSubclassOf<AObjects> FakeCoin;

	virtual bool UseEffect_Implementation(APlayer_Character* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	float SpawnZOffset = 35.f;

	//다른 물체를 미는 힘
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Item")
	float ImpulsePower = 300.f;
};
