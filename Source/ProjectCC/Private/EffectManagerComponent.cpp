// Fill out your copyright notice in the Description page of Project Settings.


#include "EffectManagerComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "ETC/FGameEffectData.h"
#include "Player_Character.h"
#include "PlayerTransformationComponent.h"

// Sets default values for this component's properties
UEffectManagerComponent::UEffectManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEffectManagerComponent::BeginPlay() {
	Super::BeginPlay();
}

void UEffectManagerComponent::PlayOneTimeEffect(const FGameEffectData& Effect, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams)
{
	AActor* OwnActor = GetOwner();
	if (!OwnActor) return;

	if (!OwnActor->HasAuthority()) return;

	Multi_PlayOneTimeEffect(Effect, TargetActor, BaseLocation, BaseRotation, RuntimeParams);
}

void UEffectManagerComponent::StartLoopEffect(FName EffectKey, const FGameEffectData& Effect, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams)
{
	AActor* OwnActor = GetOwner();
	if (!OwnActor) return;

	if (!OwnActor->HasAuthority()) return;
	if (EffectKey.IsNone()) return;

	Multi_StartLoopEffect(EffectKey, Effect, TargetActor, BaseLocation, BaseRotation, RuntimeParams);
}

void UEffectManagerComponent::StopLoopEffect(FName EffectKey)
{
	AActor* OwnActor = GetOwner();
	if (!OwnActor) return;

	if (!OwnActor->HasAuthority()) return;
	if (EffectKey.IsNone()) return;

	Multi_StopLoopEffect(EffectKey);
}

void UEffectManagerComponent::RefreshLoopEffects()
{
	TArray<FName> Keys;
	ActiveLoopEffects.GetKeys(Keys);

	for (const FName& Key : Keys) {
		FGameEffectData* EffectData = ActiveLoopEffectData.Find(Key);
		TWeakObjectPtr<AActor>* TargetPtr = ActiveLoopTargets.Find(Key);
		FGameEffectRuntimeParams* RuntimeParams = ActiveLoopRuntimeParams.Find(Key);

		if (!EffectData || !TargetPtr || !RuntimeParams) continue;
		if (!TargetPtr->IsValid()) continue;

		if (TObjectPtr<UNiagaraComponent>* FoundComp = ActiveLoopEffects.Find(Key)) {
			if (FoundComp->Get()) {
				FoundComp->Get()->Deactivate();
				FoundComp->Get()->DestroyComponent();
			}
		}

		ActiveLoopEffects.Remove(Key);

		AActor* TargetActor = TargetPtr->Get();

		Multi_StartLoopEffect(Key, *EffectData, TargetActor, TargetActor->GetActorLocation(), TargetActor->GetActorRotation(), *RuntimeParams);
	}
}

FTransform UEffectManagerComponent::ResolveEffectTransform(const FGameEffectData& EffectData, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams)
{
	FVector Location = BaseLocation;
	FRotator Rotation = BaseRotation;

	if (IsValid(TargetActor)) {
		switch (EffectData.SpawnLocationType) {
		case EGameEffectSpawnLocationType::ActorLocation:
			Location = TargetActor->GetActorLocation();
			break;
		case EGameEffectSpawnLocationType::AttachToRoot:
		case EGameEffectSpawnLocationType::AttachToMesh:
			Location = TargetActor->GetActorLocation();
			break;
		case EGameEffectSpawnLocationType::AttachToSocket:
		{
			USceneComponent* AttachComp = ResolveAttachComponent(EffectData, TargetActor);

			if (AttachComp && !EffectData.SocketName.IsNone()) {
				if (UMeshComponent* Mesh = Cast<UMeshComponent>(AttachComp)) {
					if (Mesh->DoesSocketExist(EffectData.SocketName)) {
						Location = Mesh->GetSocketLocation(EffectData.SocketName);
						Rotation = Mesh->GetSocketRotation(EffectData.SocketName);
					}
				}
			}

			break;
		}
		case EGameEffectSpawnLocationType::AttackRangeStart: {
			FVector Forward = TargetActor->GetActorForwardVector();
			Forward.Z = 0.f;
			Forward = Forward.GetSafeNormal();

			float AttackRadius = 0.f;
			RuntimeParams.TryGetFloat(TEXT("User.AttackRadius"), AttackRadius);

			Location = TargetActor->GetActorLocation() + Forward * AttackRadius;
			break;
		}
		case EGameEffectSpawnLocationType::HitPoint:
			Location = BaseLocation;
			break;
		case EGameEffectSpawnLocationType::WorldLocation:
		default:
			Location = BaseLocation;
			break;
		}
	}

	switch (EffectData.SpawnRotationType) {
	case EGameEffectSpawnRotationType::ActorRotation:
		Rotation = TargetActor->GetActorRotation();
		break;

	case EGameEffectSpawnRotationType::ForwardRotation:
	{
		FVector Forward = TargetActor->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward = Forward.GetSafeNormal();

		Rotation = Forward.Rotation();
		break;
	}
	case EGameEffectSpawnRotationType::HitNormalRotation:
	{
		FVector HitNormal = FVector::ZeroVector;
		RuntimeParams.TryGetVector(TEXT("User.HitNormal"), HitNormal);

		Rotation = HitNormal.IsNearlyZero() ? BaseRotation : HitNormal.Rotation();
		break;
	}
	case EGameEffectSpawnRotationType::SocketRotation:
		break;
	case EGameEffectSpawnRotationType::WorldRotation:
	default:
		Rotation = BaseRotation;
		break;
	}


	Location += Rotation.RotateVector(EffectData.LocationOffset);
	Rotation += EffectData.RotationOffset;

	return FTransform(Rotation, Location, EffectData.Scale);
}

