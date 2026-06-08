// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Area.h"
#include "Area_FireArea.generated.h"

/**
 * 
 */
class UPlayerConditionDataAsset;
class APlayer_Character;
class UPlayerConditionComponent;

UCLASS()
class PROJECTCC_API AArea_FireArea : public AArea
{
	GENERATED_BODY()
	
public:
	AArea_FireArea();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Condition")
	TObjectPtr<UPlayerConditionDataAsset> BurnConditionData;

	virtual void ApplyInAreaEffect_Implementation(AActor* OtherActor) override;
	virtual void ApplyStayAreaEffect_Implementation(AActor* OtherActor) override;

private:
	void ApplyBurnToPlayer(AActor* OtherActor);
};
