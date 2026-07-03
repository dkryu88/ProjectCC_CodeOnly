// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_Lotto.h"
#include "Player_Character.h"
#include "ItemDataAsset.h"
#include "Player_State.h"

bool AItem_Lotto::UseEffect_Implementation(APlayer_Character* Player)
{
	if (!Player || !ItemData) return false;
	if ((MinLottoCoin >= 0 && MaxLottoCoin < MinLottoCoin) || MinLottoCoin < 0) return false;
	
	int32 RewardCoin = FMath::RandRange(MinLottoCoin, MaxLottoCoin);
	float RewardLevel = 0;

	if (RewardCoin == 0) RewardLevel = 0.5f;
	else if (RewardCoin < 10) RewardLevel = 1.5f;
	else if (RewardCoin >= 10) RewardLevel = 2.5f;

	if (APlayer_State* PS = Player->GetThePlayerState()) {

		PS->AddCoin(RewardCoin);

		if (Player->EffectManagerComp && RewardLevel >= 0.f && RewardLevel < 3.f) {
			FGameEffectData* CoinGetEffect = ItemData->CustomEffects.Find(FName(TEXT("LottoUseEffect")));
			if (CoinGetEffect) {
				FGameEffectData CoinGetLocalEffect = *CoinGetEffect;

				FGameEffectContext Context;
				FGameEffectRuntimeParams Params;

				Context.SourceActor = Player;
				Context.SourceComponent = Player->GetMesh();
				Context.WorldLocation = Player->GetActorLocation();
				Context.WorldRotation = Player->GetActorRotation();

				Params.AddFloatParam(FName(TEXT("User.RewardLevel")), RewardLevel);

				if (RewardCoin == 0) CoinGetLocalEffect.Sound = NoCoinGet;

				Player->EffectManagerComp->PlayGameEffect_Multicast(CoinGetLocalEffect, Context, Params);
			}
		}

		return true;
	}

	return false;
}
