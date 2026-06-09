// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_CoinBag.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AObjects_CoinBag : public AObjects
{
	GENERATED_BODY()
	
public:
	AObjects_CoinBag(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Coin")
	TSubclassOf<class ACoin> CoinClass;

	UPROPERTY(EditAnywhere, Category = "Coin")
	TSubclassOf<class ACoin> MidCoinClass;

	UPROPERTY(EditAnywhere, Category = "Coin")
	int32 SearchRadius = 1;

	UPROPERTY(EditAnywhere, Category = "Coin")
	int32 SearchHeight = 1;

	virtual void Func_AttackedByPlayer_Implementation(APlayer_Character* Player) override;
	virtual void Func_Destroy_Implementation() override;
	virtual void ApplyAdditionalSetting() override;

	void SpawnCoin();
	bool CollectNearbySafeBlocksFromMap(TArray<FVector>& SafeBlockLocations, int32 instanceSearchRadius, int32 instanceSearchHeight);
};
