// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemListDataAsset.generated.h"

/**
 * 
 */
class AItem;

UCLASS()
class PROJECTCC_API UItemListDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AllItem")
	TArray<TSubclassOf<AItem>> BItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AllItem")
	TArray<TSubclassOf<AItem>> AItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AllItem")
	TArray<TSubclassOf<AItem>> SItems;
};
