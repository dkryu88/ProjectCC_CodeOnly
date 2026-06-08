// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_AirRaidFlareGun.generated.h"

/**
 * 
 */
class APlayer_Character;
class UNiagaraSystem;

UCLASS()
class PROJECTCC_API AObjects_AirRaidFlareGun : public AObjects
{
	GENERATED_BODY()
	
public:
	AObjects_AirRaidFlareGun(const FObjectInitializer& ObjectInitializer);
	virtual void ApplyAdditionalSetting() override;

	void InitAirRaid(APlayer_Character* ownPlayer, const FVector& AttackForward);

	void BeginAirRaidFlare();
	
	//나이아가라 이펙트
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayExplosionCell(FVector Center, FRotator Rotation);
private:
	UPROPERTY()
	TObjectPtr<UNiagaraSystem> ExplosionFX = nullptr;

	FTimerHandle ExplosionDelayTimerhandle;
	FTimerHandle ExplosionIntervalTimerHandle;

	FIntPoint ForwardGridDirection = FIntPoint(1, 0);
	FIntPoint RightGridDirection = FIntPoint(0, 1);
	FIntPoint AirRaidStartGrid = FIntPoint(0, 0);

	FVector CapturedForward = FVector::ForwardVector;
	float CapturedYaw = 0.f;

	FVector FlareSpawnLocation = FVector::ZeroVector;
	FVector AirRaidStartLocation = FVector::ZeroVector;

	int32 CurrentRow = 0;

	UPROPERTY()
	float RowInterval = 0.15f;

	UPROPERTY()
	float ExplosionDelay = 2.f;

	UPROPERTY()
	float ExplosionDamage = 100.f;

	UPROPERTY()
	int32 RowCount = 7;

	UPROPERTY()
	int32 RangeInBlocks = 7;
	
	bool BuildAirRaidStartLocation();
	void StartAirRaid();
	void ExplodeNextRow();
};
