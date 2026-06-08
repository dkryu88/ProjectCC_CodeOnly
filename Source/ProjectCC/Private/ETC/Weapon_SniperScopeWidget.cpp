// Fill out your copyright notice in the Description page of Project Settings.


#include "ETC/Weapon_SniperScopeWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void UWeapon_SniperScopeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_BlackMask && ScopeMaskMaterial) {
		ScopeMaskMID = UMaterialInstanceDynamic::Create(ScopeMaskMaterial, this);
		Image_BlackMask->SetBrushFromMaterial(ScopeMaskMID);

		ScopeMaskMID->SetScalarParameterValue(TEXT("ScopeRadius"), ScopeRadius);
		ScopeMaskMID->SetScalarParameterValue(TEXT("ScopeSoftness"), ScopeSoftness);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWeapon_SniperScopeWidget::SetScopeScreenPosition(const FVector2D& ScreenPosition)
{
	FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);

	if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f) {
		return;
	}

	float SafeScale = FMath::Max(ViewportScale, 0.01f);
	FVector2D LocalViewportSize = ViewportSize / SafeScale;
	FVector2D LocalScreenPosition = ScreenPosition / SafeScale;

	if (Image_BlackMask) {
		if (UCanvasPanelSlot* MaskSlot = Cast<UCanvasPanelSlot>(Image_BlackMask->Slot)) {
			MaskSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
			MaskSlot->SetAlignment(FVector2D(0.f, 0.f));
			MaskSlot->SetPosition(FVector2D(0.f, 0.f));
			MaskSlot->SetSize(LocalViewportSize);
			MaskSlot->SetZOrder(0);
		}
	}

	if (ScopeMaskMID) {
		FVector2D CenterUV(ScreenPosition.X / ViewportSize.X, ScreenPosition.Y / ViewportSize.Y);
		ScopeMaskMID->SetVectorParameterValue(TEXT("CenterUV"), FLinearColor(CenterUV.X, CenterUV.Y, 0.f, 0.f));
		ScopeMaskMID->SetScalarParameterValue(TEXT("AspectRadio"), ViewportSize.X / ViewportSize.Y);
	}

	if (Image_Scope) {
		if (UCanvasPanelSlot* ScopeSlot = Cast<UCanvasPanelSlot>(Image_Scope->Slot)) {
			FVector2D ScopeSize = ScopeSlot->GetSize();
			ScopeSlot->SetPosition(LocalScreenPosition - ScopeSize * 0.5f);
			ScopeSlot->SetZOrder(1);
		}
	}
}
