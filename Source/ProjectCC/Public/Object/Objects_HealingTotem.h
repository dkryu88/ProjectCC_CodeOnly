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
	virtual void BeginPlay() override;
	virtual void Func_Persist_Implementation(float DeltaTime) override;
	virtual void Func_Destroy_Implementation() override;
	virtual void Func_ZeroLife_Implementation() override;
	virtual void ApplyAdditionalSetting() override;
	virtual void ApplyInteractionState(APlayer_Character* InterActionPlayer) override;

	void UnifiedDestructionPath();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHealEffect();

	UPROPERTY(VisibleAnywhere, Category = "Healdata")
	class UBoxComponent* HealRangeBox;

	UPROPERTY(EditAnywhere, Category = "HealData")
	float HealRange = 3.f;

	UPROPERTY(EditAnywhere, Category = "HealData")
	float HealAmount = 5.f;

	UPROPERTY(EditAnywhere, Category = "Healing Totem")
	float HealResumeCoolTime = 10.f;

	UPROPERTY(EditAnywhere, Category = "Healing Totem")
	FName HealEffectName = TEXT("HealEffect");

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetLifeTimeEffectActive(bool bActive);

	bool bIsDestroyed = false;
	bool bHealPaused = false;
	bool bResumeCoolDown = false;

	FTimerHandle HealResumeCoolTimerHandle;

	void FinishHealResumeCoolDown();
	void PlayHealEffectOnPlayer(APlayer_Character* HealTarget);

	void UpdateLifeTimeEffectActive_Local();
};
