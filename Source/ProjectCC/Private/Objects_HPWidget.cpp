// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects_HPWidget.h"
#include "Objects.h"
#include "ObjectsDataAsset.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

void UObjects_HPWidget::InitWidget(AObjects* Objects) {
	OwnerObjects = Objects;

	SetUI();
	SetHPBar();
}

void UObjects_HPWidget::NativeDestruct() {
	OwnerObjects = nullptr;

	Super::NativeDestruct();
}

void UObjects_HPWidget::SetUI() {
	if (Overlay_HP)
	{
		Overlay_HP->SetVisibility(OwnerObjects->HP >= 0.f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (ProgressBar_HP)
	{
		ProgressBar_HP->SetVisibility(ESlateVisibility::Visible);
	}
	if (Image_HPBarFrame)
	{
		Image_HPBarFrame->SetVisibility(ESlateVisibility::Visible);
	}
}

void UObjects_HPWidget::SetHPBar()
{
	if (!ProgressBar_HP) return;

	if (!OwnerObjects || !OwnerObjects->ObjectsData)
	{
		ProgressBar_HP->SetPercent(0.f);
		return;
	}

	float MaxHP = OwnerObjects->ObjectsData->DefaultHP;

	if (MaxHP <= 0.f)
	{
		ProgressBar_HP->SetPercent(0.f);
		return;
	}

	float Percent = FMath::Clamp(OwnerObjects->HP / MaxHP, 0.f, 1.f);
	ProgressBar_HP->SetPercent(Percent);
}

void UObjects_HPWidget::RefreshWidget() {
	if (!OwnerObjects) return;

	SetUI();
	SetHPBar();
}