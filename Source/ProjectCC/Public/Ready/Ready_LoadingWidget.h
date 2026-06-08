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
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_LeftPlayerNickname;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RightPlayerNickname;
	//왼쪽/오른쪽 로딩 상태
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_LeftState;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RightState;
	//왼쪽/오른쪽 로딩 진행도
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Progress_Left;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Progress_Right;
	//왼쪽/오른쪽 초상화
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_LeftPortrait;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_RightPortrait;
	//초상화 이미지
	UPROPERTY(EditDefaultsOnly, Category="Portrait")
	TArray<TObjectPtr<UTexture2D>> PortraitTextures;

	void RefreshUI();
	void ApplyPortrait(UImage* TargetImage, int32 PortraitId);
	FString GetStateText(EReadySyncState State) const;
	float GetStateProgress(EReadySyncState State) const;

};
