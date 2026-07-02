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

USTRUCT()
struct FActiveConditionOverlayEffect {
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OverlayMID = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SourceMaterial = nullptr;

	UPROPERTY()
	int32 Priority = 0;
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
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartConditionLoopEffect(FName ConditionName, FGameEffectData EffectData, FGameEffectRuntimeParams RuntimeParams = FGameEffectRuntimeParams());

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopConditionLoopEffect(FName ConditionName);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartConditionOverlay(FName ConditionName, UMaterialInterface* OverlayMaterial, FName OpacityParamName, int32 Priority);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopConditionOverlay(FName ConditionName, float FadeOutTime);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RefreshConditionOverlayVisual();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ClearAllConditionVisualWhenPlayerOut();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayConditionOnceSound(USoundBase* Sound);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartConditionPersistSound(FName ConditionName, USoundBase* Sound);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopConditionPersistSound(FName ConditionName, float FadeOutTime);

protected:
	UPROPERTY(Transient)
	TMap<FName, FActiveConditionOverlayEffect> ActiveConditionOverlays;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UAudioComponent>> ActiveConditionPersistSounds;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultOverlayMaterial = nullptr;

	UPROPERTY(Transient)
	FName CurrentOverlayConditionName;

	UPROPERTY()
	APlayer_Character* Player = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Condition")
	TArray<FPlayerCondition> CurrentConditions;

	UPROPERTY()
	TMap<FName, float> MultiApplyIntervalMap;

	UPROPERTY()
	TMap<FName, FActiveConditionPersistEffect> ActiveConditionPersistEffects;

public:
	FTimerHandle OverlayFadeTimerHandle;

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

	void ClearAllConditionWhenPlayerOut();
	void ClearAllConditionVisualsWhenPlayerOut_Local();

	void HandleConditionEvent(EPlayerConditionEvent Event, bool bUseEndEffect);

	void StartConditionLoopEffect_Local(FName ConditionName, const FGameEffectData& EffectData, const FGameEffectRuntimeParams& RuntimeParams);
	void StopConditionLoopEffect_Local(FName ConditionName);
	void ApplyRuntimeParams(UNiagaraComponent* NiagaraComp, const FGameEffectRuntimeParams& RuntimeParams);
	bool HasSameConditionExceptIndex(FName ConditionName, int32 ExceptIndex);
	bool HasSameConditionByName(FName ConditionName);
	void RefreshConditionLoopEffectVisibility();

	void StartConditionOverlay_Local(FName ConditionName, UMaterialInterface* OverlayMaterial, FName OpacityParamName, int32 Priority);
	void StopConditionOverlay_Local(FName ConditionName, float FadeOutTime);
	void RefreshConditionOverlay_Local();
	void ApplyHighestConditionOverlay_Local();
	void NotifyTransformationOverlayChanged_Local();
	bool ShouldNoShowingConditionOverlayByTransformation_Local();

	void PlayConditionStartSound(const FPlayerCondition& Condition);
	void PlayConditionEndSound(const FPlayerCondition& Condition);

	void StartConditionPersistSound_Local(FName ConditionName, USoundBase* Sound);
	void StopConditionPersistSound_Local(FName ConditionName, float FadeOutTime);

	void PlayConditionOnceEffect(const FGameEffectData& EffectData, const FPlayerCondition& Condition);
};
