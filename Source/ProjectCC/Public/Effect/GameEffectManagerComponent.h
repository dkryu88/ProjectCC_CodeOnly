// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Effect/FGameEffectData.h"
#include "GameEffectManagerComponent.generated.h"

USTRUCT(BlueprintType)
struct FActiveTranslationFollowEffect {
	GENERATED_BODY()

	TWeakObjectPtr<UNiagaraComponent> NiagaraComp;
	TWeakObjectPtr<AActor> SourceActor;

	FVector InitialSourceLocation = FVector::ZeroVector;
	FVector InitialEffectLocation = FVector::ZeroVector;
	FRotator InitialEffectRotation = FRotator::ZeroRotator;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTCC_API UGameEffectManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGameEffectManagerComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//로컬에서만 실행 (본인 전용)
	void PlayGameEffect_Local(const FGameEffectData& EffectData, const FGameEffectContext& Context, const FGameEffectRuntimeParams& RuntimeParams = FGameEffectRuntimeParams());

	//모든 클라이언트에서 실행
	void PlayGameEffect_Multicast(const FGameEffectData& EffectData, const FGameEffectContext& Context, const FGameEffectRuntimeParams& RuntimeParams = FGameEffectRuntimeParams(), FName SoundKey = NAME_None);

	//이펙트의 루프 사운드를 Delay이후 정리(LifeTime의 경우)
	void StopEffectSoundAfterDelay(FName SoundKey, float Delay, float FadeOutTime);
protected:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayResolvedGameEffect(FGameEffectData EffectData, FVector_NetQuantize EffectLocation, FRotator EffectRotation, FVector EffectScale, FGameEffectRuntimeParams RuntimeParams, const FGameEffectContext& Context, FName SoundKey = NAME_None);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopEffectSound(FName SoundKey, float FadeOutTime);

	UPROPERTY()
	TMap<FName, TObjectPtr<UAudioComponent>> ActiveEffectSounds;

	TMap<FName, FTimerHandle> EffectSoundStopTimers;

	void StopEffectSound_Local(FName SoundKey, float FadeOutTime);

	void SpawnGameEffectAtTransform_Local(const FGameEffectData& EffectData, const FTransform& EffectTransform, const FGameEffectRuntimeParams& RuntimeParams, const FGameEffectContext& Context, FName SoundKey = NAME_None);

	FTransform ResolveGameEffectTransform(const FGameEffectData& EffectData, const FGameEffectContext& Context);
	
	void ApplyGameEffectRuntimeParams(UNiagaraComponent* NiagaraComp, const FGameEffectRuntimeParams& RuntimeParams);

	TArray<FActiveTranslationFollowEffect> ActiveTranslationFollowEffects;

	
};
