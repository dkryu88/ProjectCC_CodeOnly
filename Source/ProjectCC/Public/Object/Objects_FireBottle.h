// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_FireBottle.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AObjects_FireBottle : public AObjects
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Area")
	TSubclassOf<class AArea> FireArea;

	virtual void Func_Destroy_Implementation() override;
};
