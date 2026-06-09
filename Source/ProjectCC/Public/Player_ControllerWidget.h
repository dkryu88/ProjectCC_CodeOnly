// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player_ControllerWidget.generated.h"

/**
 * 플레이어 전체 화면에서 보일 UI
 */

class UTextBlock;
class UImage;
class UProgressBar;
class UHorizontalBox;
class UMatch_ShopWidget;
class UOverlay;
class APlayer_Character;
class APlayer_State;
class AMatch_State;
class AItem;
class AWeapon;
class AObjects;
class UCanvasPanel;
class UButton;

UENUM(BlueprintType)
enum class EPlayerUIState : uint8 {
	Playing,
	RespawnWaiting,
	StartWaiting,
	Countdown
};

UCLASS()
class PROJECTCC_API UPlayer_ControllerWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitWidget(APlayer_Character* Player);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BindEvents();
	void BindOwnerState();
	void UnBindEvents();

public:
	void SetUIState(EPlayerUIState NewState);
	void SetCountdown(int32 Number);
	void SwitchLayout();

	void ShowMatchEventCountdown(FName EventName, int32 SecondsUntilEvent);
	void ShowMatchEventActive(FName EventName, int32 RemainingSeconds);
	void ShowCanRespawnText();
	void HideMatchEventUI();

	void SetShopButtonVisible(bool bVisible);
	UButton* GetShopButton() const { return Button_Shop; }

	//Overlay내의 이미지 자동 캐싱 (Image를 찾지 못한 경우)
	UImage* FindImageByNameContains(UWidget* RootWidget, const FString& NameToken);
protected:
	void SetCoin();
	void SetRank();
	void SetItem();
	void SetWeapon();
	void SetWeaponUseCount();
	void HideWeaponUseCountUI();
	void SetHPAlert(float NewHP, float MaxHP);
	void SetMatchTime();

	void HandleCoinChanged(int32 Coin);
	void HandleItemChanged(TSubclassOf<AItem> item, int32 itemUseCount);
	void HandleWeaponChanged();
	void HandleWeaponUseCountChanged();
	void HandleMatchTimeChanged(int32 MatchTime);
	void HandleRankChanged(int32 Rank);
	void HandleHPChanged(float NewHP, float MaxHP);

	FTimerHandle FadeTimerHandle;
	FTimerHandle RetryPSBindTimerHandle;
	float LastHitHP;
	void UpdateFadeOpacity();

	//Opacity Fade 중 걸린 시간
	float FadeElapsedTime = 0.f;
	float StartImageOpacity;
	float EndImageOpacity;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CanvasPanel_Top;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MatchTime;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Rank;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CanvasPanel_Playing_Bottom;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CanvasPanel_Waiting_Bottom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Coin;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_EquippmentIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_EquippmentIcon_BG;

	//장착물 아이콘 백그라운드 0 : 물체, 1 : B등급 무기, 2 : A등급 무기, 3 : S등급 무기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TObjectPtr<UTexture2D>> EquippemntIcon_BGs;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ItemIcon;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_WeaponUseCount;

	UPROPERTY(meta = (BindWidget))
	TArray<TObjectPtr<UOverlay>> WeaponUseCountOne;

	UPROPERTY(meta = (BindWidget))
	TArray<TObjectPtr<UImage>> WeaponUseCountOneInside;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HorizontalBox_WeaponUseCountOne;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_WeaponUseCountOverTen;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_WeaponUseCountProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Shop;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CanvasPanel_Countdown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Countdown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_CountdownStart;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_HitAlert;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UOverlay> Overlay_MatchEvent;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> Text_MatchEventName;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> Text_MatchEventTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CanRespawn;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UImage> Image_WarningMatchEvent;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UImage> Image_AlertMatchEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Countdown")
	TArray<TObjectPtr<UTexture2D>> CountdownImages;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* Anim_CountDown;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* Anim_CountDownStart;

	UPROPERTY()
	TObjectPtr<APlayer_Character> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<APlayer_State> OwnerState;

	UPROPERTY()
	TObjectPtr<AMatch_State> MatchState;

	UPROPERTY()
	EPlayerUIState NowPlayerUIState = EPlayerUIState::Playing;

	UPROPERTY(EditDefaultsOnly, Category="Shop")
	TSubclassOf<UMatch_ShopWidget> Match_ShopWidget;

	UPROPERTY()
	TObjectPtr<UMatch_ShopWidget> ShopWidget;
};
