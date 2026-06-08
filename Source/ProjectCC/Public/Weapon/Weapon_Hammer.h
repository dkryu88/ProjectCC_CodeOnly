// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_Hammer.generated.h"

/**
 * 
 */
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class PROJECTCC_API AWeapon_Hammer : public AWeapon
{
	GENERATED_BODY()
	
public:
	AWeapon_Hammer(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	//¸ÁÄ¡ ¸Ó¸® Collider
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hammer|Collision")
	TObjectPtr<UStaticMesh> HammerCollisionMesh;
};
