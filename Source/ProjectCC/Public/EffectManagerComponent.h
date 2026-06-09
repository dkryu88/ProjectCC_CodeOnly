// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETC/FGameEffectData.h"
#include "EffectManagerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTCC_API UEffectManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEffectManagerComponent();
	virtual void BeginPlay() override;

	//1회성 이펙트
	void PlayOneTimeEffect(const FGameEffectData& Effect, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams = FGameEffectRuntimeParams());
	//루프 이펙트 시작
	void StartLoopEffect(FName EffectKey, const FGameEffectData& Effect, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams = FGameEffectRuntimeParams());
	//루프 이펙트 종료
	void StopLoopEffect(FName EffectKey);
	//루프 이펙트 갱신
	void RefreshLoopEffects();

public:
	UFUNCTION(NetMulticast, Unreliable)
	void Multi_PlayOneTimeEffect(const FGameEffectData& Effect, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams);

	UFUNCTION(NetMulticast, Reliable)
	void Multi_StartLoopEffect(FName EffectKey, const FGameEffectData& Effect, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams);

	UFUNCTION(NetMulticast, Reliable)
	void Multi_StopLoopEffect(FName EffectKey);

private:
	UPROPERTY()
	TMap<FName, TObjectPtr<UNiagaraComponent>> ActiveLoopEffects;

	UPROPERTY()
	TMap<FName, FGameEffectData> ActiveLoopEffectData;

	UPROPERTY()
	TMap<FName, TWeakObjectPtr<AActor>> ActiveLoopTargets;

	UPROPERTY()
	TMap<FName, FGameEffectRuntimeParams> ActiveLoopRuntimeParams;

	FTransform ResolveEffectTransform(const FGameEffectData& EffectData, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams);
	USceneComponent* ResolveAttachComponent(const FGameEffectData& Effect, AActor* TargetActor) const;
	void ApplyRuntimeParams(UNiagaraComponent* NiagaraComp, const FGameEffectRuntimeParams& RuntimeParams);
};
