// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_ToyBlocks.h"
#include "Areas/Area_ToyBlocks.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "Components/CapsuleComponent.h"

bool AItem_ToyBlocks::UseEffect_Implementation(APlayer_Character* Player) {
	if (!Player || !ToyBlocks || !Player->NowMap) return false;
	if (!Player->HasAuthority()) return false;

	AMapConstructor* Map = Player->NowMap;
	FVector PlayerLocation = Player->GetActorLocation();
	FVector ForwardDir = Player->GetActorForwardVector().GetSafeNormal2D();
	FVector TargetBlock = PlayerLocation + ForwardDir * Map->BlockSize;
	FVector TargetLocation = Map->GetTopBlockLocationFromWorld(TargetBlock);
	if (TargetLocation == FVector(-1.f, -1.f, -1.f)) return false;

	PlayerLocation.Z -= Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	if (!(FMath::Abs(TargetLocation.Z - PlayerLocation.Z) <= Map->BlockSize + 5.f)) return false;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArea_ToyBlocks* NewTrap = GetWorld()->SpawnActor<AArea_ToyBlocks>(ToyBlocks, TargetLocation, FRotator::ZeroRotator, SpawnParams);
	if (NewTrap) {
		NewTrap->OwnPlayer = Player;
		return true;
	}

	return false;
}
