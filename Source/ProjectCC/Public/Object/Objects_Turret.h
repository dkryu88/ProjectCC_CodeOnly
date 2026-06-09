// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_Turret.generated.h"

/**
 * 
 */
class UBoxComponent;

UCLASS()
class PROJECTCC_API AObjects_Turret : public AObjects
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret")
	USceneComponent* TurretHeadPivot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Turret")
	UStaticMeshComponent* TurretHeadMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Turret")
	TObjectPtr<UStaticMesh> TurretCollisionMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aim")
	float HeadTurnSpeed = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aim")
	float AttackAimHalfAngleDegree = 45.f;

	UPROPERTY()
	AActor* CurrentTarget = nullptr;

	AObjects_Turret(const FObjectInitializer& ObjectInitializer);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ApplyAdditionalSetting() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Func_Persist_Implementation(float DeltaTime) override;

	void UpdateHeadAim(float DeltaTime);
	bool IsHeadFacingTarget(AActor* Target);
	void FireAtTarget(AActor* Target);
	void ApplyTurretHeadYaw(float NewYaw);
	bool IsValidTurretTarget(AActor* Target);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	UBoxComponent* AttackRangeBox;

	UPROPERTY(ReplicatedUsing=OnRep_TurretHeadYaw)
	float TurretHeadYaw = 0.f;

protected:
	UFUNCTION()
	void OnAttackBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void TryAttackTarget();

	UFUNCTION()
	void OnRep_TurretHeadYaw();

	// 터렛 기본데미지 10
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret")
	float Damage = 2.f;
};
