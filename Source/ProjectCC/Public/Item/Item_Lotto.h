// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_Lotto.generated.h"

/**
 * 
 */
class USoundBase;

UCLASS()
class PROJECTCC_API AItem_Lotto : public AItem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item_Lotto")
	int MinLottoCoin = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item_Lotto")
	int MaxLottoCoin = 20;

	//코인 0개 얻었을 때 소리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Click")
	TObjectPtr<USoundBase> NoCoinGet;

	virtual bool UseEffect_Implementation(APlayer_Character* Player) override;
};
