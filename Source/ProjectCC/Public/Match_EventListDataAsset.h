// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Match_EventListDataAsset.generated.h"

/**
 * 
 */
class AMatch_Event;

USTRUCT(BlueprintType)
struct FMatchEventData {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AMatch_Event> Event;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float EventDuration = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName EventName = FName("Default");
};

UCLASS()
class PROJECTCC_API UMatch_EventListDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FMatchEventData> Events;
	
};
