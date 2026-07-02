// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_FireExtinguish.h"
#include "PlayerConditionDataAsset.h"
#include "PlayerConditionComponent.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "Effect/FGameEffectData.h"
#include "Player_Character.h"

void AWeapon_FireExtinguish::HitEffect_Implementation(APlayer_Character* Player, AActor* Target)
{
	Super::HitEffect_Implementation(Player, Target);

	if (!HasAuthority()) return;
	if (!Target) return;
	if (!Player) return;
	if (!ConditionData) return;

	APlayer_Character* player = Cast<APlayer_Character>(Target);
	if (!player) return;

	UPlayerConditionComponent* ConditionComp = player->FindComponentByClass<UPlayerConditionComponent>();
	if (!ConditionComp) return;

	ConditionComp->ApplyCondition(ConditionData, player, 2.f);

	if (player->EffectManagerComp && WeaponData)
	{
		FGameEffectData* SmokeEffectData = WeaponData->CustomEffects.Find(TEXT("SmokeEffect"));

		if (SmokeEffectData)
		{
			FGameEffectData EffectData = *SmokeEffectData;

			EffectData.SpawnLocationType = EGameEffectSpawnLocationType::ActorLocation;
			EffectData.SpawnRotationType = EGameEffectSpawnRotationType::ActorRotation;

			FGameEffectContext Context;
			Context.SourceActor = player;
			Context.SourceComponent = player->GetRootComponent();
			Context.WorldLocation = player->GetActorLocation();
			Context.WorldRotation = player->GetActorRotation();

			FGameEffectRuntimeParams Params;
			Params.AddFloatParam(TEXT("User.LifeTime"), 2.f);

			player->EffectManagerComp->PlayGameEffect_Multicast(EffectData, Context, Params);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Apply SightOut"));
}

