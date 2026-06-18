// Fill out your copyright notice in the Description page of Project Settings.


#include "Effect/GameEffectManagerComponent.h"
#include "Effect/FGameEffectData.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UGameEffectManagerComponent::UGameEffectManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGameEffectManagerComponent::PlayGameEffect_Local(const FGameEffectData& EffectData, const FGameEffectContext& Context, const FGameEffectRuntimeParams& RuntimeParams)
{
	FTransform EffectTransform = ResolveGameEffectTransform(EffectData, Context);
	SpawnGameEffectAtTransform_Local(EffectData, EffectTransform, RuntimeParams);
}

void UGameEffectManagerComponent::PlayGameEffect_Multicast(const FGameEffectData& EffectData, const FGameEffectContext& Context, const FGameEffectRuntimeParams& RuntimeParams)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	//서버 권한이 없다는 건 로컬이라는 뜻이므로 로컬 전용 이펙트 처리는 PlayGameEffect_Local에서 처리하고 여기서는 Return
	if (!OwnerActor->HasAuthority()) return;

	FTransform EffectTransform = ResolveGameEffectTransform(EffectData, Context);

	Multicast_PlayResolvedGameEffect(EffectData, EffectTransform.GetLocation(), EffectTransform.GetRotation().Rotator(), EffectTransform.GetScale3D(), RuntimeParams);
}

void UGameEffectManagerComponent::Multicast_PlayResolvedGameEffect_Implementation(FGameEffectData EffectData, FVector_NetQuantize EffectLocation, FRotator EffectRotation, FVector EffectScale, FGameEffectRuntimeParams RuntimeParams)
{
	FTransform EffectTransform(EffectRotation, EffectLocation, EffectScale);
	SpawnGameEffectAtTransform_Local(EffectData, EffectTransform, RuntimeParams);
}

void UGameEffectManagerComponent::SpawnGameEffectAtTransform_Local(const FGameEffectData& EffectData, const FTransform& EffectTransform, const FGameEffectRuntimeParams& RuntimeParams)
{
	if (!GetWorld()) return;

	bool bHavingNiagara = EffectData.NiagaraEffect != nullptr;
	bool bHavingSound = EffectData.Sound != nullptr;

	if (!bHavingNiagara && !bHavingSound) return;

	if (bHavingNiagara) {
		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), EffectData.NiagaraEffect, EffectTransform.GetLocation(), EffectTransform.GetRotation().Rotator(), EffectTransform.GetScale3D(), true, true);

		if (NiagaraComp) ApplyGameEffectRuntimeParams(NiagaraComp, RuntimeParams);
	}

	if (bHavingSound) {
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), EffectData.Sound, EffectTransform.GetLocation(), EffectTransform.GetRotation().Rotator());
	}
}

FTransform UGameEffectManagerComponent::ResolveGameEffectTransform(const FGameEffectData& EffectData, const FGameEffectContext& Context)
{
	AActor* OwnerActor = Context.SourceActor ? Context.SourceActor.Get() : GetOwner();
	USceneComponent* SourceComp = Context.SourceComponent.Get();

	if (!SourceComp && OwnerActor) SourceComp = OwnerActor->GetRootComponent();

	FVector FinalLocation = Context.WorldLocation;
	FRotator FinalRotation = Context.WorldRotation;

	FName SocketName = Context.OverrideSocketName.IsNone() ? EffectData.SocketName : Context.OverrideSocketName;

	switch (EffectData.SpawnLocationType) {
	case EGameEffectSpawnLocationType::WorldLocation:
		FinalLocation = Context.WorldLocation;
		break;

	case EGameEffectSpawnLocationType::ActorLocation:
		FinalLocation = OwnerActor ? OwnerActor->GetActorLocation() : Context.WorldLocation;
		break;

	case EGameEffectSpawnLocationType::SocketLocation:
		if (SourceComp && !SocketName.IsNone()) {
			FinalLocation = SourceComp->GetSocketLocation(SocketName);
		}
		else {
			FinalLocation = OwnerActor ? OwnerActor->GetActorLocation() : Context.WorldLocation;
		}
		break;	//누락
	case EGameEffectSpawnLocationType::AttackRangeStart:
		FinalLocation = Context.AttackRangeStart;
		break;
	case EGameEffectSpawnLocationType::HitPoint:
		FinalLocation = Context.HitPoint;
		break;
	default:
		FinalLocation = Context.WorldLocation;
		break;
	}

	switch (EffectData.SpawnRotationType) {
	case EGameEffectSpawnRotationType::WorldRotation:
		FinalRotation = Context.WorldRotation;
		break;
	case EGameEffectSpawnRotationType::ActorRotation:
		FinalRotation = OwnerActor ? OwnerActor->GetActorRotation() : Context.WorldRotation;
		break;
	case EGameEffectSpawnRotationType::SocketRotation:
		if (SourceComp && !SocketName.IsNone()) {
			FinalRotation = SourceComp->GetSocketRotation(SocketName);
		}
		else {
			FinalRotation = OwnerActor ? OwnerActor->GetActorRotation() : Context.WorldRotation;
		}
		break;
	case EGameEffectSpawnRotationType::ForwardRotation:
		if (OwnerActor) {
			FinalRotation = OwnerActor->GetActorForwardVector().Rotation();
		}
		else {
			FinalRotation = Context.WorldRotation;
		}
	case EGameEffectSpawnRotationType::HitNormalRotation:
		FinalRotation = Context.HitNormal.Rotation();
		break;
	default:
		FinalRotation = Context.WorldRotation;
		break;
	}
	
	FinalRotation += EffectData.RotationOffset;
	FinalLocation += FinalRotation.RotateVector(EffectData.LocationOffset);

	return FTransform(FinalRotation, FinalLocation, EffectData.Scale);

}

void UGameEffectManagerComponent::ApplyGameEffectRuntimeParams(UNiagaraComponent* NiagaraComp, const FGameEffectRuntimeParams& RuntimeParams)
{
	if (!NiagaraComp) return;

	for (const FGameEffectFloatParam& Param : RuntimeParams.FloatParams) {
		if (!Param.ParamName.IsNone()) {
			NiagaraComp->SetVariableFloat(Param.ParamName, Param.Value);
		}
	}

	for (const FGameEffectVectorParam& Param : RuntimeParams.VectorParams) {
		if (!Param.ParamName.IsNone()) {
			NiagaraComp->SetVariableVec3(Param.ParamName, Param.Value);
		}
	}

	for (const FGameEffectBoolParam& Param : RuntimeParams.BoolParams) {
		if (!Param.ParamName.IsNone()) {
			NiagaraComp->SetVariableBool(Param.ParamName, Param.Value);
		}
	}
}

