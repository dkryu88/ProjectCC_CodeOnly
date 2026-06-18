// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_Lotto.h"
#include "Player_Character.h"
#include "Player_State.h"

bool AItem_Lotto::UseEffect_Implementation(APlayer_Character* Player)
{
	if (!Player || !ItemData) return false;
	if ((MinLottoCoin >= 0 && MaxLottoCoin < MinLottoCoin) || MinLottoCoin < 0) return false;
	
	int32 RewardCoin = FMath::RandRange(MinLottoCoin, MaxLottoCoin);

	if (APlayer_State* PS = Player->GetThePlayerState()) {

		PS->AddCoin(RewardCoin);
		UE_LOG(LogTemp, Warning, TEXT("[Server] Lotto Success! Coin: %d"), RewardCoin);
		return true;
	}

	return false;
}
