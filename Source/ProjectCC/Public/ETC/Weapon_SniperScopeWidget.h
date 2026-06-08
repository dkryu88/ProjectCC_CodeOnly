// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Weapon_SniperScopeWidget.generated.h"

/**
 * 
 */
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCanvasPanelSlot;

UCLASS()
class PROJECTCC_API UWeapon_SniperScopeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable)
	void SetScopeScreenPosition(const FVector2D& ScreenPosition);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_BlackMask;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Scope;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scope")
	TObjectPtr<UMaterialInstance> ScopeMaskMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scope")
	float ScopeRadius = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scope")
	float ScopeSoftness = 0.015f;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ScopeMaskMID;

	
};
