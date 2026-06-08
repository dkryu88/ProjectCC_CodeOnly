// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/Title_Widget.h"
#include "Title/Title_PlayerController.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Throbber.h"
#include "Framework/Application/SlateApplication.h"

void UTitle_Widget::NativeConstruct() {
	Super::NativeConstruct();

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
}

void UTitle_Widget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bStatusFading || !Text_Status) return;

	StatusElapsedTime += InDeltaTime;

	if (StatusElapsedTime < ErrorVisibleTime) return;

	float FadeElapsed = StatusElapsedTime - ErrorVisibleTime;
	float Alpha = 1.f - FMath::Clamp(FadeElapsed / ErrorFadeTime, 0.f, 1.f);

	Text_Status->SetRenderOpacity(Alpha);

	if (Alpha <= 0.f) {
		ClearStatusMessage();
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
	OnTitlePlayRequested.Broadcast(GetNicknameString());
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

//닉네임 입력 박스 텍스트 변경 시 갱신
void UTitle_Widget::HandleNickNameTextChanged(const FText& NewText)
{
	FString ChangedText = NewText.ToString().TrimStartAndEnd();
	if (!ChangedText.IsEmpty()) {
		ClearStatusMessage();
	}
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


