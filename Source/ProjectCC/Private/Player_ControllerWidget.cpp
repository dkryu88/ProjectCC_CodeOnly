// Fill out your copyright notice in the Description page of Project Settings.


#include "Player_ControllerWidget.h"
#include "Match_PlayerController.h"
#include "Shop/Match_ShopWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/UserWidget.h"
#include "Player_Character.h"
#include "Player_State.h"
#include "Match_State.h"
#include "Item.h"
#include "ItemDataAsset.h"
#include "Weapon.h"
#include "WeaponDataAsset.h"
#include "Objects.h"
#include "ObjectsDataAsset.h"

void UPlayer_ControllerWidget::InitWidget(APlayer_Character* Player) {
	UnBindEvents();

	OwnerCharacter = Player;
	OwnerState = OwnerCharacter ? OwnerCharacter->GetPlayerState<APlayer_State>() : nullptr;
	MatchState = GetWorld() ? GetWorld()->GetGameState<AMatch_State>() : nullptr;
	LastHitHP = OwnerCharacter ? OwnerCharacter->BaseStats.Max_HP : 0.f;

	if (Image_HitAlert) {
		Image_HitAlert->SetRenderOpacity(0.f);
		Image_HitAlert->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetShopClosedState(false);

	BindEvents();

	SetMatchTime();
	SetRank();
	SetCoin();
	SetItem();
	SetWeapon();
	SetWeaponUseCount();
	
	//OwnerState Bind 실패 시 1프레임 뒤에 재시도
	if (!OwnerState && GetWorld()) {
		GetWorld()->GetTimerManager().SetTimer(RetryPSBindTimerHandle, this, &ThisClass::BindOwnerState, 0.1f, true);
	}

	HideMatchEventUI();
}

void UPlayer_ControllerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//무기 UseCount관련 UI를 이름으로 캐싱
	WeaponUseCountOne.Empty();
	WeaponUseCountOneInside.Empty();

	for (int32 i = 1; i <= 10; ++i) {
		FString WeaponUseCountOneName = FString::Printf(TEXT("Overlay_WeaponUseCountOne_%d"), i);
		UOverlay* Overlay_WeaponUseCountOne = Cast<UOverlay>(GetWidgetFromName(FName(*WeaponUseCountOneName)));
		FString WeaponUseCountOneInsideName = FString::Printf(TEXT("Image_WeaponUseCountOneInside_%d"), i);
		UImage* Image_WeaponUseCountOneInside = Cast<UImage>(GetWidgetFromName(FName(*WeaponUseCountOneInsideName)));

		if (!Image_WeaponUseCountOneInside && Overlay_WeaponUseCountOne) {
			Image_WeaponUseCountOneInside = FindImageByNameContains(Overlay_WeaponUseCountOne, TEXT("Inside"));
		}

		WeaponUseCountOne.Add(Overlay_WeaponUseCountOne);
		WeaponUseCountOneInside.Add(Image_WeaponUseCountOneInside);
	}

	SetShopClosedState(false);
	HideWeaponUseCountUI();
}

void UPlayer_ControllerWidget::NativeDestruct() {
	UnBindEvents();
	Super::NativeDestruct();
}

void UPlayer_ControllerWidget::BindEvents() {
	//Event 바인딩 전에 Player_State부터 바인딩
	BindOwnerState();

	if (OwnerState) {
		OwnerState->OnCoinChanged.RemoveAll(this);
		OwnerState->OnItemChanged.RemoveAll(this);
		OwnerState->OnRankChanged.RemoveAll(this);

		OwnerState->OnCoinChanged.AddUObject(this, &UPlayer_ControllerWidget::HandleCoinChanged);
		OwnerState->OnItemChanged.AddUObject(this, &UPlayer_ControllerWidget::HandleItemChanged);
		OwnerState->OnRankChanged.AddUObject(this, &UPlayer_ControllerWidget::HandleRankChanged);
	}

	if (MatchState) {
		MatchState->OnMatchTimeChanged.RemoveAll(this);
		MatchState->OnMatchTimeChanged.AddUObject(this, &UPlayer_ControllerWidget::HandleMatchTimeChanged);
	}

	if (OwnerCharacter) {
		OwnerCharacter->OnWeaponChanged.RemoveAll(this);
		OwnerCharacter->OnWeaponChanged.AddUObject(this, &UPlayer_ControllerWidget::HandleWeaponChanged);
		OwnerCharacter->OnHPChanged.RemoveAll(this);
		OwnerCharacter->OnHPChanged.AddUObject(this, &UPlayer_ControllerWidget::HandleHPChanged);
	}

	//UI 갱신 누락 방지
	SetItem();
	SetCoin();
	SetRank();
}

