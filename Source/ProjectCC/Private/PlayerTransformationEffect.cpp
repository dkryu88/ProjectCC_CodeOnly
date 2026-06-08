// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerTransformationEffect.h"
#include "PlayerTransformationComponent.h"
#include "Player_Character.h"
#include "PlayerVisualManagerComponent.h"

//각 변신이 상속받아 override
void UPlayerTransformationEffect::StartEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* usedPlayer)
{
	AddVisualEffect(Player, TransformationComp, TransformationData, usedPlayer);
}

void UPlayerTransformationEffect::PersistEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, float DeltaTime)
{
}

void UPlayerTransformationEffect::HittedEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* AttackedPlayer)
{
}

void UPlayerTransformationEffect::EndEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData)
{
}

void UPlayerTransformationEffect::EndFunction(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, bool bUseEndEffect)
{
	RemoveVisualEffect(Player, TransformationComp, TransformationData);
	if (bUseEndEffect) {
		EndEffect(Player, TransformationComp, TransformationData);
	}
}

bool UPlayerTransformationEffect::BuildVisualEffectRequest(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* UsedPlayer, FVisualEffectRequest& OutRequest)
{
	if (!TransformationData.bUseVisualManager) return false;

	OutRequest = TransformationData.VisualData;
	OutRequest.Priority = TransformationData.Priority;
	
	return true;
}

FName UPlayerTransformationEffect::ResolveVisualEffectName(const FPlayerTransformation& TransformationData)
{
	if (!TransformationData.VisualEffectName.IsNone()) {
		return TransformationData.VisualEffectName;
	}

	return TransformationData.TransformationName;
}

void UPlayerTransformationEffect::AddVisualEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* UsedPlayer)
{
	if (!TransformationData.bUseVisualManager) return;
	if (!Player || !Player->HasAuthority()) return;
	if (!Player->VisualManagerComp) return;

	FName ResolvedEffectName = ResolveVisualEffectName(TransformationData);

	if (ResolvedEffectName.IsNone()) return;

	FVisualEffectRequest Request;

	if (!BuildVisualEffectRequest(Player, TransformationComp, TransformationData, UsedPlayer, Request)) return;

	Player->VisualManagerComp->Multi_AddVisualEffect(ResolvedEffectName, Request);
}

void UPlayerTransformationEffect::RemoveVisualEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData)
{
	if (!TransformationData.bUseVisualManager) return;
	if (!Player || !Player->HasAuthority()) return;
	if (!Player->VisualManagerComp) return;

	FName ResolvedEffectName = ResolveVisualEffectName(TransformationData);
	if (ResolvedEffectName.IsNone()) return;

	Player->VisualManagerComp->Multi_RemoveVisualEffect(ResolvedEffectName);
}



