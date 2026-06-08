// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_Shield.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AObjects_Shield : public AObjects
{
	GENERATED_BODY()
	
public:
	AObjects_Shield(const FObjectInitializer& ObjectInitializer);

	virtual void ApplyAdditionalSetting() override;
};
