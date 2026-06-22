// Fill out your copyright notice in the Description page of Project Settings.


#include "Result/Result_PlayerController.h"
#include "Result/Player_ResultWidget.h"
#include "AllPlayMode_GameInstance.h"
#include "TimerManager.h"
#include "Engine/World.h"
//[사운드] 추가
#include "Effect/AudioManagerSubsystem.h"
#include "Effect/GameAudioDataAsset.h"
//[추가]세션종료
#include "AllPlayMode_SessionSubsystem.h"

void AResult_PlayerController::BeginPlay() {
	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	SetInputMode(InputMode);

	if (Player_ResultWidget) {
		ResultWidget = CreateWidget<UPlayer_ResultWidget>(this, Player_ResultWidget);
		if (ResultWidget) {
			ResultWidget->AddToViewport(100);

			if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
				ResultWidget->InitWidget(GameInstance->GetMatchResult(), GameInstance->GetMatchResults());
			}
		}
	}

	// [사운드] Ready BGM 재생
	if (IsLocalController()) {
		if (UAudioManagerSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAudioManagerSubsystem>()) {
			if (AudioSub->GetAudioData() && AudioSub->GetAudioData()->ResultBGM) {
				AudioSub->PlayBGM(AudioSub->GetAudioData()->ResultBGM, 0.5f);
			}
		}
	}

	GetWorldTimerManager().SetTimer(EnableExitTimerHandle, this, &AResult_PlayerController::EnableEixtToTitle, 3.f, false);
}

void AResult_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent) {
		InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AResult_PlayerController::OnPressedExitToTitle);
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AResult_PlayerController::OnPressedExitToTitle);
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AResult_PlayerController::OnPressedExitToTitle);
	}
}

void AResult_PlayerController::EnableEixtToTitle()
{
	bCanExitMatch = true;
	
	if (ResultWidget) {
		ResultWidget->SetExitEnabled(true);
	}
}

void AResult_PlayerController::OnPressedExitToTitle()
{
	if (!bCanExitMatch) return;

	//[추가] 게임이 끝나고 결과창에서 타이틀로 돌아갈 때
	// 기존 세션을 종료시킴
	if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		if (UAllPlayMode_SessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAllPlayMode_SessionSubsystem>()) {
			SessionSubsystem->CancelQuickMatchLAN();
		}
	}

	ClientTravel(TEXT("/Game/Maps/LV_Title"), TRAVEL_Absolute);
}



