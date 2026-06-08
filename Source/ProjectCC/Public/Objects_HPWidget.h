// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Objects_HPWidget.generated.h"

/**
 * 
 */

class UProgressBar;
class UImage;
class UOverlay;
class AObjects;

UCLASS()
class PROJECTCC_API UObjects_HPWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void InitWidget(AObjects* Object);

	UFUNCTION(BlueprintCallable)
	void RefreshWidget();

	UFUNCTION(BlueprintCallable)
	void SetHPBar();

	UFUNCTION(BlueprintCallable)
	void SetUI();

protected:
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_HP;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_HP;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_HPBarFrame;

	//UI 대상 물체
	UPROPERTY()
	TObjectPtr<AObjects> OwnerObjects;
};
