// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Effect/FGameEffectData.h"
#include "GameEffectManagerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTCC_API UGameEffectManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGameEffectManagerComponent();

	//로컬에서만 실행 (본인 전용)
	void PlayGameEffect_Local(const FGameEffectData& EffectData, const FGameEffectContext& Context, const FGameEffectRuntimeParams& RuntimeParams = FGameEffectRuntimeParams());

	//모든 클라이언트에서 실행
	void PlayGameEffect_Multicast(const FGameEffectData& EffectData, const FGameEffectContext& Context, const FGameEffectRuntimeParams& RuntimeParams = FGameEffectRuntimeParams());

protected:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayResolvedGameEffect(FGameEffectData EffectData, FVector_NetQuantize EffectLocation, FRotator EffectRotation, FVector EffectScale, FGameEffectRuntimeParams RuntimeParams);

	void SpawnGameEffectAtTransform_Local(const FGameEffectData& EffectData, const FTransform& EffectTransform, const FGameEffectRuntimeParams& RuntimeParams);

	FTransform ResolveGameEffectTransform(const FGameEffectData& EffectData, const FGameEffectContext& Context);
	
	void ApplyGameEffectRuntimeParams(UNiagaraComponent* NiagaraComp, const FGameEffectRuntimeParams& RuntimeParams);
};
