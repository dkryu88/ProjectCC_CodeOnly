// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/Title_AdditionalWidget.h"
#include "Components/Button.h"
#include "Title/Title_PlayerController.h"
#include "Sound/AllPlayMode_SoundSubsystem.h"
#include "Blueprint/WidgetTree.h"

void UTitle_AdditionalWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Yes) {
		Button_Yes->OnClicked.RemoveAll(this);
		Button_Yes->OnClicked.AddDynamic(this, &UTitle_AdditionalWidget::OnClicked_YesButton);
	}

	if (Button_No) {
		Button_No->OnClicked.RemoveAll(this);
		Button_No->OnClicked.AddDynamic(this, &UTitle_AdditionalWidget::OnClicked_NoButton);
	}

	if (Button_Cancel) {
		Button_Cancel->OnClicked.RemoveAll(this);
		Button_Cancel->OnClicked.AddDynamic(this, &UTitle_AdditionalWidget::OnClicked_CancelButton);
	}

	if (WidgetTree) {
		WidgetTree->ForEachWidget([this](UWidget* Widget) {
			if (UButton* Button = Cast<UButton>(Widget)) {
				Button->OnClicked.AddUniqueDynamic(this, &UTitle_AdditionalWidget::PlayCommonUIClickSound);
			}
		});
	}
}

void UTitle_AdditionalWidget::OnClicked_YesButton()
{
	ATitle_PlayerController* PC = GetOwningPlayer<ATitle_PlayerController>();
	if (!PC) return;

	PC->ConfirmQuitGame();
}

void UTitle_AdditionalWidget::OnClicked_NoButton()
{
	ATitle_PlayerController* PC = GetOwningPlayer<ATitle_PlayerController>();
	if (!PC) return;

	PC->CloseAdditionalWidget();
}

void UTitle_AdditionalWidget::OnClicked_CancelButton()
{
	ATitle_PlayerController* PC = GetOwningPlayer<ATitle_PlayerController>();
	if (!PC) return;

	PC->ToggleAdditionalWidget();
}

void UTitle_AdditionalWidget::PlayCommonUIClickSound() {
	if (UAllPlayMode_SoundSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAllPlayMode_SoundSubsystem>()) {
		AudioSub->PlayUIClickSound();
	}
}