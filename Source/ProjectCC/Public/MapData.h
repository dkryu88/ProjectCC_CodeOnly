// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BlockType.h"
#include "MapData.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API UMapData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 X = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Y = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Z = 15;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BlockSize = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EBlockType> MapData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntVector> FloorBlocksData;
};
