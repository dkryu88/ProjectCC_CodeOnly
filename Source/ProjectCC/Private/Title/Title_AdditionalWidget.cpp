// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/Title_AdditionalWidget.h"
#include "Components/Button.h"
#include "Title/Title_PlayerController.h"
#include "Sound/AllPlayMode_SoundSubsystem.h"	//[클릭]
#include "Blueprint/WidgetTree.h"	//[클릭]

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

	if (Button_Cancel) {	//[x버튼]
		Button_Cancel->OnClicked.RemoveAll(this);
		Button_Cancel->OnClicked.AddDynamic(this, &UTitle_AdditionalWidget::OnClicked_CancelButton);
	}

	//[클릭]
	if (WidgetTree) {
		WidgetTree->ForEachWidget([this](UWidget* widget) {
			if (UButton* Btn = Cast<UButton>(widget)) {
				Btn->OnClicked.AddUniqueDynamic(this, &UTitle_AdditionalWidget::PlayCommonUIClickSound);
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

//[클릭]
void UTitle_AdditionalWidget::PlayCommonUIClickSound()
{
	if (UAllPlayMode_SoundSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAllPlayMode_SoundSubsystem>()) {
		AudioSub->PlayUIClickSound();
	}
}
