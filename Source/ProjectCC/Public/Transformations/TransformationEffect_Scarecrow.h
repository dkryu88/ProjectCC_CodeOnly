// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerTransformationEffect.h"
#include "TransformationEffect_Scarecrow.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API UTransformationEffect_Scarecrow : public UPlayerTransformationEffect
{
	GENERATED_BODY()
	
public:
	virtual void StartEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* usedPlayer) override;
	virtual void EndFunction(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, bool bUseEndEffect) override;
};
