// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Area.h"
#include "Area_RottenCheeseSlowArea.generated.h"

/**
 * 
 */

class UPlayerConditionDataAsset;
class APlayer_Character;
class UPlayerConditionComponent;

UCLASS()
class PROJECTCC_API AArea_RottenCheeseSlowArea : public AArea
{
	GENERATED_BODY()

public:
	AArea_RottenCheeseSlowArea();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	TObjectPtr<UPlayerConditionDataAsset> SlowConditionData;

	virtual void ApplyInAreaEffect_Implementation(AActor* OtherActor) override;
	virtual void ApplyStayAreaEffect_Implementation(AActor* OtherActor) override;

private:
	void ApplySlowToPlayer(AActor* OtherActor);
	
};