USceneComponent* UEffectManagerComponent::ResolveAttachComponent(const FGameEffectData& Effect, AActor* TargetActor) const
{
	if (!IsValid(TargetActor)) return nullptr;

	if (APlayer_Character* Player = Cast<APlayer_Character>(TargetActor)) {
		if (Player->TransformationComp) {
			TArray<UMeshComponent*> TransformMeshes;
			Player->TransformationComp->GetActiveTransformationMeshes(TransformMeshes);

			for (UMeshComponent* Mesh : TransformMeshes) {
				if (IsValid(Mesh) && Mesh->IsVisible()) {
					return Mesh;
				}
			}
		}

		if (Player->GetMesh()) return Player->GetMesh();

		return Player->GetRootComponent();
	}

	if (Effect.SpawnLocationType == EGameEffectSpawnLocationType::AttachToMesh || Effect.SpawnLocationType == EGameEffectSpawnLocationType::AttachToSocket) {
		TArray<UMeshComponent*> Meshes;

		TargetActor->GetComponents<UMeshComponent>(Meshes);

		for (UMeshComponent* Mesh : Meshes) {
			if (IsValid(Mesh)) return Mesh;
		}
	}

	return TargetActor->GetRootComponent();
}

void UEffectManagerComponent::ApplyRuntimeParams(UNiagaraComponent* NiagaraComp, const FGameEffectRuntimeParams& RuntimeParams)
{
	if (!NiagaraComp) return;

	for (const FGameEffectFloatParam& Param : RuntimeParams.FloatParams) {
		if (!Param.ParamName.IsNone()) {
			NiagaraComp->SetFloatParameter(Param.ParamName, Param.Value);
		}
	}

	for (const FGameEffectVectorParam& Param : RuntimeParams.VectorParams) {
		if (!Param.ParamName.IsNone()) {
			NiagaraComp->SetVectorParameter(Param.ParamName, Param.Value);
		}
	}

	for (const FGameEffectBoolParam& Param : RuntimeParams.BoolParams) {
		if (!Param.ParamName.IsNone()) {
			NiagaraComp->SetBoolParameter(Param.ParamName, Param.Value);
		}
	}
}

void UEffectManagerComponent::Multi_StopLoopEffect_Implementation(FName EffectKey)
{
	if (EffectKey.IsNone()) return;
	if (TObjectPtr<UNiagaraComponent>* FoundComp = ActiveLoopEffects.Find(EffectKey)) {
		if (FoundComp->Get()) {
			FoundComp->Get()->Deactivate();
			FoundComp->Get()->DestroyComponent();
		}
	}

	ActiveLoopEffects.Remove(EffectKey);
	ActiveLoopEffectData.Remove(EffectKey);
	ActiveLoopTargets.Remove(EffectKey);
	ActiveLoopRuntimeParams.Remove(EffectKey);
}

void UEffectManagerComponent::Multi_StartLoopEffect_Implementation(FName EffectKey, const FGameEffectData& Effect, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams)
{
	if (EffectKey.IsNone()) return;
	if (!Effect.NiagaraEffect) return;

	if (ActiveLoopEffects.Contains(EffectKey)) Multi_StopLoopEffect(EffectKey);
	FTransform FinalTransform = ResolveEffectTransform(Effect, TargetActor, BaseLocation, BaseRotation, RuntimeParams);

	UNiagaraComponent* NiagaraComp = nullptr;

	if (Effect.bAttach) {
		USceneComponent* AttachComp = ResolveAttachComponent(Effect, TargetActor);

		if (AttachComp) {
			NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(Effect.NiagaraEffect, AttachComp, Effect.SocketName, FinalTransform.GetLocation(), FinalTransform.Rotator(), EAttachLocation::KeepWorldPosition, false, true);
		}
	}
	else {
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect.NiagaraEffect, FinalTransform.GetLocation(), FinalTransform.Rotator(), Effect.Scale, false, true);
	}

	if (!NiagaraComp) return;

	NiagaraComp->SetWorldScale3D(Effect.Scale);
	ApplyRuntimeParams(NiagaraComp, RuntimeParams);

	ActiveLoopEffects.Add(EffectKey, NiagaraComp);
	ActiveLoopEffectData.Add(EffectKey, Effect);
	ActiveLoopTargets.Add(EffectKey, TargetActor);
	ActiveLoopRuntimeParams.Add(EffectKey, RuntimeParams);
}

void UEffectManagerComponent::Multi_PlayOneTimeEffect_Implementation(const FGameEffectData& Effect, AActor* TargetActor, const FVector& BaseLocation, const FRotator& BaseRotation, const FGameEffectRuntimeParams& RuntimeParams)
{
	if (!Effect.NiagaraEffect && !Effect.Sound) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FTransform FinalTransform = ResolveEffectTransform(Effect, TargetActor, BaseLocation, BaseRotation, RuntimeParams);

	if (Effect.NiagaraEffect) {
		UNiagaraComponent* NiagaraComp = nullptr;

		if (Effect.bAttach) {
			USceneComponent* AttachComp = ResolveAttachComponent(Effect, TargetActor);
			if (AttachComp) {
				NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(Effect.NiagaraEffect, AttachComp, Effect.SocketName, FinalTransform.GetLocation(), FinalTransform.Rotator(), EAttachLocation::KeepWorldPosition, Effect.bAutoDestroy, true);
			}
		}
		else {
			NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Effect.NiagaraEffect, FinalTransform.GetLocation(), FinalTransform.Rotator(), Effect.Scale, Effect.bAutoDestroy, true);
		}
	}

	if (Effect.Sound) {
		UGameplayStatics::PlaySoundAtLocation(World, Effect.Sound, FinalTransform.GetLocation());
	}
}


