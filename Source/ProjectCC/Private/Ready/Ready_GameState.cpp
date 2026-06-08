// Fill out your copyright notice in the Description page of Project Settings.


#include "Ready/Ready_GameState.h"
#include "Net/UnrealNetwork.h"

AReady_GameState::AReady_GameState()
{
	bReplicates = true;
}

void AReady_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReady_GameState, ConnectedPlayers);
	DOREPLIFETIME(AReady_GameState, SelectedMapDisplayName);
	DOREPLIFETIME(AReady_GameState, SelectedMapLevelName);
	DOREPLIFETIME(AReady_GameState, bAllPlayersReadyToTravel);
}

void AReady_GameState::SetConnectedPlayers(int32 NewConnectedPlayers)
{
	if (!HasAuthority()) return;

	ConnectedPlayers = NewConnectedPlayers;
	ForceNetUpdate();
}

void AReady_GameState::SetSelectedMapInfo(const FString& InDisplayName)
{
	if (!HasAuthority()) return;

	SelectedMapDisplayName = InDisplayName;
	ForceNetUpdate();
}

void AReady_GameState::SetAllPlayersReadyToTravel(bool bReady)
{
	if (!HasAuthority()) return;

	bAllPlayersReadyToTravel = bReady;
	ForceNetUpdate();
}

void AReady_GameState::SetMatchStartServerTime(float ServerTime)
{
	if (!HasAuthority()) return;

	MatchStartServerTime = ServerTime;
	ForceNetUpdate();
}

float AReady_GameState::GetCountdownRemaining() {
	if (MatchStartServerTime <= 0.f) return -1.f;

	float ServerNow = GetServerWorldTimeSeconds();
	return FMath::Max(0.f, MatchStartServerTime - ServerNow);
}