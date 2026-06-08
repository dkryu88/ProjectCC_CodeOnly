// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "MapObjects_Meteor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AMapObjects_Meteor : public AObjects
{
	GENERATED_BODY()
	
public:
	AMapObjects_Meteor(const FObjectInitializer& ObjectInitializer);
	virtual void ApplyAdditionalSetting() override;

private:
	UPROPERTY(EditDEfaultsOnly, Category = "Meteor")
	TSubclassOf<AArea> Area_FireArea;

	UPROPERTY(EditDefaultsOnly, Category = "Meteor")
	float DamageAmount = 40.f;

	UFUNCTION()
	void OnMeteorHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	bool bHasLanded = false;
	void OnLanded(const FHitResult& Hit);
};
