// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagnetCoin.generated.h"

UCLASS()
class PROJECTCC_API AMagnetCoin : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AMagnetCoin();

	// 자석 코인 초기 설정(액수, 코인습득 플레이어)
	void SetupMagnetCoin(int32 value, class APlayer_Character* AttackPlayer);

protected:

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* MagnetCoinMesh;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 생성된 자석 코인을 습득 할 목표플레이어
	UPROPERTY()
	class APlayer_Character* TargetPlayer;

	int32 MagnetCoinValue = 0;
	float AliveTime = 0.f;
	FVector ThrowVelocity;

};
