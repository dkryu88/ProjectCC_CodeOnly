// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/Title_PlayerController.h"
#include "Title/Title_Widget.h"
#include "Title/Title_PlayerCharacter.h"
#include "Title/Title_AdditionalWidget.h"
#include "AllPlayMode_GameInstance.h"
#include "AllPlayMode_SessionSubsystem.h"
#include "Camera/CameraActor.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/Button.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"


ATitle_PlayerController::ATitle_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ATitle_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetTitleInputMode();

	if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		if (UAllPlayMode_SessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAllPlayMode_SessionSubsystem>()) {
			SessionSubsystem->OnSessionStateChanged.AddDynamic(this, &ATitle_PlayerController::HandleSessionStateChanged);
			HandleSessionStateChanged(SessionSubsystem->LastUIState, SessionSubsystem->LastUIMessage);
		}
	}

	CreateAndShowTitleWidget();
	FindPreviewActor();
	FindAndSetTitleCamera();
}

void ATitle_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdatePreviewRotation();
}

//타이틀 화면 키 바인딩
void ATitle_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent) {
		return;
	}
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATitle_PlayerController::OnLeftMousePressed);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ATitle_PlayerController::OnLeftMouseReleased);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATitle_PlayerController::ToggleAdditionalWidget);
}

//타이틀 화면 입력 설정
void ATitle_PlayerController::SetTitleInputMode()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	//마우스 입력 획득 중에도 커서를 숨기지 않음
	InputMode.SetHideCursorDuringCapture(false);
	//마우스를 View에 강제로 가두지 않도록 설정
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	
}
//추가 위젯 열기/닫기
void ATitle_PlayerController::ToggleAdditionalWidget()
{
	if (!IsLocalController()) return;

	if (!AdditionalWidget && Title_AdditionalWidget) {
		AdditionalWidget = CreateWidget<UTitle_AdditionalWidget>(this, Title_AdditionalWidget);
		if (AdditionalWidget) {
			AdditionalWidget->AddToViewport(200);
			AdditionalWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (!AdditionalWidget) return;

	bool bVisible = AdditionalWidget->GetVisibility() == ESlateVisibility::Visible || AdditionalWidget->GetVisibility() == ESlateVisibility::HitTestInvisible;

	if (bVisible) {
		CloseAdditionalWidget();
		TitleWidgetInstance->Button_Exit->SetVisibility(ESlateVisibility::Visible);
	}
	else {
		AdditionalWidget->SetVisibility(ESlateVisibility::Visible);
		TitleWidgetInstance->Button_Exit->SetVisibility(ESlateVisibility::Collapsed);
	}
}
//추가 위젯 닫기
void ATitle_PlayerController::CloseAdditionalWidget()
{
	if (!IsLocalController()) return;
	if (!AdditionalWidget) return;

	AdditionalWidget->SetVisibility(ESlateVisibility::Collapsed);
}

//게임 종료
void ATitle_PlayerController::ConfirmQuitGame()
{
	if (!IsLocalController()) return;

	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

//타이틀 화면 카메라 검색 및 설정
void ATitle_PlayerController::FindAndSetTitleCamera()
{
	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), FoundCameras);

	for (AActor* FoundActor : FoundCameras) {
		if (!FoundActor) continue;
		if (TitleCameraTag.IsNone() || FoundActor->ActorHasTag(TitleCameraTag)) {
			TitleCamera = Cast<ACameraActor>(FoundActor);
			break;
		}
	}

	//타이틀 카메라가 없는데 발견된 카메라의 수가 1이상이라면 첫번째 카메라를 사용
	if (!TitleCamera && FoundCameras.Num() > 0) {
		TitleCamera = Cast<ACameraActor>(FoundCameras[0]);
	}
	//발견된 타이틀 카메라로 화면 전환
	if (TitleCamera) {
		SetViewTargetWithBlend(TitleCamera, 0.f);
	}
}

//타이틀 화면 위젯 배치
void ATitle_PlayerController::CreateAndShowTitleWidget()
{
	if (!IsLocalController()) return;
	if (!TitleWidget) return;
	//실제로 화면에 위젯 생성
	TitleWidgetInstance = CreateWidget<UTitle_Widget>(this, TitleWidget);

	if (!TitleWidgetInstance) return;

	TitleWidgetInstance->AddToViewport(0);
	//위젯의 Play/Cancel 버튼을 HandlePlayRequested 함수에 바인딩
	TitleWidgetInstance->OnTitlePlayRequested.AddUniqueDynamic(this, &ATitle_PlayerController::HandlePlayRequested);
	TitleWidgetInstance->OnTitleCancelRequested.AddUniqueDynamic(this, &ATitle_PlayerController::HandleCancelRequested);

	UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	if (GameInstance) {
		FString SavedNickname = GameInstance->GetPlayerLocalNickname();
		if (!SavedNickname.IsEmpty()) {
			TitleWidgetInstance->SetNicknameText(SavedNickname);
		}

		//매칭을 진행하다가 Host가 된 경우 UI 유지 
		bool bKeepLocked = GameInstance->GetMatchFlowState() == EMatchFlowState::Searching || GameInstance->bPendingCreateLANSession;
		if (bKeepLocked) {
			TitleWidgetInstance->SetNicknameLocked(bKeepLocked);
			TitleWidgetInstance->SetMatchingMode(bKeepLocked);
			TitleWidgetInstance->SetStatusMessageShowing(FText::FromString(TEXT("Matching...")));
			return;
		}
	}

	TitleWidgetInstance->ClearStatusMessage();
}

//프리뷰 캐릭터 검색
void ATitle_PlayerController::FindPreviewActor()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATitle_PlayerCharacter::StaticClass(), FoundActors);

	for (AActor* FoundActor : FoundActors) {
		if (!FoundActor) continue;

		if (PreviewActorTag.IsNone() || FoundActor->ActorHasTag(PreviewActorTag)) {
			PreviewCharacter = Cast<ATitle_PlayerCharacter>(FoundActor);
			break;
		}
	}
	//프리뷰 캐릭터가 없는데 발견된 캐릭터의 수가 1이상이라면 첫번째 캐릭터를 사용
	if (!PreviewCharacter && FoundActors.Num() > 0) {
		PreviewCharacter = Cast<ATitle_PlayerCharacter>(FoundActors[0]);
	}

}

