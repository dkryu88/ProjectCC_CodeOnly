// Fill out your copyright notice in the Description page of Project Settings.


#include "Player_CharacterWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "PlayerTransformationComponent.h"
#include "Player_Character.h"
#include "Player_State.h"
#include "Item.h"
#include "ItemDataAsset.h"
#include "Objects_HPWidget.h"

void UPlayer_CharacterWidget::NativeDestruct()
{
	UnbindCharacterEvents();
	Super::NativeDestruct();
}

//위젯 초기 설정
void UPlayer_CharacterWidget::InitWidget(APlayer_Character* Player)
{
	UnbindCharacterEvents();

	OwnerCharacter = Player;
	OwnerState = OwnerCharacter ? OwnerCharacter->GetPlayerState<APlayer_State>() : nullptr;

	SetNickname();
	SetItemIcon();
	SetHPBar();
	SetCoin();
	SetUI();

	BindCharacterEvents();
}

//Hp, Item 변경 이벤트 바인딩
void UPlayer_CharacterWidget::BindCharacterEvents()
{
	if (OwnerCharacter) {
		OwnerCharacter->OnHPChanged.RemoveAll(this);
		OwnerCharacter->OnHPChanged.AddUObject(this, &UPlayer_CharacterWidget::HandleHPChanged);
	}
	if (OwnerState) {
		OwnerState->OnItemChanged.RemoveAll(this);
		OwnerState->OnNicknameChanged.RemoveAll(this);
		OwnerState->OnCoinChanged.RemoveAll(this);

		OwnerState->OnItemChanged.AddUObject(this, &UPlayer_CharacterWidget::HandleItemChanged);
		OwnerState->OnNicknameChanged.AddUObject(this, &UPlayer_CharacterWidget::HandleNicknameChanged);
		OwnerState->OnCoinChanged.AddUObject(this, &UPlayer_CharacterWidget::HandleCoinChanged);
	}
}

void UPlayer_CharacterWidget::UnbindCharacterEvents()
{
	if (OwnerCharacter) {
		OwnerCharacter->OnHPChanged.RemoveAll(this);
	}
	if (OwnerState) {
		OwnerState->OnItemChanged.RemoveAll(this);
		OwnerState->OnNicknameChanged.RemoveAll(this);
		OwnerState->OnCoinChanged.RemoveAll(this);
	}

}

//HP 바 갱신
void UPlayer_CharacterWidget::HandleHPChanged(float NewHP, float MaxHP)
{
	SetHPBar();
}
//아이템 아이콘 갱신
void UPlayer_CharacterWidget::HandleItemChanged(TSubclassOf<AItem> item, int32 UseCount)
{
	SetItemIcon();
}
//닉네임 갱신
void UPlayer_CharacterWidget::HandleNicknameChanged(const FString& Nickname)
{
	SetNickname();
}
//코인 수 갱신
void UPlayer_CharacterWidget::HandleCoinChanged(int32 Coin)
{
	SetCoin();
}
//코인 수 세팅
void UPlayer_CharacterWidget::SetCoin() {
	if (!Text_Coin) return;

	if (!OwnerState) {
		Text_Coin->SetText(FText::AsNumber(0));
		return;
	}

	Text_Coin->SetText(FText::AsNumber(OwnerState->GetPlayerCoin()));
}


