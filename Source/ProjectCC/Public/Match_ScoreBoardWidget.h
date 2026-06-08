// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Match_ScoreBoardWidget.generated.h"

/**
 * 각 플레이어가 보게 될 스코어보드
 */

class UVerticalBox;
class UTexture2D;
class UPlayer_ScoreBoardWidget;
class APlayer_State;
class APlayerController;

UCLASS()
class PROJECTCC_API UMatch_ScoreBoardWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeDestruct() override;

	void InitWidget(APlayerController* ownerPC);
	void UpdateScoreBoard();

	void StartAutoUpdate();
	void EndAutoUpdate();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_PlayerScore;

	UPROPERTY(EditDefaultsOnly, Category="Scoreboard")
	TSubclassOf<UPlayer_ScoreBoardWidget> ScoreWidget;

	UPROPERTY()
	TObjectPtr<APlayerController> OwnerPC;

	UPROPERTY(EditDefaultsOnly, Category="Scoreboard")
	float RefreshInterval = 0.1f;

	FTimerHandle RefreshTimerHandle;
};
