// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/Title_Widget.h"
#include "Title/Title_PlayerController.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Throbber.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "AllPlayMode_GameInstance.h"
#include "Framework/Application/SlateApplication.h"

void UTitle_Widget::NativeConstruct() {
	Super::NativeConstruct();

	if (Button_TwoPlayer) {
		Button_TwoPlayer->OnClicked.AddUniqueDynamic(this, &UTitle_Widget::HandleTwoPlayerButtonClicked);
	}
	if (Button_FourPlayer) {
		Button_FourPlayer->OnClicked.AddUniqueDynamic(this, &UTitle_Widget::HandleFourPlayerButtonClicked);
	}
	if (Image_TwoPlayerButton) {
		Image_TwoPlayerButton->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Image_FourPlayerButton) {
		Image_FourPlayerButton->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (Image_SelectedFrame) {
		Image_SelectedFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(Image_SelectedFrame->Slot)) {
			FrameSlot->SetZOrder(10);
			FrameSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		}
	}

	if (Button_Play) {
		//Play버튼에 함수 바인딩
		Button_Play->OnClicked.AddUniqueDynamic(this, &UTitle_Widget::HandlePlayButtonClicked);
	}

	if (Button_Exit) {
		//Play버튼에 함수 바인딩
		Button_Exit->OnClicked.AddUniqueDynamic(this, &UTitle_Widget::HandleExitButtonClicked);
	}

	if (Button_CancelMatch) {
		Button_CancelMatch->OnClicked.AddUniqueDynamic(this, &UTitle_Widget::HandleCancelButtonClicked);
		Button_CancelMatch->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Throbber_Matching) {
		Throbber_Matching->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Text_Status) {
		//위젯이 처음 화면에 생성될 때 보이는 기본 상태 메세지
		Text_Status->SetText(FText::FromString(TEXT("Enter Your Nickname.")));
	}

	//ApplyMatchMode(EMatchMode::TwoPlayers, true);
	//[머지][버그] 매치모드 선택한 걸 가져와서 Image_SelectedFrame이 유지되도록
	UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	if (GameInstance) {
		EMatchMode SavedMatchMode = GameInstance->GetSelectedMatchMode();
		ApplyMatchMode(SavedMatchMode, true);
	}
	else {	//보험용
		ApplyMatchMode(EMatchMode::TwoPlayers, true);
	}
}

void UTitle_Widget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bFrameMoving && Image_SelectedFrame) {
		UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(Image_SelectedFrame->Slot);
		if (FrameSlot) {
			FVector2D CurrentPosition = FrameSlot->GetPosition();
			FVector2D NewPosition = FMath::Vector2DInterpTo(CurrentPosition, TargetFramePosition, InDeltaTime, FrameMoveSpeed);

			FrameSlot->SetPosition(NewPosition);
			
			float Distance = FVector2D::Distance(NewPosition, TargetFramePosition);

			if (Distance <= 0.5f) {
				FrameSlot->SetPosition(TargetFramePosition);
				bFrameMoving = false;
			}
		}
	}

	if (bStatusFading && Text_Status) {
		StatusElapsedTime += InDeltaTime;

		if (StatusElapsedTime >= ErrorVisibleTime) {
			float FadeElapsed = StatusElapsedTime - ErrorVisibleTime;
			float Alpha = 1.f - FMath::Clamp(FadeElapsed / ErrorFadeTime, 0.f, 1.f);

			Text_Status->SetRenderOpacity(Alpha);

			if (Alpha <= 0.f) {
				ClearStatusMessage();
			}
		}
	}

}

//닉네임 획득
FString UTitle_Widget::GetNicknameString() {
	//입력칸이 없다면 빈 문자열 반환
	if (!EditableTextBox_Nickname) {
		return FString();
	}

	//닉네임 입력칸 내에 입력받은 닉네임 반환
	return EditableTextBox_Nickname->GetText().ToString();
}

//닉네임 설정
void UTitle_Widget::SetNicknameText(const FString& nickname)
{
	if (!EditableTextBox_Nickname) return;

	EditableTextBox_Nickname->SetText(FText::FromString(nickname));
	RefreshNicknameBox();
}

