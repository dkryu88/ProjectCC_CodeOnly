// Fill out your copyright notice in the Description page of Project Settings.


#include "Ready/Ready_PlayerController.h"
#include "Ready/Ready_LoadingWidget.h"
#include "AllPlayMode_GameInstance.h"
#include "Player_State.h"
#include "Ready/PlayMode_Ready.h"

void AReady_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;

	if (Ready_LoadingWidgetClass && !Ready_LoadingWidget) {
		Ready_LoadingWidget = CreateWidget<UReady_LoadingWidget>(this, Ready_LoadingWidgetClass);
		if (Ready_LoadingWidget) {
			Ready_LoadingWidget->AddToViewport(100);
		}
	}

	UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	if (!GameInstance) return;

	if (!bDataSubmitted) {
		Server_SubmitData(GameInstance->GetPlayerLocalNickname(), GameInstance->GetLocalPortraitId());
		bDataSubmitted = true;
	}
	
	if (!bReadyScreenLoadedSent) {
		Server_NotifyReadyScreenLoaded();
		bReadyScreenLoadedSent = true;
	}

	//준비가 완료되었는지 0.1초 단위로 계속 확인
	GetWorld()->GetTimerManager().SetTimer(TravelCheckTimerHandle, this, &AReady_PlayerController::TryNotifyReadyToTravel, 0.1f, true);
}
void AReady_PlayerController::TryNotifyReadyToTravel()
{
	if (bReadyToTravelSent) return;

	APlayer_State* PS = GetPlayerState<APlayer_State>();
	if (!PS) return;

	bool bProfileSynced = PS->GetReadySyncState() >= EReadySyncState::ProfileSynced;

	if (!bProfileSynced) return;

	Server_NotifyReadyToTravel();
	bReadyToTravelSent = true;

	//준비 완료가 확인되면 타이머 종료
	GetWorld()->GetTimerManager().ClearTimer(TravelCheckTimerHandle);
}

//<ServerRPC> PlayerState에 닉네임, 초상화 ID 저장
void AReady_PlayerController::Server_SubmitData_Implementation(const FString& nickname, int32 portraitId)
{
	APlayer_State* Player_State = GetPlayerState<APlayer_State>();
	if (!Player_State) return;

	Player_State->SetNickName(nickname);
	Player_State->SetPortraitId(portraitId);

	Player_State->SetReadySyncState(EReadySyncState::ProfileSynced);
}

//<ServerRPC> 플레이어가 화면 이동 준비가 완료되었음을 알림
void AReady_PlayerController::Server_NotifyReadyToTravel_Implementation()
{
	if (APlayMode_Ready* ReadyMode = GetWorld()->GetAuthGameMode<APlayMode_Ready>()) {
		ReadyMode->NotifyReadyToTravel(this);
	}
}
//<ServerRPC> Ready 화면 Widget이 준비되었다고 서버에 알림
void AReady_PlayerController::Server_NotifyReadyScreenLoaded_Implementation()
{
	if (APlayMode_Ready* ReadyMode = GetWorld()->GetAuthGameMode<APlayMode_Ready>()) {
		ReadyMode->NotifyReadyScreenLoaded(this);
	}
}
