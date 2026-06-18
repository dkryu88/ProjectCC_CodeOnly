// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_Shield.h"
#include "Player_Character.h"
#include "Objects.h"

bool AItem_Shield::UseEffect_Implementation(APlayer_Character* Player) {
	if (!HasAuthority() || !Player || !Shield) return false;
	if (Player->NowSupport) {
		Player->NowSupport->Destroy();
		Player->NowSupport = nullptr;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Player;
	SpawnParams.Instigator = Player;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 플레이어 위치, 회전값으로 생성
	AObjects* NewShield = GetWorld()->SpawnActor<AObjects>(Shield, Player->GetActorLocation(), Player->GetActorRotation(), SpawnParams);
	if (NewShield) {
		if (Player->SupportSlot) {
			NewShield->OwnPlayer = Player;
			NewShield->EquipSupport(Player);
			Player->NowSupport = NewShield;
			return true;
		}
		else {
			NewShield->Destroy();
		}
	}
	return false;
}

