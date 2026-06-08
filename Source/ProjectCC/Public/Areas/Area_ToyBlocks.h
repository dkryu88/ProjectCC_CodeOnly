// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Area.h"
#include "Area_ToyBlocks.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AArea_ToyBlocks : public AArea
{
	GENERATED_BODY()
	
public:
	AArea_ToyBlocks();

protected:
	virtual void ApplyInAreaEffect_Implementation(AActor* OtherActor) override;
	virtual void ApplyStayAreaEffect_Implementation(AActor* OtherActor) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ToyBlocks")
	float DamageAmount = 5.f;
};