//마우스로 프리뷰 캐릭터 회전
void ATitle_PlayerController::UpdatePreviewRotation()
{
	if (!bIsPreviewDragging || !PreviewCharacter) return;

	float MouseX = 0.f;
	float MouseY = 0.f;

	if (!GetMousePosition(MouseX, MouseY)) return;

	//이전 Tick 마우스 위치와 현재 마우스 위치의 X값 차이를 확인
	float ChangeX = MouseX - LastMouseX;
	LastMouseX = MouseX;

	if (!FMath::IsNearlyZero(ChangeX)) {
		PreviewCharacter->AddPreviewYaw(ChangeX * PreviewRotateSpeed);
	}
}

//마우스 우클릭 시작
void ATitle_PlayerController::OnLeftMousePressed()
{
	bIsPreviewDragging = true;
	
	float MouseX = 0.f;
	float MouseY = 0.f;
	if (GetMousePosition(MouseX, MouseY)) {
		LastMouseX = MouseX;
	}

	if (PreviewCharacter) {
		PreviewCharacter->StopTurnToFront();
	}
}

//마우스 우클릭 완료
void ATitle_PlayerController::OnLeftMouseReleased()
{
	bIsPreviewDragging = false;

	if (PreviewCharacter) {
		PreviewCharacter->TurnToFront();
	}
}

