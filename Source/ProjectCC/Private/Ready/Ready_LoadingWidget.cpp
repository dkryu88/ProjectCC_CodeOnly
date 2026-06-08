// Fill out your copyright notice in the Description page of Project Settings.


#include "Ready/Ready_LoadingWidget.h"
#include "Ready/Ready_GameState.h"
#include "Player_State.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

void UReady_LoadingWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime) {
	Super::NativeTick(MyGeometry, DeltaTime);
	RefreshUI();
}
//UI 갱신
void UReady_LoadingWidget::RefreshUI() {
	UWorld* World = GetWorld();
	if (!World) return;
	//Ready화면 GameState 읽어옴
	AReady_GameState* ReadyGameState = World->GetGameState<AReady_GameState>();
	if (!ReadyGameState) return;
	if (Text_MapName) {
		Text_MapName->SetText(FText::FromString(ReadyGameState->SelectedMapDisplayName));
	}

	APlayer_State* LeftPS = nullptr;
	APlayer_State* RightPS = nullptr;
	//각 플레이어의 Player_State 캐스팅
	if (ReadyGameState->PlayerArray.Num() > 0) {
		LeftPS = Cast<APlayer_State>(ReadyGameState->PlayerArray[0]);
	}

	if (ReadyGameState->PlayerArray.Num() > 1) {
		RightPS = Cast<APlayer_State>(ReadyGameState->PlayerArray[1]);
	}
	//각 플레이어의 Player_State에 저장된 닉네임, 플레이어 로딩 상태, 로딩 진행도 가져옴
	if (LeftPS) {
		if (Text_LeftPlayerNickname) {
			Text_LeftPlayerNickname->SetText(FText::FromString(LeftPS->GetNickName()));
		}
		if (Text_LeftState) {
			Text_LeftState->SetText(FText::FromString(GetStateText(LeftPS->GetReadySyncState())));
		}
		if (Progress_Left) {
			Progress_Left->SetPercent(GetStateProgress(LeftPS->GetReadySyncState()));
		}
		ApplyPortrait(Image_LeftPortrait, LeftPS->GetPortraitId());
	}

	if (RightPS) {
		if (Text_RightPlayerNickname) {
			Text_RightPlayerNickname->SetText(FText::FromString(RightPS->GetNickName()));
		}
		if (Text_RightState) {
			Text_RightState->SetText(FText::FromString(GetStateText(RightPS->GetReadySyncState())));
		}
		if (Progress_Right) {
			Progress_Right->SetPercent(GetStateProgress(RightPS->GetReadySyncState()));
		}
		ApplyPortrait(Image_RightPortrait, RightPS->GetPortraitId());
	}
}
//초상화 적용
void UReady_LoadingWidget::ApplyPortrait(UImage* TargetImage, int32 PortraitId)
{
	if (!TargetImage) return;
	if (!PortraitTextures.IsValidIndex(PortraitId)) return;
	if (PortraitTextures[PortraitId]) {
		FSlateBrush Brush;
		Brush.SetResourceObject(PortraitTextures[PortraitId]);
		TargetImage->SetBrush(Brush);
	}
}
//로딩 상태 메세지
FString UReady_LoadingWidget::GetStateText(EReadySyncState State) const
{
	switch (State) {
	case EReadySyncState::None:
		return TEXT("Ready to Connect");
	case EReadySyncState::JoinedReadyLevel:
		return TEXT("Enter Ready Level");
	case EReadySyncState::ProfileSynced:
		return TEXT("Profiel Sync Completed");
	case EReadySyncState::ReadyScreenLoaded:
		return TEXT("Ready to show loading Screen");
	case EReadySyncState::ReadyToTravel:
		return TEXT("Ready to Play");
	default:
		return TEXT("Fail Loading");
	}
}
//로딩 상태 진행도
float UReady_LoadingWidget::GetStateProgress(EReadySyncState State) const
{
	switch (State) {
	case EReadySyncState::None:
		return 0.f;
	case EReadySyncState::JoinedReadyLevel:
		return 0.25f;
	case EReadySyncState::ProfileSynced:
		return 0.5f;
	case EReadySyncState::ReadyScreenLoaded:
		return 0.75f;
	case EReadySyncState::ReadyToTravel:
		return 1.f;
	default:
		return 0.f;
	}
}
