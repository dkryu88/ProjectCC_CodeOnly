// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Item_Shield.generated.h"

/**
 *
 */
UCLASS()
class PROJECTCC_API AItem_Shield : public AItem
{
	GENERATED_BODY()

protected:
	/** 아이템 사용 시 호출되는 핵심 로직 */
	virtual bool UseEffect_Implementation(class APlayer_Character* Player) override;

private:
	/** 소환할 방패 오브젝트의 클래스 (에디터에서 Objects_Shield 블루프린트 지정) */
	UPROPERTY(EditAnywhere, Category = "Shield")
	TSubclassOf<class AObjects> Shield;
};
