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
//void UReady_LoadingWidget::RefreshUI() {
//	UWorld* World = GetWorld();
//	if (!World) return;
//	//Ready화면 GameState 읽어옴
//	AReady_GameState* ReadyGameState = World->GetGameState<AReady_GameState>();
//	if (!ReadyGameState) return;
//	if (Text_MapName) {
//		Text_MapName->SetText(FText::FromString(ReadyGameState->SelectedMapDisplayName));
//	}
//
//	APlayer_State* LeftPS = nullptr;
//	APlayer_State* RightPS = nullptr;
//	//각 플레이어의 Player_State 캐스팅
//	if (ReadyGameState->PlayerArray.Num() > 0) {
//		LeftPS = Cast<APlayer_State>(ReadyGameState->PlayerArray[0]);
//	}
//
//	if (ReadyGameState->PlayerArray.Num() > 1) {
//		RightPS = Cast<APlayer_State>(ReadyGameState->PlayerArray[1]);
//	}
//	//각 플레이어의 Player_State에 저장된 닉네임, 플레이어 로딩 상태, 로딩 진행도 가져옴
//	if (LeftPS) {
//		if (Text_LeftPlayerNickname) {
//			Text_LeftPlayerNickname->SetText(FText::FromString(LeftPS->GetNickName()));
//		}
//		if (Text_LeftState) {
//			Text_LeftState->SetText(FText::FromString(GetStateText(LeftPS->GetReadySyncState())));
//		}
//		if (Progress_Left) {
//			Progress_Left->SetPercent(GetStateProgress(LeftPS->GetReadySyncState()));
//		}
//		ApplyPortrait(Image_LeftPortrait, LeftPS->GetPortraitId());
//	}
//
//	if (RightPS) {
//		if (Text_RightPlayerNickname) {
//			Text_RightPlayerNickname->SetText(FText::FromString(RightPS->GetNickName()));
//		}
//		if (Text_RightState) {
//			Text_RightState->SetText(FText::FromString(GetStateText(RightPS->GetReadySyncState())));
//		}
//		if (Progress_Right) {
//			Progress_Right->SetPercent(GetStateProgress(RightPS->GetReadySyncState()));
//		}
//		ApplyPortrait(Image_RightPortrait, RightPS->GetPortraitId());
//	}
//}
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

//[4인]추가
void UReady_LoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 배열 초기화 및 순서대로 담기 (순서 1->2->3->4 보장)
	NicknameTexts = { Text_Player1_Nickname, Text_Player2_Nickname, Text_Player3_Nickname, Text_Player4_Nickname };
	StateTexts = { Text_Player1_State, Text_Player2_State, Text_Player3_State, Text_Player4_State };
	ProgressBars = { Progress_Player1, Progress_Player2, Progress_Player3, Progress_Player4 };
	PortraitImages = { Image_Player1_Portrait, Image_Player2_Portrait, Image_Player3_Portrait, Image_Player4_Portrait };
}

//[4인]수정-재정의(새롭게 선언한 변수에 맞춰서)
void UReady_LoadingWidget::RefreshUI()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AReady_GameState* ReadyGameState = World->GetGameState<AReady_GameState>();
	if (!ReadyGameState) return;

	if (Text_MapName) {
		Text_MapName->SetText(FText::FromString(ReadyGameState->SelectedMapDisplayName));
	}

	//[수정] 로딩창에서 첫번째 칸에 본인의 아이디와 초상화가 나오게
	// 4개의 모든 슬롯을 숨김상태로 초기화
	for (int32 i = 0; i < 4; ++i) {
		if (NicknameTexts.IsValidIndex(i) && NicknameTexts[i]) NicknameTexts[i]->SetVisibility(ESlateVisibility::Collapsed);
		if (StateTexts.IsValidIndex(i) && StateTexts[i]) StateTexts[i]->SetVisibility(ESlateVisibility::Collapsed);
		if (ProgressBars.IsValidIndex(i) && ProgressBars[i]) ProgressBars[i]->SetVisibility(ESlateVisibility::Collapsed);
		if (PortraitImages.IsValidIndex(i) && PortraitImages[i]) PortraitImages[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// 본인의 PlayerState를 가져옴
	APlayerController* LocalPC = GetOwningPlayer();
	APlayerState* LocalPS = LocalPC ? LocalPC->PlayerState : nullptr;

	// 본인은 0번자리이므로, 다른사람들이 사용할 시작번호 1 초기화
	int32 NextOtherPlayerSlot = 1;

	for (APlayerState* PSBase : ReadyGameState->PlayerArray) {
		APlayer_State* PS = Cast<APlayer_State>(PSBase);

		if (PS) {
			int32 SlotIndex = 0;
			if (PS == LocalPS) {	//본인은 0번
				SlotIndex = 0;
			}
			else {	//다른사람은 1번부터 배정
				SlotIndex = NextOtherPlayerSlot;
				NextOtherPlayerSlot++;
			}

			if (SlotIndex >= 0 && SlotIndex < 4) {
				if (NicknameTexts.IsValidIndex(SlotIndex) && NicknameTexts[SlotIndex]) {
					NicknameTexts[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
					NicknameTexts[SlotIndex]->SetText(FText::FromString(PS->GetNickName()));
				}
				if (StateTexts.IsValidIndex(SlotIndex) && StateTexts[SlotIndex]) {
					StateTexts[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
					StateTexts[SlotIndex]->SetText(FText::FromString(GetStateText(PS->GetReadySyncState())));
				}
				if (ProgressBars.IsValidIndex(SlotIndex) && ProgressBars[SlotIndex]) {
					ProgressBars[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
					ProgressBars[SlotIndex]->SetPercent(GetStateProgress(PS->GetReadySyncState()));
				}
				if (PortraitImages.IsValidIndex(SlotIndex) && PortraitImages[SlotIndex]) {
					PortraitImages[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
					ApplyPortrait(PortraitImages[SlotIndex], PS->GetPortraitId());
				}
			}
		}
	}

}

