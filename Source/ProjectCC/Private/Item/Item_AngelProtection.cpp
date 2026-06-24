// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_AngelProtection.h"
#include "Player_Character.h"
#include "ItemDataAsset.h"
#include "PlayerConditionComponent.h"
#include "PlayerConditionDataAsset.h"

bool AItem_AngelProtection::UseEffect_Implementation(APlayer_Character* Player)
{
	if (!HasAuthority() || !Player || !InvincibleDataAsset) return false;

	if (Player->ConditionComp) {
		Player->ConditionComp->ApplyCondition(InvincibleDataAsset, Player, DamageImmunityDuration);

		if (Player->EffectManagerComp) {
			FGameEffectData* EffectData = &ItemData->UseEffect;
			if (EffectData) {
				FGameEffectContext Context;
				FGameEffectRuntimeParams Params;

				Context.SourceActor = Player;
				Context.SourceComponent = Player->GetMesh();
				Context.WorldLocation = Player->GetActorLocation();
				Context.WorldRotation = Player->GetActorRotation();

				Params.AddFloatParam(TEXT("User.LifetTime"), DamageImmunityDuration);

				Player->EffectManagerComp->PlayGameEffect_Multicast(*EffectData, Context, Params);
			}
		}

		return true;
	}

	return false;
}
