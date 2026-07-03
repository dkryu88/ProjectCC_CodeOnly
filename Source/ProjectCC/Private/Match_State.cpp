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
	DOREPLIFETIME(AMatch_State, ShopPriceData);
}


int32 AMatch_State::GetShopPrice(EShopBoxs Box)
{
	switch (Box) {
	case EShopBoxs::B_Box:		return ShopPriceData.B_BoxPrice;
	case EShopBoxs::A_Box:		return ShopPriceData.A_BoxPrice;
	case EShopBoxs::S_Box:		return ShopPriceData.S_BoxPrice;
	case EShopBoxs::Random_Box: return ShopPriceData.Random_BoxPrice;
	default:					return 999;
	}
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

void AMatch_State::SetShopPriceData(const FShopPriceData& NewData)
{
	if (!HasAuthority()) return;

	ShopPriceData = NewData;
	OnRep_ShopPriceData();
}

void AMatch_State::OnRep_MatchTime()
{
	OnMatchTimeChanged.Broadcast(MatchTime);
}

void AMatch_State::OnRep_ShopPriceData()
{
	//필요시 가격 변동 매치 이벤트 구현 가능
}
