// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AllPlayMode_GameInstance.h"
#include "Player_ResultWidget.generated.h"

/**
 * 
 */

class UTextBlock;
class UImage;
class UTexture2D;
class UPlayer_ScoreBoardWidget;
class UVerticalBox;
class UWidgetAnimation;
class APlayer_State;

USTRUCT(BlueprintType)
struct FResultBackgroundData {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName MapName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> BackgroundTexture = nullptr;
};

UCLASS()
class PROJECTCC_API UPlayer_ResultWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitWidget(const FMatchResultData& OwnData, const TArray<FMatchResultData>& Results);
	void SetAllPlayerResults(FString OwnNickname, int32 Rank, const TArray<FMatchResultData>& Results);
	void ShowPlayerRankMedal(int32 Rank);
	void SetExitEnabled(bool bEndEnabled);

	void SetBackgroundImageByMapName(FName MapName);

protected:
	UPROPERTY()
	TObjectPtr<APlayerController> OwnerPC;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Player_OwnNickname;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_RankMedal;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* Anim_RankMedal;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Background;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Rank;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Coin;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CanExit;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_PlayerResults;

	UPROPERTY(EditDefaultsOnly, Category = "Scoreboard")
	TSubclassOf<UPlayer_ScoreBoardWidget> ScoreWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Result")
	TArray<FResultBackgroundData> ResultBackgrounds;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result")
	TObjectPtr<UTexture2D> DefaultBackgroundTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result")
	TArray<TObjectPtr<UTexture2D>> RankMedals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result Medal")
	float StartSize = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result Medal")
	float EndSize = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result Medal")
	float SizeAnimDuration = 2.f;

	FTimerHandle ShowMedalTimer;
};
