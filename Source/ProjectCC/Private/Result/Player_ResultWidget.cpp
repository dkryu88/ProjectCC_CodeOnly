// Fill out your copyright notice in the Description page of Project Settings.


#include "Result/Player_ResultWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Player_ScoreBoardWidget.h"
#include "Player_State.h"

void UPlayer_ResultWidget::InitWidget(const FMatchResultData& OwnData, const TArray<FMatchResultData>& Results)
{
	SetBackgroundImageByMapName(OwnData.MapName);

	if (Text_Player_OwnNickname) {
		Text_Player_OwnNickname->SetText(FText::FromString(OwnData.Nickname));
	}
	if (Text_Rank)
	{
		Text_Rank->SetText(FText::FromString(FString::Printf(TEXT("%d"), OwnData.Rank)));
	}
	if (Text_Coin) {
		Text_Coin->SetText(FText::AsNumber(OwnData.Coin));
	}

	SetAllPlayerResults(OwnData.Nickname,OwnData.Rank, Results);

	GetWorld()->GetTimerManager().ClearTimer(ShowMedalTimer);
	GetWorld()->GetTimerManager().SetTimer(ShowMedalTimer, FTimerDelegate::CreateUObject(this, &UPlayer_ResultWidget::ShowPlayerRankMedal, OwnData.Rank), 0.5f, false);

	SetExitEnabled(false);
}

void UPlayer_ResultWidget::SetAllPlayerResults(FString OwnNickname, int32 Rank, const TArray<FMatchResultData>& Results)
{
	if (!VerticalBox_PlayerResults || !ScoreWidget) return;

	VerticalBox_PlayerResults->ClearChildren();

	for (const FMatchResultData& Data : Results) {
		UPlayer_ScoreBoardWidget* Row = CreateWidget<UPlayer_ScoreBoardWidget>(GetOwningPlayer(), ScoreWidget);
		if (!Row) continue;

		bool bIsSelf = (Data.Nickname == OwnNickname && Data.Rank == Rank) ? true : false;

		Row->InitWidgetWithoutPlayerState(Data, bIsSelf);
		VerticalBox_PlayerResults->AddChild(Row);
	}
}

void UPlayer_ResultWidget::ShowPlayerRankMedal(int32 Rank)
{
	if (RankMedals.Num() < 1 || !Image_RankMedal) return;

	if (Rank > 0 && Rank < 5 && RankMedals[Rank - 1]) {
		Image_RankMedal->SetBrushFromTexture(RankMedals[Rank - 1]);
		Image_RankMedal->SetVisibility(ESlateVisibility::Visible);
		if (Anim_RankMedal) {
			StopAnimation(Anim_RankMedal);
			PlayAnimation(Anim_RankMedal, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f);
		}
	}
	else {
		if (Anim_RankMedal) {
			StopAnimation(Anim_RankMedal);
		}
		Image_RankMedal->SetVisibility(ESlateVisibility::Collapsed);
	}
	
}

void UPlayer_ResultWidget::SetExitEnabled(bool bEndEnabled)
{
	if (!Text_CanExit) return;

	Text_CanExit->SetVisibility(bEndEnabled ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

	if (bEndEnabled) {
		Text_CanExit->SetText(FText::FromString(TEXT("Press Z to Exit")));
	}
}

void UPlayer_ResultWidget::SetBackgroundImageByMapName(FName MapName)
{
	if (!Image_Background) return;

	for (const FResultBackgroundData& Data : ResultBackgrounds) {
		if (Data.MapName == MapName && Data.BackgroundTexture) {
			Image_Background->SetBrushFromTexture(Data.BackgroundTexture);
			return;
		}
	}

	if (DefaultBackgroundTexture) {
		Image_Background->SetBrushFromTexture(DefaultBackgroundTexture);
	}
}