//Player의 State 바인딩 (최초 바인딩 실패 시 재시도 용도)
void UPlayer_ControllerWidget::BindOwnerState()
{
	if (!OwnerCharacter) return;

	APlayer_State* NewState = OwnerCharacter->GetPlayerState<APlayer_State>();
	if (!NewState) return;

	//최초 바인딩 실패 시 다시 바인딩 시도이므로 이미 같은 State가 있다면 Skip
	if (OwnerState == NewState) return;

	//NewState와 다르다면 Dumy Data 또는 쓰레기값이 있을 가능성이 높으므로 Coin,Item,Rank 바인딩 제거
	if (OwnerState) {
		OwnerState->OnCoinChanged.RemoveAll(this);
		OwnerState->OnItemChanged.RemoveAll(this);
		OwnerState->OnRankChanged.RemoveAll(this);
	}

	//플레이어의 State를 정상적으로 재적용
	OwnerState = NewState;
	OwnerState->OnCoinChanged.RemoveAll(this);
	OwnerState->OnItemChanged.RemoveAll(this);
	OwnerState->OnRankChanged.RemoveAll(this);

	//재적용된 State에 대해 Coin, Item, Rank 재바인딩
	OwnerState->OnCoinChanged.AddUObject(this, &UPlayer_ControllerWidget::HandleCoinChanged);
	OwnerState->OnItemChanged.AddUObject(this, &UPlayer_ControllerWidget::HandleItemChanged);
	OwnerState->OnRankChanged.AddUObject(this, &UPlayer_ControllerWidget::HandleRankChanged);

	//Coin, Item, Rank 초기화
	SetCoin();
	SetRank();
	SetItem();
}

void UPlayer_ControllerWidget::UnBindEvents() {
	if (OwnerState) {
		OwnerState->OnCoinChanged.RemoveAll(this);
		OwnerState->OnItemChanged.RemoveAll(this);
		OwnerState->OnRankChanged.RemoveAll(this);
	}

	if (MatchState) {
		MatchState->OnMatchTimeChanged.RemoveAll(this);
	}

	if (OwnerCharacter) {
		OwnerCharacter->OnWeaponChanged.RemoveAll(this);
		OwnerCharacter->OnHPChanged.RemoveAll(this);
	}
}

void UPlayer_ControllerWidget::SetUIState(EPlayerUIState NewState)
{
	if (NowPlayerUIState == NewState) return;
	NowPlayerUIState = NewState;

	SwitchLayout();
}

