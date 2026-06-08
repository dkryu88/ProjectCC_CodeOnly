// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_GiantMagnet.generated.h"

/**
 * 
 */
class AMagnetCoin;
class UPlayerConditionDataAsset;

UCLASS()
class PROJECTCC_API AWeapon_GiantMagnet : public AWeapon
{
	GENERATED_BODY()
	
protected:
	// 공격이 적중했을 때 실행되는 함수 오버라이드
	virtual void HitEffect_Implementation(APlayer_Character* Player, AActor* Target) override;
	// 공격을 사용했을 때 실행되는 함수 오버라이드
	virtual bool UseEffect_Implementation(APlayer_Character* Player) override;

	//자석 코인 생성
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnMagnetCoins(int32 Amount, APlayer_Character* Player, APlayer_Character* Victim);

public:
	// 1원 코인
	UPROPERTY(EditDefaultsOnly, Category = "MagnetCoin")
	TSubclassOf<AMagnetCoin>MagnetCoin1;
	// 5원 코인
	UPROPERTY(EditDefaultsOnly, Category = "MagnetCoin")
	TSubclassOf<AMagnetCoin>MagnetCoin5;
	// 자석 컨디션
	UPROPERTY(EditAnywhere, BluePrintReadOnly, Category = "MagnetConditionDataAsset")
	TObjectPtr <UPlayerConditionDataAsset> MagnetConditionDataAsset;
	// 컨디션 지속시간
	UPROPERTY(EditAnywhere, BluePrintReadOnly, Category = "MagnetConditionDataAsset")
	float ConditionDuration = 3.f;
};
