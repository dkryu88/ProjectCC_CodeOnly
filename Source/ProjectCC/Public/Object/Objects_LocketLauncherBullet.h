// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_LocketLauncherBullet.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AObjects_LocketLauncherBullet : public AObjects
{
	GENERATED_BODY()
	
public:
	AObjects_LocketLauncherBullet(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void Func_Destroy_Implementation() override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayExplosionEffects(FVector ExplosionCenter, float Radius);

	UPROPERTY(EditAnywhere, Category = "CamereShake")
	TSubclassOf<UCameraShakeBase> ExplosionCameraShake;

	UPROPERTY(EditAnywhere, Category = "DamagaData")
	float DamageAmount = 70.f;
};
