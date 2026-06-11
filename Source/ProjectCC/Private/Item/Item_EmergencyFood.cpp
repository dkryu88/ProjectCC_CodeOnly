// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_EmergencyFood.h"
#include "Player_Character.h"
#include "ItemDataAsset.h"
#include "Player_State.h"
#include "Kismet/GameplayStatics.h"	// [사운드]

bool AItem_EmergencyFood::UseEffect_Implementation(APlayer_Character* Player) {
	if (!Player || !ItemData) return false;

	Player->HPChange(HealAmount);

	// [사운드]
	FVector EatLocation = Player->GetActorLocation() + FVector(0.f, 0.f, 150.f);
	Multicast_PlayEatSound(EatLocation);

	return true;
}

// [사운드] 멀티캐스트 재생
void AItem_EmergencyFood::Multicast_PlayEatSound_Implementation(FVector SpawnLocation)
{
	if (EatSound) {
		UGameplayStatics::PlaySoundAtLocation(this, EatSound, SpawnLocation, FRotator::ZeroRotator,1.f, 1.f, 0.f, nullptr);
	}
}
