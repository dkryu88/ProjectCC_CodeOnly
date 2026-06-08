// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerConditions.h"
#include "PlayerConditionEffect.h"
#include "PlayerConditionDataAsset.h"
#include "PlayerConditionComponent.generated.h"

class APlayer_Character;

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

	UPROPERTY()
	APlayer_Character* Player = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Condition")
	TArray<FPlayerCondition> CurrentConditions;

	UPROPERTY()
	TMap<FName, float> MultiApplyIntervalMap;

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
};
