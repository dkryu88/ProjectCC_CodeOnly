// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerSoundDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API UPlayerSoundDataAsset : public UDataAsset
{
	GENERATED_BODY()
	

public:
	UPROPERTY(EditDefaultsOnly, Category = "FootStep")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<USoundBase>> SurfaceSoundMap;

	UPROPERTY(EditDefaultsOnly, Category = "Jump")
	TObjectPtr<USoundBase> JumpSound;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TObjectPtr<USoundBase> DodgeSound;
};
