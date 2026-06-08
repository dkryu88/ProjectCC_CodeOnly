// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_Turret.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AItem_Turret : public AItem
{
	GENERATED_BODY()

public:
	virtual bool UseEffect_Implementation(class APlayer_Character* Player) override;

	UPROPERTY(EditAnywhere, Category = "Turret")
	TSubclassOf<class AObjects_Turret> TurretClass;
};
