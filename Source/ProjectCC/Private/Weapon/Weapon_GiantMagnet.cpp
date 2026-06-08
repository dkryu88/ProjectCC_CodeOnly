// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_GiantMagnet.h"
#include "Player_Character.h"
#include "Player_State.h"
#include "MagnetCoin.h"
#include "Coin.h"
#include "TimerManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Conditions/PlayerConditionEffect_Magnetic.h"
#include "PlayerConditionComponent.h"

void AWeapon_GiantMagnet::HitEffect_Implementation(APlayer_Character* Player, AActor* Target)
{
	if (!HasAuthority() || !Player || !Target) return;

	APlayer_Character* Victim = Cast<APlayer_Character>(Target);
	if (Victim && Victim->GetThePlayerState()) {
		int32 CurrentCoin = Victim->GetThePlayerState()->GetPlayerCoin();
		int32 StealAmount = CurrentCoin / 2;

		if (StealAmount > 0) {
			Victim->GetThePlayerState()->AddCoin(-StealAmount);
			Multicast_SpawnMagnetCoins(StealAmount, Player, Victim);
		}
	}

	if (Player->ConditionComp && MagnetConditionDataAsset) {
		Player->ConditionComp->ApplyCondition(MagnetConditionDataAsset, Player, ConditionDuration);
	}
}

bool AWeapon_GiantMagnet::UseEffect_Implementation(APlayer_Character* Player)
{
	if (HasAuthority() && Player && Player->ConditionComp) {
		Player->ConditionComp->ApplyCondition(MagnetConditionDataAsset, Player, ConditionDuration);
	}
	return true;
}

//磊仿 内牢 积己
void AWeapon_GiantMagnet::Multicast_SpawnMagnetCoins_Implementation(int32 Amount, APlayer_Character* Player, APlayer_Character* Victim)
{
	if (!Player || !Victim) return;

	FVector SpawnLocation = Victim->GetActorLocation() + FVector(0.f, 0.f, float(FMath::RandRange(50.0, 75.0)));
	int32 Remaining = Amount;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	//5内牢 积己
	int32 Count5 = Remaining / 5;
	for (int32 i = 0; i < Count5; i++) {
		AMagnetCoin* NewMagnetCoin = GetWorld()->SpawnActor<AMagnetCoin>(MagnetCoin5, SpawnLocation, FRotator::ZeroRotator, Params);
		if (NewMagnetCoin) NewMagnetCoin->SetupMagnetCoin(5, Player);
	}

	//1内牢 积己
	int32 Count1 = Remaining % 5;
	for (int32 i = 0; i < Count1; i++) {
		AMagnetCoin* NewMagnetCoin = GetWorld()->SpawnActor<AMagnetCoin>(MagnetCoin1, SpawnLocation, FRotator::ZeroRotator, Params);
		if (NewMagnetCoin) NewMagnetCoin->SetupMagnetCoin(1, Player);
	}

}

