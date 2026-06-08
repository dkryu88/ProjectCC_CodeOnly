// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponListDataAsset.generated.h"

/**
 * 모든 Weapon 클래스에 대한 DataAsset 정의
 */

class AWeapon;

UCLASS()
class PROJECTCC_API UWeaponListDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AllWeapon")
	TArray<TSubclassOf<AWeapon>> BWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AllWeapon")
	TArray<TSubclassOf<AWeapon>> AWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AllWeapon")
	TArray<TSubclassOf<AWeapon>> SWeapons;
};