//닉네임 수정, 입력 허용/방지
void UTitle_Widget::SetNicknameLocked(bool bLocked)
{
	bNicknameLocked = bLocked;

	if (EditableTextBox_Nickname) {
		EditableTextBox_Nickname->SetIsReadOnly(bLocked);

		if (bLocked) {
			EditableTextBox_Nickname->SetHintText(FText::GetEmpty());
		}
		else {
			RefreshNicknameBox();
		}
	}
}

void UTitle_Widget::SetMatchingMode(bool bMatching)
{
	if (Button_Play) {
		Button_Play->SetVisibility(bMatching ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		Button_Play->SetIsEnabled(!bMatching);
	}
	if (Throbber_Matching) {
		Throbber_Matching->SetVisibility(bMatching ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_CancelMatch) {
		Button_CancelMatch->SetVisibility(bMatching ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_CancelMatch->SetIsEnabled(bMatching);
	}

	if (Button_TwoPlayer) {
		Button_TwoPlayer->SetIsEnabled(!bMatching);
	}
	if (Button_FourPlayer) {
		Button_FourPlayer->SetIsEnabled(!bMatching);
	}
}

void UTitle_Widget::SetJoinCompleteMode()
{
	if (Button_Play) {
		Button_Play->SetVisibility(ESlateVisibility::Collapsed);
		Button_Play->SetIsEnabled(false);
	}

	if (Throbber_Matching) {
		Throbber_Matching->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Button_CancelMatch) {
		Button_CancelMatch->SetVisibility(ESlateVisibility::Collapsed);
		Button_CancelMatch->SetIsEnabled(false);
	}

	//[머지][버그] 클라이언트가 Matching Complete 방송하면서 4인방 인원모집 대기중일 때 2인/4인버튼 선택되는 버그 수정
	if (Button_TwoPlayer) {
		Button_TwoPlayer->SetIsEnabled(false);
	}
	if (Button_FourPlayer) {
		Button_FourPlayer->SetIsEnabled(false);
	}

	SetNicknameLocked(true);
}

//Text_Status의 Text 변경
void UTitle_Widget::SetStatusMessage(const FText& text)
{
	if (!Text_Status) return;

	CurrentStatusVisibleTIme = ErrorVisibleTime;

	Text_Status->SetText(text);
	Text_Status->SetVisibility(ESlateVisibility::Visible);
	Text_Status->SetRenderOpacity(1.f);

	bStatusFading = true;
	StatusElapsedTime = 0.f;
	
}

//키보드를 닉네임 입력칸에 바인딩
void UTitle_Widget::FocusNicknameBox()
{
	if (EditableTextBox_Nickname) {
		EditableTextBox_Nickname->SetKeyboardFocus();
	}
}

void UTitle_Widget::ClearStatusMessage()
{
	if (Text_Status) {
		Text_Status->SetText(FText::GetEmpty());
		Text_Status->SetVisibility(ESlateVisibility::Collapsed);
		Text_Status->SetRenderOpacity(1.f);
	}

	bStatusFading = false;
	StatusElapsedTime = 0.f;
}

void UTitle_Widget::SetStatusMessageShowing(const FText& text)
{
	if (!Text_Status) return;

	Text_Status->SetText(text);
	Text_Status->SetVisibility(ESlateVisibility::Visible);
	Text_Status->SetRenderOpacity(1.f);

	bStatusFading = false;
	StatusElapsedTime = 0.f;
}

void UTitle_Widget::SetStatusMessageFadeOut(const FText& text, float VisibleTime)
{
	if (!Text_Status) return;

	CurrentStatusVisibleTIme = VisibleTime;

	Text_Status->SetText(text);
	Text_Status->SetVisibility(ESlateVisibility::Visible);
	Text_Status->SetRenderOpacity(1.f);

	bStatusFading = true;
	StatusElapsedTime = 0.f;
}

//OnTitlePlayRequest 이벤트를 발생시켜 입력받은 닉네임을 PlayerController가 저장하도록 지정
void UTitle_Widget::HandlePlayButtonClicked()
{
	OnTitlePlayRequested.Broadcast(GetNicknameString(), SelectedMatchMode);
}

void UTitle_Widget::HandleCancelButtonClicked() {
	OnTitleCancelRequested.Broadcast();
}

void UTitle_Widget::HandleExitButtonClicked()
{
	ATitle_PlayerController* PC = GetOwningPlayer<ATitle_PlayerController>();
	if (!PC) return;

	PC->ToggleAdditionalWidget();

}

void UTitle_Widget::HandleTwoPlayerButtonClicked()
{
	SetMatchMode(EMatchMode::TwoPlayers);
}

void UTitle_Widget::HandleFourPlayerButtonClicked()
{
	SetMatchMode(EMatchMode::FourPlayers);
}

//닉네임 입력 박스 텍스트 변경 시 갱신
void UTitle_Widget::HandleNickNameTextChanged(const FText& NewText)
{
	FString ChangedText = NewText.ToString().TrimStartAndEnd();
	if (!ChangedText.IsEmpty()) {
		ClearStatusMessage();
	}
}

void UTitle_Widget::SetMatchMode(EMatchMode TheMatchMode)
{
	ApplyMatchMode(TheMatchMode, false);
}

//닉네임 입력 박스 갱신
void UTitle_Widget::RefreshNicknameBox()
{
	if (!EditableTextBox_Nickname) return;

	bool bHasText = !GetNicknameString().IsEmpty();
	bool bIsFocused = EditableTextBox_Nickname->HasKeyboardFocus();

	if (bHasText || bIsFocused) {
		EditableTextBox_Nickname->SetHintText(FText::GetEmpty());
	}
	else {
		EditableTextBox_Nickname->SetHintText(FText::FromString(TEXT("Enter Your Nickname.")));
	}
}

void UTitle_Widget::ApplyMatchMode(EMatchMode NewMatchMode, bool bInstant)
{
	if (SelectedMatchMode == NewMatchMode && !bInstant) return;

	SelectedMatchMode = NewMatchMode;

	RefreshMatchModeButtons();
	UpdateSelectedFrameTargetPosition();

	if (bInstant) SnapSelectedFrameToTargetLocation();
	else bFrameMoving = true;
}

void UTitle_Widget::RefreshMatchModeButtons()
{
	bool bTwoPlayerSelected = SelectedMatchMode == EMatchMode::TwoPlayers;
	bool bFourPlayerSelected = SelectedMatchMode == EMatchMode::FourPlayers;

	if (Image_TwoPlayerButton) {
		Image_TwoPlayerButton->SetColorAndOpacity(bTwoPlayerSelected ? FLinearColor::White : UnSelectedButtonTint);
	}
	if (Image_FourPlayerButton) {
		Image_FourPlayerButton->SetColorAndOpacity(bFourPlayerSelected ? FLinearColor::White : UnSelectedButtonTint);
	}

	if (Image_SelectedFrame) {
		Image_SelectedFrame->SetColorAndOpacity(FLinearColor::White);
	}
}

void UTitle_Widget::UpdateSelectedFrameTargetPosition()
{
	if (!Image_SelectedFrame) return;

	UWidget* TargetWidget = nullptr;

	switch (SelectedMatchMode) {
	case EMatchMode::TwoPlayers:
		TargetWidget = Overlay_TwoPlayer;
		break;
	case EMatchMode::FourPlayers:
		TargetWidget = Overlay_FourPlayer;
		break;
	default:
		break;
	}

	if (!TargetWidget) return;

	UCanvasPanelSlot* TargetSlot = Cast<UCanvasPanelSlot>(TargetWidget->Slot);
	UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(Image_SelectedFrame->Slot);

	if (!TargetSlot || !FrameSlot) return;

	FVector2D TargetPosition = TargetSlot->GetPosition();
	FVector2D TargetSize = TargetSlot->GetSize();
	FVector2D TargetAlignment = TargetSlot->GetAlignment();

	//버튼의 실제 중앙 좌표 계산
	FVector2D TargetCenterPosition = TargetPosition + TargetSize * (FVector2D(0.5f, 0.5f) - TargetAlignment);
		
	FrameSlot->SetAlignment(FVector2D(0.5f, 0.5f));

	TargetFramePosition = TargetCenterPosition + FramePositionOffset;
}

void UTitle_Widget::SnapSelectedFrameToTargetLocation()
{
	if (!Image_SelectedFrame) return;

	UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(Image_SelectedFrame->Slot);

	if (!FrameSlot) return;

	FrameSlot->SetPosition(TargetFramePosition);
	bFrameMoving = false;
}


