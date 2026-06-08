// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_HealingTotem.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AObjects_HealingTotem : public AObjects
{
	GENERATED_BODY()

public:
	AObjects_HealingTotem(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void Func_Persist_Implementation(float DeltaTime) override;
	virtual void Func_Destroy_Implementation() override;
	virtual void Func_ZeroLife_Implementation() override;
	virtual void ApplyAdditionalSetting() override;

	void UnifiedDestructionPath();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHealEffect();

	UPROPERTY(VisibleAnywhere, Category = "Healdata")
	class UBoxComponent* HealRangeBox;

	UPROPERTY(EditAnywhere, Category = "HealData")
	float HealRange = 3.f;

	UPROPERTY(EditAnywhere, Category = "HealData")
	float HealAmount = 5.f;

	bool bIsDestroyed = false;
};
