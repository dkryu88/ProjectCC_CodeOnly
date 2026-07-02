// Fill out your copyright notice in the Description page of Project Settings.


#include "Ready/PlayMode_Ready.h"
#include "Ready/Ready_GameState.h"
#include "Ready/Ready_PlayerController.h"
#include "Player_State.h"
#include "AllPlayMode_SessionSubsystem.h"
#include "AllPlayMode_GameInstance.h"
#include "Kismet/GameplayStatics.h"

APlayMode_Ready::APlayMode_Ready()
{
	GameStateClass = AReady_GameState::StaticClass();
	PlayerStateClass = APlayer_State::StaticClass();
	PlayerControllerClass = AReady_PlayerController::StaticClass();

	bUseSeamlessTravel = true;
	 
	AvailableMatchLevels = { TEXT("LV_Match_Volcano") };
}

void APlayMode_Ready::BeginPlay()
{
	if (UAllPlayMode_GameInstance* GI = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		MaxPlayers = GI->GetMaxPlayersByMode();
	}

	Super::BeginPlay();
	ChooseRandomMatchLevel();
	UpdateConnectedPlayers();
	AssignPortraitId();
}

void APlayMode_Ready::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!HasAuthority()) return;
	if (!ErrorMessage.IsEmpty()) return;

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (OSS) {
		IOnlineSessionPtr SessionInterface = OSS->GetSessionInterface();
		if (SessionInterface.IsValid()) {
			FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
			if (Session && Session->SessionSettings.bAllowJoinInProgress == false) {
				ErrorMessage = TEXT("Ready : Game is already running!");
				return;
			}
		}
	}

	if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		if (GetNumPlayers() >= GameInstance->GetMaxPlayersByMode()) {
			ErrorMessage = TEXT("Ready : Room is Full!");
			return;
		}
	}
}

void APlayMode_Ready::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UpdateConnectedPlayers();
	AssignPortraitId();
}

//플레이할 맵을 랜덤으로 선정
void APlayMode_Ready::ChooseRandomMatchLevel()
{
	AReady_GameState* ReadyGameState = GetGameState<AReady_GameState>();
	if (!ReadyGameState || AvailableMatchLevels.Num() <= 0) return;

	int32 Index = FMath::RandRange(0, AvailableMatchLevels.Num() - 1);
	ReadyGameState->SelectedMapLevelName = AvailableMatchLevels[Index];

	FString RawName = ReadyGameState->SelectedMapLevelName.ToString();
	RawName.RemoveFromStart(TEXT("LV_Match_"));
	ReadyGameState->SelectedMapDisplayName = RawName;
}
//현재 접속 인원 수를 GameState에 반영
void APlayMode_Ready::UpdateConnectedPlayers() {
	AReady_GameState* ReadyGameState = GetGameState<AReady_GameState>();
	if (!ReadyGameState || !GameState) return;

	ReadyGameState->ConnectedPlayers = GameState->PlayerArray.Num();
}
//게임 시작 판정 (모든 플레이어가 게임 시작 준비가 되었는지 확인)
void APlayMode_Ready::CheckAutoStartMatch()
{
	if (!HasAuthority()) return;

	AReady_GameState* ReadyGameState = GetGameState<AReady_GameState>();
	if (!ReadyGameState || !GameState) return;
	if (GameState->PlayerArray.Num() < MaxPlayers) return;
	for (APlayerState* PSBase : GameState->PlayerArray) {
		APlayer_State* PS = Cast<APlayer_State>(PSBase);
		if (!PS || PS->GetReadySyncState() != EReadySyncState::ReadyToTravel) return;
	}

	if (ReadyGameState->bAllPlayersReadyToTravel == true) return;
	ReadyGameState->SetAllPlayersReadyToTravel(true);

	int32 TotalPlayers = GameState->PlayerArray.Num();

	if (UAllPlayMode_SessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UAllPlayMode_SessionSubsystem>()) {
		SessionSubsystem->SetTotalMatchPlayers(TotalPlayers);
		UE_LOG(LogTemp, Warning, TEXT("[ReadyGameMode] Set ExpectedMatchPlayers = %d"), TotalPlayers);
	}

	if (GetWorldTimerManager().IsTimerActive(MatchStartTimerHandle)) return;

	float ServerNow = ReadyGameState->GetServerWorldTimeSeconds();
	ReadyGameState->SetMatchStartServerTime(ServerNow + ReadyGameState->StartCountDonwDuration);

	//모든 플레이어의 화면을 LV_Match_선택된 맵 으로 이동
	GetWorldTimerManager().SetTimer(MatchStartTimerHandle, this, &APlayMode_Ready::StartMatchTravel, ReadyGameState->StartCountDonwDuration, false);
}

void APlayMode_Ready::StartMatchTravel()
{
	if (!HasAuthority()) return;

	AReady_GameState* ReadyGameState = GetGameState<AReady_GameState>();
	if (!ReadyGameState) return;

	if (ReadyGameState->SelectedMapPath.IsEmpty()) return;

	GetWorld()->ServerTravel(ReadyGameState->SelectedMapPath + ReadyGameState->SelectedMapLevelName.ToString() + TEXT("?listen"));
}

void APlayMode_Ready::AssignPortraitId()
{
	if (!GameState) return;

	for (int32 i = 0; i < GameState->PlayerArray.Num(); ++i) {
		APlayer_State* PS = Cast<APlayer_State>(GameState->PlayerArray[i]);
		if (!PS) continue;
		PS->SetPortraitId(i);
		PS->ForceNetUpdate();
	}
}


//PlayerState에 닉네임, 초상화 ID 저장
void APlayMode_Ready::NotifyPlayerProfileSynced(APlayerController* PC)
{
	if (APlayer_State* PS = PC ? PC->GetPlayerState<APlayer_State>() : nullptr) {
		PS->SetReadySyncState(EReadySyncState::ProfileSynced);
	}
}
//Ready 화면 Widget이 준비되었다고 서버에 알림
void APlayMode_Ready::NotifyReadyScreenLoaded(APlayerController* PC)
{
	if (APlayer_State* PS = PC ? PC->GetPlayerState<APlayer_State>() : nullptr) {
		PS->SetReadySyncState(EReadySyncState::ReadyScreenLoaded);
	}
}
//플레이어가 화면 이동 준비가 완료되었음을 알림
void APlayMode_Ready::NotifyReadyToTravel(APlayerController* PC) {
	if (APlayer_State* PS = PC ? PC->GetPlayerState<APlayer_State>() : nullptr) {
		PS->SetReadySyncState(EReadySyncState::ReadyToTravel);
	}

	CheckAutoStartMatch();
}