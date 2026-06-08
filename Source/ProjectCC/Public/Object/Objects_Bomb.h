// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_Bomb.generated.h"

/**
 * 
 */
class AMapConstructor;

UCLASS()
class PROJECTCC_API AObjects_Bomb : public AObjects
{
	GENERATED_BODY()
	
public:
	AObjects_Bomb(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")
	float DamageAmount = 60.f;

	virtual void BeginPlay() override;
	virtual void Func_BecomeNormalType_Implementation(const FHitResult& Hit) override;
	virtual void Func_Destroy_Implementation() override;

private:
	//폭발 딜레이 타이머
	FTimerHandle ExplodeTimerHandle;
	//중복 폭발 방지
	bool bExploded = false;
	//중복 착지 방지
	bool bLanded = false;
	//현재 맵 데이터
	AMapConstructor* map;
	//폭발 기능
	void Explode();
};
