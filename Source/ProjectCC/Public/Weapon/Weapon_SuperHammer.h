// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_SuperHammer.generated.h"

/**
 * 
 */
class UPlayerConditionDataAsset;
class APlayer_Character;

UCLASS()
class PROJECTCC_API AWeapon_SuperHammer : public AWeapon
{
	GENERATED_BODY()
	
public:
	AWeapon_SuperHammer(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void EquipEffect_Implementation(APlayer_Character* Player) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;
	virtual bool InteractionWeaponFunction(EFunctionInterActionReason Reason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hammer|Collision")
	TObjectPtr<UStaticMesh> SuperHammerCollisionMesh;

	UPROPERTY(EditAnywhere, Category="ConditionData")
	TObjectPtr<UPlayerConditionDataAsset> DamageImmunityDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SuperHammerDuration")
	float Duration = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperHammerCount")
	int32 AttackCount = 10;
private:
	void AutoAttack();
	void FinishHammerDuration();
	
	FTimerHandle AttackTimerHandle;
	FTimerHandle ExpireTimerHandle;

	UPROPERTY()
	APlayer_Character* OwnPlayer;
};
