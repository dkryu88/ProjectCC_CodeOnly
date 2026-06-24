// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerConditions.h"
#include "PlayerConditionEffect.h"
#include "PlayerConditionDataAsset.h"
#include "PlayerConditionComponent.generated.h"

class APlayer_Character;
class UNiagaraComponent;

USTRUCT()
struct FActiveConditionPersistEffect {
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> NiagaraComp = nullptr;

	UPROPERTY()
	FGameEffectData EffectData;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTCC_API UPlayerConditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerConditionComponent();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartConditionLoopEffect(FName ConditionName, FGameEffectData EffectData);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopConditionLoopEffect(FName ConditionName);

	UPROPERTY()
	APlayer_Character* Player = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Condition")
	TArray<FPlayerCondition> CurrentConditions;

	UPROPERTY()
	TMap<FName, float> MultiApplyIntervalMap;

	UPROPERTY()
	TMap<FName, FActiveConditionPersistEffect> ActiveConditionPersistEffects;

public:	
	void ApplyCondition(UPlayerConditionDataAsset* ConditionData, APlayer_Character* CauseCharacter, float CustomDuration, bool bUseCustomValue = false, float CustomValue = 0.f);
	void RemoveCondition(FName TargetConditionName);
	void RemoveSameNameCondition(FName TargetConditionName, bool bEndEffect);
	void RemoveSameCategoryCondition(EPlayerConditionType ConditionType, bool bEndEffect, int32 Priority);
	void RemoveLowPriorityCondition(int32 Priority, bool bEndEffect);
	bool CheckCondition(FName TargetConditionName);
	bool IgnoreDebuff(UPlayerConditionDataAsset* ConditionData) const;
	//Priority를 5로 넣으면 Priority완 상관없이 제거, 타입이 미지정되면 어떤 타입이든 현재 변신을 제거하는 기능으로 변경
	void TryRemoveTransform(int32 Priority, EPlayerConditionType Type, bool bEndEffect);

	bool TryGetVisualSlotForCondition(const FPlayerCondition& NewCondition, bool bEndEffect);
	bool CanTransformationGetVisualSlot(int32 NewPriority) const;

	void RemoveAnimationConditionsForTransformation(int32 NewPriority, bool bEndEffect);
	void ResumeCurrentConditionAnimation();

	void HandleConditionEvent(EPlayerConditionEvent Event, bool bUseEndEffect);

	void StartConditionLoopEffect_Local(FName ConditionName, const FGameEffectData& EffectData);
	void StopConditionLoopEffect_Local(FName ConditionName);
	bool HasSameConditionExceptIndex(FName ConditionName, int32 ExceptIndex);
	bool HasSameConditionByName(FName ConditionName);
	void RefreshConditionLoopEffectVisibility();

	void PlayConditionOnceEffect(const FGameEffectData& EffectData, const FPlayerCondition& Condition);
};
