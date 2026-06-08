// Fill out your copyright notice in the Description page of Project Settings.


#include "Player_State.h"
#include "Item.h"
#include "Weapon.h"
#include "Net/UnrealNetwork.h"

APlayer_State::APlayer_State()
{
	bReplicates = true;
}

void APlayer_State::CopyProperties(APlayerState* NewPlayerState)
{
	Super::CopyProperties(NewPlayerState);

	APlayer_State* NewPS = Cast<APlayer_State>(NewPlayerState);
	if (!NewPS) return;

	NewPS->PlayerCoin = PlayerCoin;
	NewPS->PlayerRank = PlayerRank;
	NewPS->PlayerEliminate = PlayerEliminate;
	NewPS->PlayerOut = PlayerOut;
	NewPS->ItemUseCount = ItemUseCount;
	NewPS->StartSpawnSlotIndex = StartSpawnSlotIndex;

	NewPS->EquippedItem = EquippedItem;
	NewPS->ReservedWeapon = ReservedWeapon;

	NewPS->NickName = NickName;
	NewPS->PortraitId = PortraitId;
	NewPS->ReadySyncState = ReadySyncState;
	NewPS->bMatchLevelLoaded = bMatchLevelLoaded;
}

void APlayer_State::OverrideWith(APlayerState* OldPlayerState)
{
	Super::OverrideWith(OldPlayerState);

	APlayer_State* OldPS = Cast<APlayer_State>(OldPlayerState);
	if (!OldPS) return;

	PlayerCoin = OldPS->PlayerCoin;
	PlayerRank = OldPS->PlayerRank;
	PlayerEliminate = OldPS->PlayerEliminate;
	PlayerOut = OldPS->PlayerOut;
	ItemUseCount = OldPS->ItemUseCount;
	StartSpawnSlotIndex = OldPS->StartSpawnSlotIndex;

	EquippedItem = OldPS->EquippedItem;
	ReservedWeapon = OldPS->ReservedWeapon;

	NickName = OldPS->NickName;
	PortraitId = OldPS->PortraitId;
	ReadySyncState = OldPS->ReadySyncState;
	bMatchLevelLoaded = OldPS->bMatchLevelLoaded;
}

void APlayer_State::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APlayer_State, PlayerCoin);
	DOREPLIFETIME(APlayer_State, PlayerRank);
	DOREPLIFETIME(APlayer_State, PlayerEliminate);
	DOREPLIFETIME(APlayer_State, PlayerOut);
	DOREPLIFETIME(APlayer_State, ItemUseCount);
	DOREPLIFETIME(APlayer_State, StartSpawnSlotIndex);

	DOREPLIFETIME(APlayer_State, EquippedItem);
	DOREPLIFETIME(APlayer_State, ReservedWeapon);

	DOREPLIFETIME(APlayer_State, NickName);
	DOREPLIFETIME(APlayer_State, PortraitId);
	DOREPLIFETIME(APlayer_State, ReadySyncState);
	DOREPLIFETIME(APlayer_State, bMatchLevelLoaded);
}

void APlayer_State::AddCoin(int32 CoinValue)
{
	if (!HasAuthority()) return;

	PlayerCoin += CoinValue;
	OnCoinChanged.Broadcast(PlayerCoin);
	ForceNetUpdate();
}

void APlayer_State::SetPlayerCoin(int32 coin)
{
	if (!HasAuthority()) return;

	PlayerCoin = coin;
	OnCoinChanged.Broadcast(PlayerCoin);
	ForceNetUpdate();
}

void APlayer_State::SetPlayerRank(int32 rank)
{
	if (!HasAuthority()) return;

	PlayerRank = rank;
	OnRankChanged.Broadcast(PlayerRank);
	ForceNetUpdate();
}

void APlayer_State::SetPlayerEliminate(int32 Eliminate)
{
	if (!HasAuthority()) return;

	PlayerEliminate = Eliminate;
	OnEliminateChanged.Broadcast(PlayerEliminate);

	ForceNetUpdate();
}

void APlayer_State::SetPlayerOut(int32 Out)
{
	if (!HasAuthority()) return;

	PlayerOut = Out;
	OnOutChanged.Broadcast(PlayerOut);

	ForceNetUpdate();
}

void APlayer_State::SetEquippedItem(TSubclassOf<AItem> TheItem, int32 TheUseCount)
{
	if (!HasAuthority()) return;

	EquippedItem = TheItem;
	ItemUseCount = TheUseCount;

	OnItemChanged.Broadcast(EquippedItem, ItemUseCount);
	ForceNetUpdate();
}

void APlayer_State::SetNickName(const FString& nickname)
{
	if (!HasAuthority()) return;

	NickName = nickname;

	OnNicknameChanged.Broadcast(NickName);
	ForceNetUpdate();
}

void APlayer_State::SetPortraitId(int32 portraitId)
{
	if (!HasAuthority()) return;

	if (PortraitId == portraitId) {
		OnPortraitIdChanged.Broadcast(PortraitId);
		ForceNetUpdate();
		return;
	}

	PortraitId = portraitId;

	OnPortraitIdChanged.Broadcast(PortraitId);
	ForceNetUpdate();
}

void APlayer_State::SetReadySyncState(EReadySyncState State)
{
	if (!HasAuthority()) return;

	if (ReadySyncState == State) return;

	ReadySyncState = State;
	ForceNetUpdate();
}

void APlayer_State::SetMatchLevelLoaded(bool bLoaded)
{
	if (!HasAuthority()) return;
	bMatchLevelLoaded = bLoaded;
}

void APlayer_State::SetSpawnSlotIndex(int32 Index)
{
	StartSpawnSlotIndex = Index;
}

void APlayer_State::SetReservedWeapon(TSubclassOf<AWeapon> weapon)
{
	if (!HasAuthority()) return;
	ReservedWeapon = weapon;
	ForceNetUpdate();
}

void APlayer_State::ClearEquippedItem()
{
	if (!HasAuthority()) return;
	EquippedItem = nullptr;
	ItemUseCount = 0;

	OnItemChanged.Broadcast(EquippedItem, ItemUseCount);
	ForceNetUpdate();
}

void APlayer_State::ClearReservedWeapon()
{
	if (!HasAuthority()) return;
	ReservedWeapon = nullptr;
	ForceNetUpdate();
}

void APlayer_State::AddPlayerEliminate()
{
	if (!HasAuthority()) return;
	OnEliminateChanged.Broadcast(PlayerEliminate);
	PlayerEliminate += 1;
}

void APlayer_State::AddPlayerOut()
{
	if (!HasAuthority()) return;
	OnOutChanged.Broadcast(PlayerOut);
	PlayerOut += 1;
}

//UI 갱신을 위한 OnRep
void APlayer_State::OnRep_PlayerNickname()
{
	OnNicknameChanged.Broadcast(NickName);
}

void APlayer_State::OnRep_PlayerCoin()
{
	OnCoinChanged.Broadcast(PlayerCoin);
}

void APlayer_State::OnRep_PlayerRank()
{
	OnRankChanged.Broadcast(PlayerRank);
}

void APlayer_State::OnRep_Eliminate()
{
	OnEliminateChanged.Broadcast(PlayerEliminate);
}

void APlayer_State::OnRep_Out()
{
	OnOutChanged.Broadcast(PlayerOut);
}

void APlayer_State::OnRep_EquippedItem()
{
	OnItemChanged.Broadcast(EquippedItem, ItemUseCount);
}

void APlayer_State::OnRep_PortraitId()
{
	OnPortraitIdChanged.Broadcast(PortraitId);
}
