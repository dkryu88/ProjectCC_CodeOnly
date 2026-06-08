// Fill out your copyright notice in the Description page of Project Settings.


#include "Match_ScoreBoardWidget.h"
#include "Player_ScoreBoardWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Player_State.h"


void UMatch_ScoreBoardWidget::InitWidget(APlayerController* ownerPC)
{
	OwnerPC = ownerPC;
	UpdateScoreBoard();

	//스코어보드가 입력을 막지 않도록 설정
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMatch_ScoreBoardWidget::NativeDestruct()
{
	EndAutoUpdate();
	Super::NativeDestruct();
}

void UMatch_ScoreBoardWidget::StartAutoUpdate() {
	if (!GetWorld()) return;
	if (GetWorld()->GetTimerManager().IsTimerActive(RefreshTimerHandle)) return;

	GetWorld()->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UMatch_ScoreBoardWidget::UpdateScoreBoard, RefreshInterval, true);
}

void UMatch_ScoreBoardWidget::EndAutoUpdate()
{
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
}

void UMatch_ScoreBoardWidget::UpdateScoreBoard()
{
	if (!VerticalBox_PlayerScore || !OwnerPC) return;

	VerticalBox_PlayerScore->ClearChildren();

	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS || !ScoreWidget) return;

	TArray<APlayer_State*> Players;

	for (APlayerState* PS : GS->PlayerArray) {
		if (APlayer_State* MyPS = Cast<APlayer_State>(PS)) {
			Players.Add(MyPS);
		}
	}

	//각 플레이어들의 순위로 스코어보드 순서 결정
	Players.Sort([](const APlayer_State& A, const APlayer_State& B) {
		if (A.GetPlayerRank() != B.GetPlayerRank()) {
			return A.GetPlayerRank() < B.GetPlayerRank();
		}

		//완전 동점일 경우 A를 위로 fallback
		return A.GetNickName() < B.GetNickName();
	});

	APlayer_State* LocalPS = OwnerPC->GetPlayerState<APlayer_State>();

	for (APlayer_State* PS : Players) {
		if (!PS) continue;

		UPlayer_ScoreBoardWidget* Score = CreateWidget<UPlayer_ScoreBoardWidget>(this, ScoreWidget);
		if (!Score) continue;

		const bool bIsSelf = (PS == LocalPS);
		Score->InitWidget(PS, bIsSelf);

		VerticalBox_PlayerScore->AddChild(Score);
	}
}


