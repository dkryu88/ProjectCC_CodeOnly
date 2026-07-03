// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Effect/FGameEffectData.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "KillPlane.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UGameEffectManagerComponent;

enum class EKillPlaneEffectSurfaceAxis : uint8
{
	X,
	Y,
	Z
};

USTRUCT()
struct FKillPlaneSurfaceInfo
{
	GENERATED_BODY()

	EKillPlaneEffectSurfaceAxis SurfaceAxis = EKillPlaneEffectSurfaceAxis::Z;

	FVector LocalNormal = FVector::UpVector;
	FVector WorldNormal = FVector::UpVector;

	// +면이면 1, -면이면 -1
	float SurfaceValue = 1.f;
};

UCLASS()
class PROJECTCC_API AKillPlane : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKillPlane();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	//Overlap 처리 델리게이트
	UFUNCTION()
	void OnKillPlaneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	//상시 이펙트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect")
	TObjectPtr<UNiagaraComponent> LifeTimeEffectComp;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TObjectPtr<UGameEffectManagerComponent> EffectManagerComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<UAudioComponent> LifeTimeAudioComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> LifeTimeSound;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KillPlane")
	UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KillPlane")
	UBoxComponent* KillCollider;
	//KillCollider 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillPlane")
	FVector SizeMagnification = FVector(1.f, 1.f, 1.f);
	//KillCollider 크기 보정값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillPlane")
	FVector ColliderOffset = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effect")
	FGameEffectData KillPlaneDestroyEffect;

	UPROPERTY(EditDefaultsOnly, Category="Effect")
	float EffectMinPlaneSizeForShapeLocation = 25.f;

	UPROPERTY(EditDefaultsOnly, Category="Effect")
	float EffectPlaneSizeScale = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Effect")
	bool bSpawnEffectOnTopSurface = true;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float EffectSurfaceOffsetZ = 2.f;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> ProcessedActors;

	UPROPERTY(EditAnywhere, Category = "KillPlane|Sound")
	float DestroySoundCooldown = 0.15f;

	float LastDestroySoundTime = -10.f;
public:
	void SetSizeofKillColliderwithMesh();

	void ApplyLifeTimeEffectParams();

	void PlayDestroyEffectForActor(AActor* TargetActor, UPrimitiveComponent* TargetComp);

	FKillPlaneSurfaceInfo GetEffectSurfaceInfo(AActor* TargetActor);

	FVector GetEffectLocationFromSurface(AActor* TargetActor, const FKillPlaneSurfaceInfo& SurfaceInfo);
	FRotator GetEffectRotationFromSurface(const FKillPlaneSurfaceInfo& SurfaceInfo);
	FVector GetEffectPlaneScaleForShapeLocation(AActor* TargetActor, UPrimitiveComponent* TargetComp, EKillPlaneEffectSurfaceAxis SurfaceAxis);
};
