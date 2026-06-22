// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player_State.h"
#include "Ready_LoadingWidget.generated.h"

/**
 * 
 */

class UTextBlock;
class UImage;
class UProgressBar;

UCLASS()
class PROJECTCC_API UReady_LoadingWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

protected:
	//선정된 맵 이름
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_MapName;

	//왼쪽/오른쪽 플레이어 닉네임
	//UPROPERTY(meta=(BindWidget))
	//TObjectPtr<UTextBlock> Text_LeftPlayerNickname;
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UTextBlock> Text_RightPlayerNickname;
	////왼쪽/오른쪽 로딩 상태
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UTextBlock> Text_LeftState;
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UTextBlock> Text_RightState;
	////왼쪽/오른쪽 로딩 진행도
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UProgressBar> Progress_Left;
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UProgressBar> Progress_Right;
	////왼쪽/오른쪽 초상화
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UImage> Image_LeftPortrait;
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UImage> Image_RightPortrait;
	
	//초상화 이미지
	UPROPERTY(EditDefaultsOnly, Category="Portrait")
	TArray<TObjectPtr<UTexture2D>> PortraitTextures;

	void RefreshUI();
	void ApplyPortrait(UImage* TargetImage, int32 PortraitId);
	FString GetStateText(EReadySyncState State) const;
	float GetStateProgress(EReadySyncState State) const;

protected:
	//[4인]추가-기존 Left,Right 변수 대신 1~4번의 변수로 통일, BindWidget->BindWidgetOptional
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Player1_Nickname;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Player2_Nickname;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Player3_Nickname;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Player4_Nickname;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Player1_State;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Player2_State;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Player3_State;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Player4_State;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Progress_Player1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Progress_Player2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Progress_Player3;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Progress_Player4;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Player1_Portrait;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Player2_Portrait;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Player3_Portrait;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Player4_Portrait;

	//[4인]추가-컴포넌트를 담을 배열
	TArray<UTextBlock*> NicknameTexts;
	TArray<UTextBlock*> StateTexts;
	TArray<UProgressBar*> ProgressBars;
	TArray<UImage*> PortraitImages;
	//[4인]추가
	virtual void NativeConstruct() override;
};
