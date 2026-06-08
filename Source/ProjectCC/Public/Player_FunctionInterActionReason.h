#pragma once

#include "CoreMinimal.h"
#include "Player_FunctionInterActionReason.generated.h"

UENUM(BlueprintType)
enum class EFunctionInterActionReason : uint8 {
	None		UMETA(DisplayName = "None"),
	Move		UMETA(DisplayName = "Move"),
	Jump		UMETA(DisplayName = "Jump"),
	Aim			UMETA(DisplayName = "Aim"),
	Dodge		UMETA(DisplayName = "Dodge"),
	InterAction UMETA(DisplayName = "InterAction"),
	UseItem		UMETA(DisplayName = "UseItem"),
	Hitted		UMETA(DisplayName = "Hitted"),
	Attack		UMETA(DisplayName = "Attacked"),
	Drop		UMETA(DisplayName = "WeaponDropped")
};