//닉네임 세팅
void UPlayer_CharacterWidget::SetNickname()
{
	if (!Text_Nickname) return;
	if (!OwnerState) {
		Text_Nickname->SetText(FText::GetEmpty());
		return;
	}

	Text_Nickname->SetText(FText::FromString(OwnerState->GetNickName()));
}
//아이템 아이콘 세팅 / 갱신
void UPlayer_CharacterWidget::SetItemIcon()
{
	if (!Image_ItemIcon) return;

	bool bIsSelf = OwnerCharacter && OwnerCharacter->IsLocallyControlled();
	if (bIsSelf)
	{
		Image_ItemIconFrame->SetVisibility(ESlateVisibility::Collapsed);
		Image_ItemIconBG->SetVisibility(ESlateVisibility::Collapsed);
		Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (!OwnerState) {
		Image_ItemIconFrame->SetVisibility(ESlateVisibility::Collapsed);
		Image_ItemIconBG->SetVisibility(ESlateVisibility::Collapsed);
		Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TSubclassOf<AItem> item = OwnerState->GetEquippedItem();
	if (!item) {
		Image_ItemIconFrame->SetVisibility(ESlateVisibility::Collapsed);
		Image_ItemIconBG->SetVisibility(ESlateVisibility::Collapsed);
		Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	AItem* DefaultItem = item->GetDefaultObject<AItem>();
	if (!DefaultItem || !DefaultItem->ItemData || !DefaultItem->ItemData->ItemIcon) {
		Image_ItemIconFrame->SetVisibility(ESlateVisibility::Visible);
		Image_ItemIconBG->SetVisibility(ESlateVisibility::Visible);
		Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Image_ItemIcon->SetBrushFromTexture(Image_SecretItemIcon);
	Image_ItemIcon->SetColorAndOpacity(FLinearColor::White);
	Image_ItemIcon->SetBrushTintColor(FSlateColor(FLinearColor::White));
	Image_ItemIconFrame->SetVisibility(ESlateVisibility::Visible);
	Image_ItemIconBG->SetVisibility(ESlateVisibility::Visible);
	Image_ItemIcon->SetVisibility(ESlateVisibility::Visible);
}

//HP바 세팅 / 갱신
void UPlayer_CharacterWidget::SetHPBar()
{
	if (!ProgressBar_HP) return;
	if (!OwnerCharacter || OwnerCharacter->BaseStats.Max_HP <= 0.f) {
		ProgressBar_HP->SetPercent(0.f);
		return;
	}

	float Percent = OwnerCharacter->GetCurrentHP() / OwnerCharacter->BaseStats.Max_HP;
	ProgressBar_HP->SetPercent(Percent);
}
//UI 최종 세팅
void UPlayer_CharacterWidget::SetUI()
{
	bool bIsSelf = OwnerCharacter && OwnerCharacter->IsLocallyControlled();
	
	//변신이 위젯을 숨기는 경우, 대상 캐릭터가 본인 캐릭터가 아니라면 숨김 처리
	if (CanvasPanel_CharacterWidget) {
		bool bHideByTransformation = false;

		if (OwnerCharacter && OwnerCharacter->TransformationComp) {
			FPlayerTransformation& TransformData = OwnerCharacter->TransformationComp->CurrentTransformation;

			bHideByTransformation = TransformData.bActive && !TransformData.bExposureCharacterWidget;
		}
		CanvasPanel_CharacterWidget->SetVisibility(!bIsSelf && bHideByTransformation ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	//대상 캐릭터가 본인 캐릭터가 아니라면 HP를 보여주지 않음
	if (ProgressBar_HP && Image_HPBarFrame) {
		ProgressBar_HP->SetVisibility(bIsSelf ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Image_HPBarFrame->SetVisibility(bIsSelf ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	//대상 캐릭터가 본인 캐릭터라면 아이템 아이콘을 보여주지 않음 (Player_ControllerWidget에서 보여줌)
	if (Image_ItemIcon && Image_ItemIconFrame && Image_ItemIconBG && bIsSelf) {
		Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		Image_ItemIconFrame->SetVisibility(ESlateVisibility::Collapsed);
		Image_ItemIconBG->SetVisibility(ESlateVisibility::Collapsed);
	}
	//코인은 본인 캐릭터임과 상관없이 보여줌
	if (Text_Coin && Image_CoinIcon) {
		Text_Coin->SetVisibility(ESlateVisibility::Visible);
		Image_CoinIcon->SetVisibility(ESlateVisibility::Visible);
	}
}

