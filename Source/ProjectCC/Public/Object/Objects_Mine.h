// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_Mine.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AObjects_Mine : public AObjects
{
	GENERATED_BODY()
	
public:
	AObjects_Mine(const FObjectInitializer& ObjectInitializer);

	virtual void ApplyAdditionalSetting() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mine")
	float DamageAmount = 30.f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	bool bIsExploded = false;
};
