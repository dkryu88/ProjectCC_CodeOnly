// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_NGradeRandomWeaponBox.generated.h"

enum class EGrade : uint8;
enum class EItemGrade : uint8;

/**
 * 
 */
UCLASS()
class PROJECTCC_API AItem_NGradeRandomWeaponBox : public AItem
{
	GENERATED_BODY()

protected:
	virtual bool UseEffect_Implementation(class APlayer_Character* Player) override;

private:
	EGrade MapItemGradeToMatchGrade(EItemGrade InItemGrade);
};
