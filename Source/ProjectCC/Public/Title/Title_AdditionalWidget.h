// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Title_AdditionalWidget.generated.h"

/**
 * 
 */

class UButton;

UCLASS()
class PROJECTCC_API UTitle_AdditionalWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnClicked_YesButton();

	UFUNCTION()
	void OnClicked_NoButton();

	UFUNCTION()
	void OnClicked_CancelButton();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Yes;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_No;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Cancel;
};
