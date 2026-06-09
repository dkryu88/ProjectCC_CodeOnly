// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KillPlane.generated.h"

class UBoxComponent;
class UEffectManagerComponent;

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KillPlane")
	UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KillPlane")
	UBoxComponent* KillCollider;
	UPROPERTY()
	UEffectManagerComponent* EffectManagerComp;
	//KillCollider 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector SizeMagnification = FVector(1.f, 1.f, 1.f);
	//KillCollider 크기 보정값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector ColliderOffset = FVector(0.f, 0.f, 0.f);

public:
	void SetSizeofKillColliderwithMesh();
};
