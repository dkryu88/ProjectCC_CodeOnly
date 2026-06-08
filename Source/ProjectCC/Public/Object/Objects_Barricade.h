// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_Barricade.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AObjects_Barricade : public AObjects
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void ApplyAdditionalSetting() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
	float TraceDistance = 150.f;
};
