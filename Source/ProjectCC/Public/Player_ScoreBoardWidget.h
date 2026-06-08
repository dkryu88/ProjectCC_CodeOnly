// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AllPlayMode_GameInstance.h"
#include "Player_ScoreBoardWidget.generated.h"

/**
 * 각 플레이어의 스코어를 보여주는 위젯 (스코어 보드 소속)
 */

class UTextBlock;
class UImage;
class APlayer_State;
class AItem;
class UTexture2D;
class UOverlay;

UCLASS()
class PROJECTCC_API UPlayer_ScoreBoardWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitWidget(APlayer_State* playerState, bool bIsSelf);
	void InitWidgetWithoutPlayerState(const FMatchResultData& Result, bool bIsSelf);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Rank;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Nickname;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Coin;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Eliminate;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Out;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Item;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ScoreBoard_Individual_BackGround;

	UPROPERTY(meta = (BindWIdget))
	TObjectPtr<UOverlay> Overlay_ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScoreBoard")
	TObjectPtr<UTexture2D> Image_ScoreBoard_Individual_BackGround_Owner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScoreBoard")
	TObjectPtr<UTexture2D> Image_ScoreBoard_Individual_BackGround_Enemy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ItemIcon")
	TObjectPtr<UTexture2D> Image_ItemIcon_Secret;
};
