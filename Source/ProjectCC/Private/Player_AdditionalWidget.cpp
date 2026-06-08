// Fill out your copyright notice in the Description page of Project Settings.


#include "Player_AdditionalWidget.h"
#include "Components/Image.h"

void UPlayer_AdditionalWidget::PlayFadeOutAndRemove()
{
	if (FadeOutAnim) {
		UnbindAllFromAnimationFinished(FadeOutAnim);

		FWidgetAnimationDynamicEvent EndDelegate;
		EndDelegate.BindDynamic(this, &UPlayer_AdditionalWidget::HandleFadeOutFinished);

		BindToAnimationFinished(FadeOutAnim, EndDelegate);
		PlayAnimation(FadeOutAnim);
	}
	else {
		RemoveFromParent();
	}
}

void UPlayer_AdditionalWidget::SetImageByID(int32 ImageID)
{
	if (!Image_Additional) return;

	for (const FAdditionalImage& ImageData : AdditionalImages) {
		if (ImageData.ImageID == ImageID) {
			if (ImageData.Image) {
				Image_Additional->SetBrushFromTexture(ImageData.Image);
				Image_Additional->SetRenderOpacity(1.f);
				Image_Additional->SetVisibility(ESlateVisibility::Visible);
				return;
			}
		}
	}

	Image_Additional->SetVisibility(ESlateVisibility::Collapsed);
}

void UPlayer_AdditionalWidget::ClearImage()
{
	if (!Image_Additional) return;

	Image_Additional->SetRenderOpacity(1.f);
	Image_Additional->SetVisibility(ESlateVisibility::Collapsed);
}

void UPlayer_AdditionalWidget::HandleFadeOutFinished() {
	RemoveFromParent();
}
