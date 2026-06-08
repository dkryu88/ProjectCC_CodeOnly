// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlockType.generated.h"

UENUM(BlueprintType)
enum class EBlockType : uint8 {
	Empty UMETA(DisplayName="Empty"),
	Normal UMETA(DisplayName = "Normal Block"),
	Transparency UMETA(DisplayName = "transparency Block"),
	Damage	UMETA(DisplayName = "Damage Block")
};
