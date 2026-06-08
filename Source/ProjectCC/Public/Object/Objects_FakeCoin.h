// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_FakeCoin.generated.h"

/**
 * 
 */

UCLASS()
class PROJECTCC_API AObjects_FakeCoin : public AObjects
{
	GENERATED_BODY()
	
public:
	AObjects_FakeCoin(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void ApplyAdditionalSetting() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FakeCoin")
	float ColliderRadius = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FakeCoin")
	float DamageAmount = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FakeCoin")
	float SpinSpeed = -125.f;

private:
	bool bIsExploded = false;



	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool vFromSweep, const FHitResult& SweepResult);
};
