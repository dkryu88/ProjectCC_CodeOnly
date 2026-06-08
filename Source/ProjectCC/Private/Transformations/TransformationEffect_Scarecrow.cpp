// Fill out your copyright notice in the Description page of Project Settings.


#include "Transformations/TransformationEffect_Scarecrow.h"
#include "Player_Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UTransformationEffect_Scarecrow::StartEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* usedPlayer)
{
	Super::StartEffect(Player, TransformationComp, TransformationData, usedPlayer);
	if (Player) Player->AddSpeedController(TEXT("Slow"), 0.5f, 0.f, true, 3);
}

void UTransformationEffect_Scarecrow::EndFunction(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, bool bUseEndEffect)
{
	if (Player) Player->RemoveSpeedControllerByName(TEXT("Slow"));

	Super::EndFunction(Player, TransformationComp, TransformationData, bUseEndEffect);
}
