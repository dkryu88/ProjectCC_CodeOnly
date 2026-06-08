// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/PlayMode_Title.h"
#include "Title/Title_PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "OnlineSubsystemTypes.h"
#include "AllPlayMode_GameInstance.h"
#include "AllPlayMode_SessionSubsystem.h"

APlayMode_Title::APlayMode_Title()
{
	PlayerControllerClass = ATitle_PlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;

	bUseSeamlessTravel = true;
}

void APlayMode_Title::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	if (!GameInstance) return;

	if (GameInstance->bPendingCreateLANSession) {
		GameInstance->bPendingCreateLANSession = false;
		if (UAllPlayMode_SessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UAllPlayMode_SessionSubsystem>()) {
			SessionSubsystem->CreateLANSessionInternal();
		}
	}
}

void APlayMode_Title::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!HasAuthority()) return;
	if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		if (UAllPlayMode_SessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAllPlayMode_SessionSubsystem>()) {
			SessionSubsystem->NotifyHostPlayerJoining();
		}
	}

}

//새 플레이어가 세션에 접속하면 호출
void APlayMode_Title::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Warning, TEXT("[TitleGameMode] PostLogin called. NewPlayer=%s"), *GetNameSafe(NewPlayer));

	if (GameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TitleGameMode] PlayerArray Num = %d"), GameState->PlayerArray.Num());
	}

	CheckAutoStartReadyLevel();
}

void APlayMode_Title::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

//매칭 상태 확인 후 LV_Ready로 이동
void APlayMode_Title::CheckAutoStartReadyLevel()
{
	if (!GameState)
	{
		UE_LOG(LogTemp, Error, TEXT("[TitleGameMode] GameState is null"));
		return;
	}

	int32 CurrentPlayers = GameState->PlayerArray.Num();
	UE_LOG(LogTemp, Warning, TEXT("[TitleGameMode] CurrentPlayers = %d"), CurrentPlayers);


	if (CurrentPlayers >= MaxMatchPlayers) {
		//중복 이동 방지
		bTravelingToReady = true;

		if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
			if (UAllPlayMode_SessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAllPlayMode_SessionSubsystem>()) {
				SessionSubsystem->NotifyHostPlayerJoin();
			}
		}

		FString TravelURL = ReadyLevelPath + TEXT("?listen");
		UE_LOG(LogTemp, Warning, TEXT("[TitleGameMode] Travel to LV_Ready"));

		GetWorldTimerManager().SetTimerForNextTick([this, TravelURL]() {
			if (UWorld* World = GetWorld()) {
				World->ServerTravel(TravelURL);
			}
		});
	}

}
