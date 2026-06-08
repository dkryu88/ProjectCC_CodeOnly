// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopStock.h"
#include "Match_ShopWidget.generated.h"

/**
 * 플레이어 상점과 관련된 위젯
 */

class UButton;
class UTextBlock;
class APlayer_State;

UCLASS()
class PROJECTCC_API UMatch_ShopWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnClicked_B_Box();

	UFUNCTION()
	void OnClicked_A_Box();

	UFUNCTION()
	void OnClicked_S_Box();

	UFUNCTION()
	void OnClicked_Random_Box();

	void Purchase(EShopBoxs Box);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_B_Box;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_A_Box;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_S_Box;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Random_Box;
};
