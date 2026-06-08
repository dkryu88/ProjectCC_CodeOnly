// Fill out your copyright notice in the Description page of Project Settings.


#include "Player_ScoreBoardWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Player_State.h"
#include "Item.h"
#include "ItemDataAsset.h"

void UPlayer_ScoreBoardWidget::InitWidget(APlayer_State* playerState, bool bIsSelf)
{
	if (!playerState) return;

	if (Image_ScoreBoard_Individual_BackGround) {
		UTexture2D* TargetBackground = bIsSelf ? Image_ScoreBoard_Individual_BackGround_Owner : Image_ScoreBoard_Individual_BackGround_Enemy;

		if (TargetBackground) {
			Image_ScoreBoard_Individual_BackGround->SetBrushFromTexture(TargetBackground, true);
		}
	}

	if (Text_Rank) {
		Text_Rank->SetText(FText::AsNumber(playerState->GetPlayerRank()));
	}

	if (Text_Nickname) {
		Text_Nickname->SetText(FText::FromString(playerState->GetNickName()));
	}

	if (Text_Coin) {
		Text_Coin->SetText(FText::AsNumber(playerState->GetPlayerCoin()));
	}

	if (Text_Eliminate) {
		Text_Eliminate->SetText(FText::AsNumber(playerState->GetPlayerEliminate()));
	}

	if (Text_Out) {
		Text_Out->SetText(FText::AsNumber(playerState->GetPlayerOut()));
	}

	if (Overlay_ItemIcon) {
		Overlay_ItemIcon->SetVisibility(ESlateVisibility::Visible);
	}

	if (!Image_Item) return;

	TSubclassOf<AItem> EquippedItem = playerState->GetEquippedItem();

	if (!EquippedItem) {
		Image_Item->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	//대상이 자신일 경우 아이템 아이콘을 온전하게 보여줌
	if (bIsSelf) {
		AItem* DefaultItem = EquippedItem->GetDefaultObject<AItem>();
		if (DefaultItem && DefaultItem->ItemData && DefaultItem->ItemData->ItemIcon) {
			Image_Item->SetBrushFromTexture(DefaultItem->ItemData->ItemIcon);
			Image_Item->SetVisibility(ESlateVisibility::Visible);
			return;
		}

		Image_Item->SetVisibility(ESlateVisibility::Collapsed);
	}

	//대상이 자신이 아닌 경우 아이템 아이콘을 ?아이콘으로 대체
	if (Image_ItemIcon_Secret) {
		Image_Item->SetBrushFromTexture(Image_ItemIcon_Secret);
		Image_Item->SetVisibility(ESlateVisibility::Visible);
	}
	else {
		Image_Item->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPlayer_ScoreBoardWidget::InitWidgetWithoutPlayerState(const FMatchResultData& Result, bool bIsSelf)
{
	if (Image_ScoreBoard_Individual_BackGround) {
		UTexture2D* TargetBackground = bIsSelf ? Image_ScoreBoard_Individual_BackGround_Owner : Image_ScoreBoard_Individual_BackGround_Enemy;

		if (TargetBackground) {
			Image_ScoreBoard_Individual_BackGround->SetBrushFromTexture(TargetBackground, true);
		}
	}

	if (Text_Rank) {
		Text_Rank->SetText(FText::AsNumber(Result.Rank));
	}

	if (Text_Nickname) {
		Text_Nickname->SetText(FText::FromString(Result.Nickname));
	}

	if (Text_Coin) {
		Text_Coin->SetText(FText::AsNumber(Result.Coin));
	}

	if (Text_Eliminate) {
		Text_Eliminate->SetText(FText::AsNumber(Result.Eliminate));
	}

	if (Text_Out) {
		Text_Out->SetText(FText::AsNumber(Result.Out));
	}

	if (Overlay_ItemIcon) {
		Overlay_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}
