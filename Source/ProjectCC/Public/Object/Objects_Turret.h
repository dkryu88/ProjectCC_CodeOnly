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
	AObjects_Turret(const FObjectInitializer& ObjectInitializer);

	virtual void ApplyAdditionalSetting() override;
	virtual void Func_Persist_Implementation(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	UBoxComponent* AttackRangeBox;

protected:
	UFUNCTION()
	void OnAttackBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void TryAttackTarget();

	// 터렛 기본데미지 10
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret")
	float Damage = 2.f;
};