void UPlayer_ControllerWidget::SetCountdown(int32 Number)
{
	if (!Image_Countdown || !Image_CountdownStart) return;

	if (Number < 0 || Number > 6) {
		Image_Countdown->SetVisibility(ESlateVisibility::Collapsed);
		Image_CountdownStart->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	int32 Index = Number - 1;

	if (!CountdownImages.IsValidIndex(Index) || !CountdownImages[Index]) {
		if (Anim_CountDown) {
			StopAnimation(Anim_CountDown);
		}
		if (Index != 0) {
			Image_Countdown->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if (Index == 0) {
			Image_CountdownStart->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (Index > 0) {
		Image_CountdownStart->SetVisibility(ESlateVisibility::Collapsed);
		Image_Countdown->SetBrushFromTexture(CountdownImages[Index]);
		Image_Countdown->SetVisibility(ESlateVisibility::Visible);
		Image_Countdown->SetRenderOpacity(1.f);
		RenderTransform.Scale = FVector2D(1.f, 1.f);
		if (Anim_CountDown) {
			StopAnimation(Anim_CountDown);
			PlayAnimation(Anim_CountDown, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f);
		}
	}
	else if (Index == 0) {
		Image_Countdown->SetVisibility(ESlateVisibility::Collapsed);
		Image_CountdownStart->SetVisibility(ESlateVisibility::Visible);
		Image_CountdownStart->SetRenderOpacity(1.f);
		if (Anim_CountDown) {
			StopAnimation(Anim_CountDownStart);
			PlayAnimation(Anim_CountDownStart, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f);
		}
	}
}

void UPlayer_ControllerWidget::SwitchLayout()
{
	APawn* MyPawn = GetOwningPlayerPawn();
	if (!IsValid(MyPawn)) return;

	if (NowPlayerUIState == EPlayerUIState::Countdown) {
		CanvasPanel_Countdown->SetVisibility(ESlateVisibility::Visible);
		CanvasPanel_Top->SetVisibility(ESlateVisibility::Collapsed);
		CanvasPanel_Playing_Bottom->SetVisibility(ESlateVisibility::Collapsed);
		CanvasPanel_Waiting_Bottom->SetVisibility(ESlateVisibility::Collapsed);
		Image_HitAlert->SetVisibility(ESlateVisibility::Collapsed);
		Text_Coin->SetVisibility(ESlateVisibility::Collapsed);
		Text_CanRespawn->SetVisibility(ESlateVisibility::Collapsed);
	}

	else if (NowPlayerUIState == EPlayerUIState::Playing) {
		CanvasPanel_Countdown->SetVisibility(ESlateVisibility::Collapsed);
		CanvasPanel_Top->SetVisibility(ESlateVisibility::Visible);
		CanvasPanel_Top->SetRenderOpacity(1.f);
		CanvasPanel_Playing_Bottom->SetVisibility(ESlateVisibility::Visible);
		CanvasPanel_Playing_Bottom->SetRenderOpacity(1.f);
		CanvasPanel_Waiting_Bottom->SetVisibility(ESlateVisibility::Collapsed);
		Image_HitAlert->SetRenderOpacity(0.f);
		Image_HitAlert->SetVisibility(ESlateVisibility::Visible);
		Text_Coin->SetVisibility(ESlateVisibility::Visible);
		Text_CanRespawn->SetVisibility(ESlateVisibility::Collapsed);
	}

	else if (NowPlayerUIState == EPlayerUIState::RespawnWaiting) {
		CanvasPanel_Countdown->SetVisibility(ESlateVisibility::Collapsed);
		CanvasPanel_Top->SetVisibility(ESlateVisibility::Visible);
		CanvasPanel_Top->SetRenderOpacity(1.f);
		CanvasPanel_Playing_Bottom->SetVisibility(ESlateVisibility::Collapsed);
		CanvasPanel_Waiting_Bottom->SetVisibility(ESlateVisibility::Visible);
		CanvasPanel_Waiting_Bottom->SetRenderOpacity(1.f);
		Image_HitAlert->SetVisibility(ESlateVisibility::Collapsed);
		Text_Coin->SetVisibility(ESlateVisibility::Visible);
		Text_CanRespawn->SetVisibility(ESlateVisibility::Collapsed);
	}

	else if (NowPlayerUIState == EPlayerUIState::StartWaiting) {
		CanvasPanel_Countdown->SetVisibility(ESlateVisibility::Collapsed);
		CanvasPanel_Top->SetVisibility(ESlateVisibility::Collapsed);
		CanvasPanel_Playing_Bottom->SetVisibility(ESlateVisibility::Collapsed);
		CanvasPanel_Waiting_Bottom->SetVisibility(ESlateVisibility::Collapsed);
		Image_HitAlert->SetVisibility(ESlateVisibility::Collapsed);
		Text_Coin->SetVisibility(ESlateVisibility::Collapsed);
		Text_CanRespawn->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPlayer_ControllerWidget::SetShopButtonVisible(bool bVisible)
{
	if (!Button_Shop) return;

	Button_Shop->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	Button_Shop->SetIsEnabled(true);
}

void UPlayer_ControllerWidget::SetShopClosedState(bool bClosed)
{
	if (Image_ShopClosedOverlay) Image_ShopClosedOverlay->SetVisibility(bClosed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UPlayer_ControllerWidget::SetMatchTime() {
	if (!Text_MatchTime) return;
	if (!MatchState) {
		Text_MatchTime->SetText(FText::FromString(TEXT("00:00")));
		return;
	}

	int32 TotalSeconds = MatchState->GetMatchTime();
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;

	Text_MatchTime->SetText(FText::FromString(FString::Printf(TEXT("%02d   %02d"), Minutes, Seconds)));
}

void UPlayer_ControllerWidget::ShowMatchEventCountdown(FName EventName, int32 SecondsUntilEvent)
{
	if (SecondsUntilEvent <= 0 || EventName.IsNone()) {
		HideMatchEventUI();
		return;
	}

	if (!Overlay_MatchEvent || !Text_MatchEventName || !Text_MatchEventTime) return;

	Overlay_MatchEvent->SetVisibility(ESlateVisibility::Visible);

	if (Image_WarningMatchEvent) {
		Image_WarningMatchEvent->SetVisibility(ESlateVisibility::Visible);
	}
	if (Image_AlertMatchEvent) {
		Image_AlertMatchEvent->SetVisibility(ESlateVisibility::Collapsed);
	}

	Text_MatchEventName->SetText(FText::FromName(EventName));
	Text_MatchEventTime->SetText(FText::AsNumber(SecondsUntilEvent));
}

void UPlayer_ControllerWidget::ShowMatchEventActive(FName EventName, int32 RemainingSeconds)
{
	if (RemainingSeconds <= 0 || EventName.IsNone()) {
		HideMatchEventUI();
		return;
	}
	if (!Overlay_MatchEvent || !Text_MatchEventName || !Text_MatchEventTime) return;

	Overlay_MatchEvent->SetVisibility(ESlateVisibility::Visible);

	if (Image_WarningMatchEvent) {
		Image_WarningMatchEvent->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Image_AlertMatchEvent) {
		Image_AlertMatchEvent->SetVisibility(ESlateVisibility::Visible);
	}

	Text_MatchEventName->SetText(FText::FromName(EventName));
	Text_MatchEventTime->SetText(FText::AsNumber(RemainingSeconds));
}

void UPlayer_ControllerWidget::ShowCanRespawnText()
{
	if (Text_CanRespawn) {
		Text_CanRespawn->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPlayer_ControllerWidget::HideMatchEventUI()
{
	if (Overlay_MatchEvent) {
		Overlay_MatchEvent->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Text_MatchEventName) {
		Text_MatchEventName->SetText(FText::GetEmpty());
	}

	if (Text_MatchEventTime) {
		Text_MatchEventTime->SetText(FText::GetEmpty());
	}

	if (Image_AlertMatchEvent) {
		Image_AlertMatchEvent->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Image_WarningMatchEvent) {
		Image_WarningMatchEvent->SetVisibility(ESlateVisibility::Collapsed);
	}
}

//각 Root내에 존재하는 NameToken을 이름으로 가진 이미지 자동 캐싱 (Image를 찾지 못한 경우 호출, 자식 Widget이 없을 때 까지 반복 호출)
UImage* UPlayer_ControllerWidget::FindImageByNameContains(UWidget* RootWidget, const FString& NameToken)
{
	if (!RootWidget) return nullptr;

	if (UImage* Image = Cast<UImage>(RootWidget)) {
		if (Image->GetName().Contains(NameToken)) return Image;
	}

	if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(RootWidget)) {
		int32 ChildCount = PanelWidget->GetChildrenCount();

		for (int32 i = 0; i < ChildCount; ++i) {
			UWidget* Child = PanelWidget->GetChildAt(i);
			if (UImage* FoundImage = FindImageByNameContains(Child, NameToken)) return FoundImage;
		}
	}

	return nullptr;
}

void UPlayer_ControllerWidget::SetCoin() {
	if (!Text_Coin) return;
	if (!OwnerState && OwnerCharacter) {
		OwnerState = OwnerCharacter->GetPlayerState<APlayer_State>();
	}

	int32 Coin = OwnerState ? OwnerState->GetPlayerCoin() : 0;
	Text_Coin->SetText(FText::AsNumber(Coin));
}

void UPlayer_ControllerWidget::SetRank() {
	if (!Text_Rank) return;
	if (!OwnerState && OwnerCharacter) {
		OwnerState = OwnerCharacter->GetPlayerState<APlayer_State>();
	}
	if (!OwnerState) {
		Text_Rank->SetText(FText::FromString(TEXT("-")));
		return;
	}
	Text_Rank->SetText(FText::AsNumber(OwnerState->GetPlayerRank()));
}

void UPlayer_ControllerWidget::SetItem() {
	if (!Image_ItemIcon) return;

	if (!OwnerState && OwnerCharacter) {
		OwnerState = OwnerCharacter->GetPlayerState<APlayer_State>();
	}

	if (!OwnerState) {
		Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TSubclassOf<AItem> Item = OwnerState->GetEquippedItem();
	if (!Item) {
		Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	AItem* NowItem = Item->GetDefaultObject<AItem>();
	if (!NowItem || !NowItem->ItemData || !NowItem->ItemData->ItemIcon) {
		Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Image_ItemIcon->SetBrushFromTexture(NowItem->ItemData->ItemIcon);
	Image_ItemIcon->SetColorAndOpacity(FLinearColor::White);
	Image_ItemIcon->SetBrushTintColor(FSlateColor(FLinearColor::White));
	Image_ItemIcon->SetRenderOpacity(1.f);
	Image_ItemIcon->SetVisibility(ESlateVisibility::Visible);
}

void UPlayer_ControllerWidget::SetWeapon()
{
	if (!Image_EquippmentIcon || !Image_EquippmentIcon_BG || !Overlay_WeaponUseCount || !OwnerCharacter) return;

	if (!OwnerCharacter->NowObjects && !OwnerCharacter->NowWeapon) {
		Image_EquippmentIcon->SetVisibility(ESlateVisibility::Collapsed);
		Image_EquippmentIcon_BG->SetVisibility(ESlateVisibility::Collapsed);
		HideWeaponUseCountUI();
		return;
	}
	else if (OwnerCharacter->NowWeapon && !OwnerCharacter->NowWeapon->WeaponData) {
		Image_EquippmentIcon->SetVisibility(ESlateVisibility::Collapsed);
		Image_EquippmentIcon_BG->SetVisibility(ESlateVisibility::Collapsed);
		HideWeaponUseCountUI();
		return;
	}
	else if (OwnerCharacter->NowObjects && !OwnerCharacter->NowObjects->ObjectsData) {
		Image_EquippmentIcon->SetVisibility(ESlateVisibility::Collapsed);
		Image_EquippmentIcon_BG->SetVisibility(ESlateVisibility::Collapsed);
		HideWeaponUseCountUI();
		return;
	}

	UTexture2D* EquippmentIcon = OwnerCharacter->GetWidgetEquippmentSlotIcon();

	//무기 아이콘 UI 설정
	if (!EquippmentIcon) {
		Image_EquippmentIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	else {
		Image_EquippmentIcon->SetBrushFromTexture(EquippmentIcon);
		Image_EquippmentIcon->SetColorAndOpacity(FLinearColor::White);
		Image_EquippmentIcon->SetRenderOpacity(1.f);
		Image_EquippmentIcon->SetVisibility(ESlateVisibility::Visible);
	}
	
	//장착물의 종류/등급에 따라 Icon의 BackGround 설정
	if (OwnerCharacter->NowWeapon && OwnerCharacter->NowWeapon->WeaponData) {
		switch (OwnerCharacter->NowWeapon->WeaponData->WeaponGrade) {
		case EWeaponGrade::B:
			Image_EquippmentIcon_BG->SetBrushFromTexture(EquippemntIcon_BGs[1]);
			break;
		case EWeaponGrade::A:
			Image_EquippmentIcon_BG->SetBrushFromTexture(EquippemntIcon_BGs[2]);
			break;
		case EWeaponGrade::S:
			Image_EquippmentIcon_BG->SetBrushFromTexture(EquippemntIcon_BGs[3]);
			break;
		default:
			Image_EquippmentIcon_BG->SetBrushFromTexture(EquippemntIcon_BGs[0]);
			break;
		}
		Image_EquippmentIcon_BG->SetColorAndOpacity(FLinearColor::White);
		Image_EquippmentIcon_BG->SetRenderOpacity(1.f);
		Image_EquippmentIcon_BG->SetVisibility(ESlateVisibility::Visible);
	}
	else if(OwnerCharacter->NowObjects && OwnerCharacter->NowObjects->ObjectsData){
		Image_EquippmentIcon_BG->SetBrushFromTexture(EquippemntIcon_BGs[0]);
		Image_EquippmentIcon_BG->SetColorAndOpacity(FLinearColor::White);
		Image_EquippmentIcon_BG->SetRenderOpacity(1.f);
		Image_EquippmentIcon_BG->SetVisibility(ESlateVisibility::Visible);
	}

	if (OwnerCharacter->NowWeapon) {
		OwnerCharacter->NowWeapon->OnWeaponUseCountChanged.RemoveAll(this);
		OwnerCharacter->NowWeapon->OnWeaponUseCountChanged.AddUObject(this, &UPlayer_ControllerWidget::SetWeaponUseCount);
		SetWeaponUseCount();
	}
	else {
		HideWeaponUseCountUI();
	}
}

void UPlayer_ControllerWidget::SetWeaponUseCount()
{
	UE_LOG(LogTemp, Warning, TEXT("SetWeaponUseCount CALLED"));

	if (!OwnerCharacter || !OwnerCharacter->NowWeapon || !OwnerCharacter->NowWeapon->WeaponData) {
		HideWeaponUseCountUI();
		return;
	}

	float UsePercent = OwnerCharacter->GetWidgetWeaponSlotPercent();
	int32 RemainUseCount = OwnerCharacter->NowWeapon ? OwnerCharacter->NowWeapon->NowUseCount : 0;
	int32 MaxUseCount = OwnerCharacter->NowWeapon ? OwnerCharacter->NowWeapon->WeaponData->MaxUseCount : 0;

	//무기의 남은 사용횟수가 없거나, 사용횟수를 사용하지 않는 경우(무기가 없거나, 물체를 장착 중인 경우) 숨김처리
	if (UsePercent <= 0.f || RemainUseCount <= 0 || !OwnerCharacter->NowWeapon) {
		HideWeaponUseCountUI();
		return;
	}

	if (Overlay_WeaponUseCount && Overlay_WeaponUseCount->GetVisibility() != ESlateVisibility::Visible) {
		Overlay_WeaponUseCount->SetVisibility(ESlateVisibility::Visible);
	}

	//MaxUseCount만큼 칸을 만들고 남은 사용횟수 만큼만 Image를 채우고 나머지는 비움 (MaxUseCount가 10 이하일때)
	if (MaxUseCount <= 10) {
		if (!HorizontalBox_WeaponUseCountOne) {
			HideWeaponUseCountUI();
			return;
		}
		HorizontalBox_WeaponUseCountOne->SetVisibility(ESlateVisibility::Visible);

		int32 OverlayCount = FMath::Clamp(MaxUseCount, 0, WeaponUseCountOne.Num());
		int32 ImageCount = FMath::Clamp(RemainUseCount, 0, WeaponUseCountOneInside.Num());

		for (int32 i = 0; i < WeaponUseCountOne.Num(); ++i) {
			UOverlay* Overlay_WeaponUseCountOne = WeaponUseCountOne[i].Get();
			UImage* Image_WeaponUseCountOneInside = WeaponUseCountOneInside[i].Get();

			bool bShowOverlay = i < OverlayCount;
			bool bShowImage = i < ImageCount;

			if (Overlay_WeaponUseCountOne) {
				bShowOverlay ? Overlay_WeaponUseCountOne->SetVisibility(ESlateVisibility::Visible) : Overlay_WeaponUseCountOne->SetVisibility(ESlateVisibility::Collapsed);
			}

			if (Image_WeaponUseCountOneInside) {
				bShowImage ? Image_WeaponUseCountOneInside->SetVisibility(ESlateVisibility::Visible) : Image_WeaponUseCountOneInside->SetVisibility(ESlateVisibility::Collapsed);
				Image_WeaponUseCountOneInside->SetColorAndOpacity(FLinearColor::White);
				Image_WeaponUseCountOneInside->SetRenderOpacity(1.f);
			}
		}
	}
	//MaxUseCount가 10 이상일 때 ProgressBar를 보여줌
	else if (MaxUseCount > 10) {
		if (!Overlay_WeaponUseCountOverTen || !ProgressBar_WeaponUseCountProgressBar) {
			HideWeaponUseCountUI();
			return;
		}

		Overlay_WeaponUseCountOverTen->SetVisibility(ESlateVisibility::Visible);
		ProgressBar_WeaponUseCountProgressBar->SetPercent(UsePercent);
		ProgressBar_WeaponUseCountProgressBar->SetFillColorAndOpacity(FLinearColor::White);
		ProgressBar_WeaponUseCountProgressBar->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPlayer_ControllerWidget::HideWeaponUseCountUI()
{
	if (Overlay_WeaponUseCount) {
		Overlay_WeaponUseCount->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (HorizontalBox_WeaponUseCountOne) {
		HorizontalBox_WeaponUseCountOne->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Overlay_WeaponUseCountOverTen) {
		Overlay_WeaponUseCountOverTen->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ProgressBar_WeaponUseCountProgressBar) {
		ProgressBar_WeaponUseCountProgressBar->SetPercent(0.f);
		ProgressBar_WeaponUseCountProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	}

	for (TObjectPtr<UOverlay>& Overlay_WeaponUseCountOne : WeaponUseCountOne) {
		if (Overlay_WeaponUseCountOne) {
			Overlay_WeaponUseCountOne->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	for (TObjectPtr<UImage>& Image_WeaponUseCountOneInside : WeaponUseCountOneInside) {
		if (Image_WeaponUseCountOneInside) {
			Image_WeaponUseCountOneInside->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPlayer_ControllerWidget::SetHPAlert(float NewHP, float MaxHP)
{
	if (!Image_HitAlert) return;

	if (!OwnerCharacter || OwnerCharacter->bIsOut) {
		GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
		Image_HitAlert->SetVisibility(ESlateVisibility::Collapsed);
		LastHitHP = NewHP;
		return;
	}

	float HPPercent = MaxHP > 0.f ? FMath::Clamp(NewHP / MaxHP, 0.f, 1.f) : 0.f;
	float TargetOpacity = HPPercent < 3.f ? (1.f - HPPercent) : 0.f;

	bool bDamaged = NewHP < LastHitHP;
	bool bHealedOrSame = NewHP >= LastHitHP;

	LastHitHP = NewHP;
	GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);

	if (bDamaged) {
		StartImageOpacity = HPPercent < 0.3f ? 1.f : FMath::Max(0.5f, HPPercent);
		EndImageOpacity = TargetOpacity;
	}
	else if (bHealedOrSame) {
		StartImageOpacity = Image_HitAlert->GetRenderOpacity();
		EndImageOpacity = TargetOpacity;
	}

	float FadeDuration = 0.5f;
	FadeElapsedTime = 0.f;

	if (StartImageOpacity <= 0.f && EndImageOpacity <= 0.f) {
		Image_HitAlert->SetRenderOpacity(0.f);
		Image_HitAlert->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Image_HitAlert->SetVisibility(ESlateVisibility::Visible);
	Image_HitAlert->SetRenderOpacity(StartImageOpacity);

	GetWorld()->GetTimerManager().SetTimer(FadeTimerHandle, this, &UPlayer_ControllerWidget::UpdateFadeOpacity, 0.015f, true);
}

void UPlayer_ControllerWidget::UpdateFadeOpacity()
{
	if (!Image_HitAlert || !GetWorld()) return;

	FadeElapsedTime += 0.015f;

	float Alpha = FMath::Clamp(FadeElapsedTime / 0.5f, 0.f, 1.f);
	float NewOpacity = FMath::Lerp(StartImageOpacity, EndImageOpacity, Alpha);

	Image_HitAlert->SetRenderOpacity(NewOpacity);

	if (Alpha >= 1.f) {
		if (EndImageOpacity <= 0.f) {
			Image_HitAlert->SetVisibility(ESlateVisibility::Collapsed);
		}
		else {
			Image_HitAlert->SetRenderOpacity(EndImageOpacity);
		}
		GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
	}
}

void UPlayer_ControllerWidget::HandleCoinChanged(int32 Coin) {
	SetCoin();
}

void UPlayer_ControllerWidget::HandleMatchTimeChanged(int32 MatchTime)
{
	SetMatchTime();
}

void UPlayer_ControllerWidget::HandleRankChanged(int32 Rank)
{
	SetRank();
}

void UPlayer_ControllerWidget::HandleHPChanged(float NewHP, float MaxHP)
{
	SetHPAlert(NewHP, MaxHP);
}


void UPlayer_ControllerWidget::HandleItemChanged(TSubclassOf<AItem> item, int32 itemUseCount) {
	SetItem();
}

void UPlayer_ControllerWidget::HandleWeaponChanged()
{
	SetWeapon();
}

void UPlayer_ControllerWidget::HandleWeaponUseCountChanged() {
	SetWeaponUseCount();
}

