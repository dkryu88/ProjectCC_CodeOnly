// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDataAsset.generated.h"

UENUM(BlueprintType)
enum class EItemGrade : uint8
{
	None UMETA(DisplayName = "Default"),
	B    UMETA(DisplayName = "B Grade"),
	A    UMETA(DisplayName = "A Grade"),
	S    UMETA(DisplayName = "S Grade")
};


class UItemEffect;
/**
 * 각각의 아이템 효과와 이름을 관리
 */
UCLASS()
class PROJECTCC_API UItemDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	//아이템 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemName = "Default";

	//아이템 사용 가능 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 MaxUseCount = 1;

	//변신 중 아이템 사용 가능 유무
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	bool CanUseWhileTransforming = false;

	//아이템 등급
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	EItemGrade ItemGrade = EItemGrade::B;

	//아이템 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UTexture2D> ItemIcon = nullptr;
};
