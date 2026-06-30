// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FootStepDataAsset.generated.h"

/**
 * 
 */

//UENUM(BlueprintType)
//enum class EFootStepActionType : uint8
//{
//Walk,
//jump,
//Dodge
//};

UCLASS()
class PROJECTCC_API UFootStepDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Footstep|Walk")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<USoundBase>> SurfaceSoundMap;

	UPROPERTY(EditDefaultsOnly,Category = "Jump")
	TObjectPtr<USoundBase> JumpSound;

	/*UPROPERTY(EditDefaultsOnly, Category = "Footstep|Jump")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<USoundBase>> JumpSoundMap;

	UPROPERTY(EditDefaultsOnly, Category = "Footstep|Dodge")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<USoundBase>> DodgeSoundMap;*/
};
