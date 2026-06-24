// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_PunchGun.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AWeapon_PunchGun : public AWeapon
{
	GENERATED_BODY()

public:
	AWeapon_PunchGun(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay();
	virtual bool BeforeAttackWeaponFunction() override;
	virtual void AdditionalUnEquipWeaponFunction() override;
	virtual void ApplyWorldState() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ApplyEquipState() override;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GloveMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> SpringMesh;

	FVector GloveBaseLocation;
	FVector SpringBaseScale;
	FVector SpringBaseLocation;

	float SpringMeshNaturalY = 0.f;
	float CachedLaunchDistance = 0.f;
	bool bIsAttacking = false;
	float AttackAlpha = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FRotator EquipRotation = FRotator::ZeroRotator;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySpringAttack(float LaunchDistance);

	UFUNCTION(Server, Reliable)
	void Server_TriggerSpringAttack();

	void PlaySpringAttack(float LaunchDistance);
	float GetGloveLaunchDistance();
	void OnTimelineUpdate(float Value);
	void OnTimelineFinished();

	FVector GetForwardMostPointOnMesh(UStaticMeshComponent* MeshComp, const FVector& ForwardDir);
};
