// Fill out your copyright notice in the Description page of Project Settings.


#include "Match_State.h"
#include "Net/UnrealNetwork.h"

AMatch_State::AMatch_State()
{
	bReplicates = true;
}

void AMatch_State::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMatch_State, MatchTime);
	DOREPLIFETIME(AMatch_State, bMatchStarted);
	DOREPLIFETIME(AMatch_State, bMatchEnded);
	DOREPLIFETIME(AMatch_State, DelayEndServerTime);
}

void AMatch_State::SetDelayEndServerTime(float Time)
{
	DelayEndServerTime = Time;
}

void AMatch_State::SetMatchTime(int32 NewMatchTime)
{
	MatchTime = NewMatchTime;
	OnMatchTimeChanged.Broadcast(MatchTime);
}

void AMatch_State::SetMatchStarted(bool bMatchState)
{
	bMatchStarted = bMatchState;
}

void AMatch_State::SetMatchEnded(bool bMatchState)
{
	bMatchEnded = bMatchState;
}

void AMatch_State::OnRep_MatchTime()
{
	OnMatchTimeChanged.Broadcast(MatchTime);
}


