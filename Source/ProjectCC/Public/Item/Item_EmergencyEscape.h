// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_EmergencyEscape.generated.h"

/**
 * 
 */
class UPlayerConditionDataAsset;
class AMapConstructor;

UCLASS()
class PROJECTCC_API AItem_EmergencyEscape : public AItem
{
	GENERATED_BODY()
	
protected:
	//무적 지속시간 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConditionData")
	float InvincibilityDuration = 1.f;

	//무적 효과 데이터 에셋
	UPROPERTY(EditAnywhere, Category="ConditionData")
	TObjectPtr<UPlayerConditionDataAsset> InvincibilityDataAsset;

	//안전 블록 탐색 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConditionData")
	float MaxSafeSearchDistance = 1000.f;

	//아이템 효과에 적용된 캐릭터들의 정지 시간 (갑작스런 이동에 대한 인지 시간 마련)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConditionData")
	float InputBlockDuration = 0.3f;

public:
	virtual bool UseEffect_Implementation(class APlayer_Character* Player) override;

private:
	FVector GetSafeSwapLocation(const FVector& TargetLocation, AMapConstructor* Map, AActor* IgnoreItemUser, AActor* IgnoreTarget);
};
