// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player_CharacterWidget.generated.h"

/**
 * 플레이어 캐릭터 머리 위에 떠있는 위젯
 */

class UTextBlock;
class UImage;
class UProgressBar;
class UCanvasPanel;
class APlayer_Character;
class APlayer_State;
class AItem;

UCLASS()
class PROJECTCC_API UPlayer_CharacterWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void InitWidget(APlayer_Character* Player);

	UFUNCTION(BlueprintCallable)
	void SetNickname();

	UFUNCTION(BlueprintCallable)
	void SetItemIcon();

	UFUNCTION(BlueprintCallable)
	void SetHPBar();

	UFUNCTION(BlueprintCallable)
	void SetCoin();

	UFUNCTION(BlueprintCallable)
	void SetUI();

protected:
	virtual void NativeDestruct() override;

protected:
	//플레이어 캐릭터 위젯 캔버스 (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel_CharacterWidget;
	//플레이어 닉네임 (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Nickname;
	//플레이어 닉네임 백그라운드 (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_NicknameBG;
	//플레이어 아이템 아이콘 (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ItemIcon;
	//플레이어 아이템 프레임 (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ItemIconFrame;
	//플레이어 아이템 백그라운드 (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ItemIconBG;
	//플레이어 HP (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_HP;
	//플레이어 HP Frame (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_HPBarFrame;
	//코인 아이콘 (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_CoinIcon;
	//플레이어 코인 수 (위젯)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Coin;
	//아이템 아이콘 (시크릿, 위젯)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecretItemIcon")
	TObjectPtr<UTexture2D> Image_SecretItemIcon;
	//현재 플레이어 캐릭터
	UPROPERTY()
	TObjectPtr<APlayer_Character> OwnerCharacter;
	//현재 플레이어 데이터
	UPROPERTY()
	TObjectPtr<APlayer_State> OwnerState;

public:
	void BindCharacterEvents();
	void UnbindCharacterEvents();

	void HandleHPChanged(float NewHP, float MaxHP);
	void HandleItemChanged(TSubclassOf<AItem> item, int32 UseCount);
	void HandleNicknameChanged(const FString& Nickname);
	void HandleCoinChanged(int32 Coin);
};
