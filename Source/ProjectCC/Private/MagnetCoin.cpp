// Fill out your copyright notice in the Description page of Project Settings.


#include "MagnetCoin.h"
#include "Player_Character.h"
#include "EffectManagerComponent.h"
#include "Player_State.h"

// Sets default values
AMagnetCoin::AMagnetCoin()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MagnetCoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagnetCoinMesh"));
	SetRootComponent(MagnetCoinMesh);

	MagnetCoinMesh->SetCollisionProfileName(TEXT("NoCollision"));

	EffectManagerComp = CreateDefaultSubobject<UEffectManagerComponent>(TEXT("EffectManager"));
}

//Magnet Coin 최초 설정
void AMagnetCoin::SetupMagnetCoin(int32 value, APlayer_Character* AttackPlayer)
{
	MagnetCoinValue = value;
	TargetPlayer = AttackPlayer;

	ThrowVelocity = FVector(FMath::RandRange(-250.f, 250.f), FMath::RandRange(-250.f, 250.f), 150.f);

	SetLifeSpan(3.0f);
}

// Called every frame
void AMagnetCoin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!TargetPlayer) return;
	
	AliveTime += DeltaTime;
	FVector CurrentLocation = GetActorLocation();

	if (AliveTime < 0.4f) {
		ThrowVelocity.Z -= 980.f * DeltaTime;
		AddActorWorldOffset(ThrowVelocity * DeltaTime);
	}
	else {
		FVector TargetLocation = TargetPlayer->GetActorLocation();
		FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();

		float PullSpeed = 500.f + (AliveTime * 250.f);
		AddActorWorldOffset(Direction * PullSpeed * DeltaTime);

		if (FVector::Dist(CurrentLocation, TargetLocation) < 60.f) {
			if (HasAuthority() && TargetPlayer->GetThePlayerState()) {
				TargetPlayer->GetThePlayerState()->AddCoin(MagnetCoinValue);
			}
			Destroy();
		}
	}
}

