// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageBlock.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AMapConstructor;

UCLASS()
class PROJECTCC_API ADamageBlock : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADamageBlock();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* BlockCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* DamageCollider;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	float DamageAmount = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	float DamageInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	float DamageCheckPeriod = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	float TriggerPadding = 5.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Damage")
	float BlockSize = 100.f;

	UPROPERTY()
	TWeakObjectPtr<AMapConstructor> NowMap;

	UPROPERTY()
	FIntVector GridLocation = FIntVector::ZeroValue;

	FTimerHandle DamageTickTimerHandle;

	TMap<TWeakObjectPtr<AActor>, float> ActorsDamageIntervals;

protected:
	
	UFUNCTION()
	void OnDamageTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDamageTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void UpdateOverlappingDamageTargets();

	UFUNCTION()
	void RemoveFromMap();

public:	
	void InitializeDamageBlock(float InBlockSize, AMapConstructor* Map, const FIntVector& Grid);

	bool CanDamageActor(AActor* TargetActor);
	void ApplyDamageToActor(AActor* OtherActor);

	FIntVector GetGridLocation() { return GridLocation; }
	AMapConstructor* GetNowMap() { return NowMap.Get(); }
};
