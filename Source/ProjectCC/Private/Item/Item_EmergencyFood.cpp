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
	/*FVector EatLocation = Player->GetActorLocation() + FVector(0.f, 0.f, 150.f);
	Multicast_PlayEatSound(EatLocation);*/

	// 플레이어에서 소리가 나도록 위임, 횟수가 1에서 0으로 바뀌어 바로 destroy되어도 사운드는 잘 재생됨
	// ItemUseSound는 Item.h에서 선언된 객체 참조, 비상식량 블루프린트에서 사운드큐 설정해야 함.
	if (Player->HasAuthority()) {
		Player->Multicast_PlayItemUseSound(ItemUseSound);
	}
	
	return true;
}

// [사운드] 멀티캐스트 재생
//void AItem_EmergencyFood::Multicast_PlayEatSound_Implementation(FVector SpawnLocation)
//{
//	if (EatSound) {
//		UGameplayStatics::PlaySoundAtLocation(this, EatSound, SpawnLocation, FRotator::ZeroRotator,1.f, 1.f, 0.f, nullptr);
//	}
//}
