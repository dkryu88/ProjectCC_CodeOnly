// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_SnowBomb.generated.h"

/**
 * 
 */
class UPlayerTransformationDataAsset;

UCLASS()
class PROJECTCC_API AObjects_SnowBomb : public AObjects
{
	GENERATED_BODY()

public:
	AObjects_SnowBomb(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SnowBomb")
	float DamageAmount = 10.f;

	UPROPERTY(EditAnywhere, Category = "SnowMan")
	TObjectPtr<UPlayerTransformationDataAsset> SnowManTransformationData;

	virtual void BeginPlay() override;
	virtual void Func_BecomeNormalType_Implementation(const FHitResult& Hit) override;
	virtual void Func_Destroy_Implementation() override;

	// [사운드]===========================================
	UPROPERTY(EditDefaultsOnly, Category="Sound")
	TObjectPtr<class USoundBase> FreezeSound;

	// 사운드 재생 멀티캐스트
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFreezeSound(FVector PlayLocation);

private:
	//폭발 딜레이 타이머
	FTimerHandle ExplodeTimerHandle;
	//중복 폭발 방지
	bool bExploded = false;
	//중복 착지 방지
	bool bLanded = false;
	//현재 맵 데이터
	AMapConstructor* map;
	//눈폭발 기능
	void SnowExplode();
};