//플레이 버튼 입력 시 기능
void ATitle_PlayerController::HandlePlayRequested(FString NickName)
{
	FString SanitizedNickname;
	FText ErrorText;

	//닉네임이 유효하지 않다면 Error Text
	if (!ValidateNickname(NickName, SanitizedNickname, ErrorText)) {
		if (TitleWidgetInstance) {
			TitleWidgetInstance->SetStatusMessage(ErrorText);
			TitleWidgetInstance->SetMatchingMode(false);
			TitleWidgetInstance->SetNicknameLocked(false);
		}
		return;
	}

	//게임 내에서 사용되는 GameInstance를 캐스팅
	UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	if (!GameInstance) {
		if (TitleWidgetInstance) {
			TitleWidgetInstance->SetStatusMessage(FText::FromString("Can't Found GameInstance"));
			TitleWidgetInstance->SetMatchingMode(false);
			TitleWidgetInstance->SetNicknameLocked(false);
		}
		return;
	}

	//GameInstance에 입력받은 닉네임 저장
	GameInstance->SetLocalPlayerNickname(SanitizedNickname);
	if (TitleWidgetInstance) {
		TitleWidgetInstance->SetNicknameText(SanitizedNickname);
		TitleWidgetInstance->SetNicknameLocked(true);
		TitleWidgetInstance->SetMatchingMode(true);
		TitleWidgetInstance->SetStatusMessage(FText::FromString(TEXT("Matching...")));
	}
	//현재 게임 흐름 상태를 Searching으로 변경
	GameInstance->SetMatchFlowState(EMatchFlowState::Searching);

	//GameInstance의 SessionSubsystem으로 QuickMatch 시작
	UAllPlayMode_SessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAllPlayMode_SessionSubsystem>();
	if (!SessionSubsystem) {
		if (TitleWidgetInstance) {
			TitleWidgetInstance->SetStatusMessage(FText::FromString("SessionSubsystem not found"));
			TitleWidgetInstance->SetMatchingMode(false);
			TitleWidgetInstance->SetNicknameLocked(false);
		}
		return;
	}
	SessionSubsystem->QuickMatchLAN();
}

void ATitle_PlayerController::HandleCancelRequested()
{
	UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	if (!GameInstance) return;

	UAllPlayMode_SessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAllPlayMode_SessionSubsystem>();
	if (!SessionSubsystem) return;

	SessionSubsystem->CancelQuickMatchLAN();
}

//세션 상태 변경을 확인 (Title 화면에서 매칭 상태를 플레이어에게 보여줌)
void ATitle_PlayerController::HandleSessionStateChanged(ESessionUIState State, const FString& Message)
{
	if (!TitleWidgetInstance) return;

	UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance());

	switch (State) {
	case ESessionUIState::Searching:
	case ESessionUIState::Hosting:
		if (GameInstance) {
			GameInstance->SetMatchFlowState(EMatchFlowState::Searching);
		}
		TitleWidgetInstance->SetMatchingMode(true);
		TitleWidgetInstance->SetNicknameLocked(true);
		TitleWidgetInstance->SetStatusMessageShowing(FText::FromString(TEXT("Matching...")));
		break;
	case ESessionUIState::Joining:
		if (GameInstance) {
			GameInstance->SetMatchFlowState(EMatchFlowState::Searching);
		}
		TitleWidgetInstance->SetJoinCompleteMode();
		TitleWidgetInstance->SetStatusMessageShowing(FText::FromString(TEXT("Matching Complete!")));
		break;
	case ESessionUIState::Matched:
		if (GameInstance) {
			GameInstance->SetMatchFlowState(EMatchFlowState::None);
		}
		TitleWidgetInstance->SetJoinCompleteMode();
		TitleWidgetInstance->SetStatusMessageShowing(FText::FromString(TEXT("Matching Complete!")));
		break;

	case ESessionUIState::None:
		if (GameInstance) {
			GameInstance->SetMatchFlowState(EMatchFlowState::None);
		}
		TitleWidgetInstance->SetMatchingMode(false);
		TitleWidgetInstance->SetNicknameLocked(false);
		if (Message.Equals(TEXT("Matching Cancelled!"))) {
			TitleWidgetInstance->SetStatusMessageFadeOut(FText::FromString(Message), 1.f);
		}
		else {
			TitleWidgetInstance->SetStatusMessage(FText::FromString(Message));
		}
		break;

	case ESessionUIState::Failed:
	default:
		if (GameInstance) {
			GameInstance->SetMatchFlowState(EMatchFlowState::None);
		}
		TitleWidgetInstance->SetMatchingMode(false);
		TitleWidgetInstance->SetNicknameLocked(false);
		TitleWidgetInstance->SetStatusMessage(FText::FromString(Message));
		break;
	}
}


//닉네임이 유효한지 검사
bool ATitle_PlayerController::ValidateNickname(const FString& nickname, FString& OutSanitized, FText& OutErrorText)
{
	OutSanitized = nickname;
	OutSanitized.TrimStartAndEndInline();

	if (OutSanitized.IsEmpty()) {
		OutErrorText = FText::FromString(TEXT("Press your Nickname."));
		return false;
	}

	if (OutSanitized.Len() > 12 || OutSanitized.Len() < 2)
	{
		OutErrorText = FText::FromString(TEXT("Nickname must be 2 ~ 12 words."));
		return false;
	}

	return true;
}

