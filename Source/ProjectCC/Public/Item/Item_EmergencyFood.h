// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_EmergencyFood.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AItem_EmergencyFood : public AItem
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float HealAmount = 20.f;

	virtual bool UseEffect_Implementation(APlayer_Character* Player) override;
	
	// [사운드]=========================================
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<class USoundBase> EatSound;

	// [사운드] 멀티캐스트 재생 함수
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayEatSound(FVector SpawnLocation);

};
