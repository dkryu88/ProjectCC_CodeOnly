// Fill out your copyright notice in the Description page of Project Settings.


#include "Match_PlayerController.h"
#include "AllPlayMode_GameInstance.h"
#include "Player_Character.h"
#include "Player_ControllerWidget.h"
#include "Player_State.h"
#include "PlayMode_Match.h"
#include "Match_ScoreBoardWidget.h"
#include "Shop/Match_ShopWidget.h"
#include "Match_State.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "TimerManager.h"
//[사운드] 추가
#include "Effect/AudioManagerSubsystem.h"
#include "Effect/GameAudioDataAsset.h"


void AMatch_PlayerController::BeginPlay() {
	Super::BeginPlay();

	if (!IsLocalController()) return;

	// [사운드]매치 BGM 재생
	//if (UAllPlayMode_GameInstance* GI = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
	//	if (GI->AudioData) {
	//		FName CurrentMapName = FName(*GetWorld()->GetName());

	//		if (USoundBase** FoundBGM = GI->AudioData->MatchBGM_Map.Find(CurrentMapName)) {
	//			if (UAudioManagerSubsystem* AudioSub = GI->GetSubsystem<UAudioManagerSubsystem>()) {
	//				AudioSub->PlayBGM(*FoundBGM, 1.f);
	//			}
	//		}
	//	}
	//}
	if (UAudioManagerSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAudioManagerSubsystem>()) {
		FString MatchMapName = UGameplayStatics::GetCurrentLevelName(this, true);
		AudioSub->PlayBGMByMapName(FName(*MatchMapName), 1.f);
	}


	GetWorldTimerManager().SetTimer(SetupRetryTimerHandle, this, &AMatch_PlayerController::TryFinishLocalSetup, SetupRetryInterval, true);

}

void AMatch_PlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	if (!IsLocalController()) return;

	//[추가]매치 종료시 크래쉬 방지
	if (InPawn == nullptr) return;

	TryFinishLocalSetup();

	//ScreenWidget이 없으면 새로 생성
	if (Player_ControllerWidget && !ScreenWidget && GetLocalPlayer()) {	//&& GetLocalPlayer() 추가, 에디터플레이 중 강제종료시 에러창 발생 방지
		ScreenWidget = CreateWidget<UPlayer_ControllerWidget>(this, Player_ControllerWidget);
		if (ScreenWidget) {
			ScreenWidget->AddToViewport(0);
		}
	}
	//ScreenWidget이 있으면 현재 소유중인 Player를 기준으로 초기화
	if (ScreenWidget) {
		APlayer_Character* player = Cast<APlayer_Character>(InPawn);
		if (player) {
			ScreenWidget->InitWidget(player);
			//Shop 버튼 바인딩
			BindShopButton();
		}
	}

	if (bWaitingRespawn) {
		ScreenWidget->SetUIState(EPlayerUIState::RespawnWaiting);
		if (bCanRespawnNow) {
			ScreenWidget->ShowCanRespawnText();
		}
	}
	else {
		AMatch_State* MS = GetWorld() ? GetWorld()->GetGameState<AMatch_State>() : nullptr;

		if (MS && MS->IsMatchStarted()) {
			ScreenWidget->SetUIState(EPlayerUIState::Playing);
		}
		else {
			ScreenWidget->SetUIState(EPlayerUIState::StartWaiting);
		}
	}

	UpdateShopButtonVisibility();

	if (InPawn) {
		ApplyGameInputMode();
	}

	if (!bWaitingRespawn) {
		SetPlayWidget();
	}
}

void AMatch_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent) {
		InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AMatch_PlayerController::OnPressedSpaceKey);
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AMatch_PlayerController::OpenScoreBoard);
		InputComponent->BindKey(EKeys::Tab, IE_Released, this, &AMatch_PlayerController::CloseScoreBoard);
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMatch_PlayerController::CloseShop);
	}
}

void AMatch_PlayerController::UpdateShopButtonVisibility()
{
	if (!ScreenWidget) return;

	bool bShowShopButton = bWaitingRespawn && !bAlreadyPurchasedInShop;
	ScreenWidget->SetShopButtonVisible(bShowShopButton);
}

void AMatch_PlayerController::SaveResultData(const FMatchResultData& OwnerResult, const TArray<FMatchResultData>& AllResults)
{
	if (!IsLocalController()) return;

	if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		GameInstance->SetMatchResult(OwnerResult);
		GameInstance->SetMatchResults(AllResults);
	}
}

void AMatch_PlayerController::EndMatch()
{
	if (!IsLocalController()) return;

	ClientTravel(TEXT("/Game/Levels/LV_Result"), TRAVEL_Absolute);
}

void AMatch_PlayerController::TryFinishLocalSetup()
{
	if (!IsLocalController()) return;

	UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	APlayer_State* Player_State = GetPlayerState<APlayer_State>();

	if (!GameInstance || !Player_State) return;

	ApplyLocalInputMapping();

	if (GetPawn()) {
		ApplyGameInputMode();
	}

	if (Player_State->GetNickName().IsEmpty()) {
		FString LocalNickname = GameInstance->GetPlayerLocalNickname();
		int32 LocalPortraitId = GameInstance->GetLocalPortraitId();

		if (!LocalNickname.IsEmpty()) {
			Server_SubmitMatchData(LocalNickname, LocalPortraitId);
		}
	}

	if (!bReportedMatchLoaded && !Player_State->IsMatchLevelLoaded()) {
		Server_ReportMatchLoaded();
		bReportedMatchLoaded = true;
	}

	if (bInputMappingApplied && !Player_State->GetNickName().IsEmpty() && Player_State->IsMatchLevelLoaded()) {
		GetWorldTimerManager().ClearTimer(SetupRetryTimerHandle);
	}
}

void AMatch_PlayerController::ApplyLocalInputMapping()
{
	if (bInputMappingApplied) return;
	if (!MatchInputMappingContext) return;

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer()) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)) {
			Subsystem->AddMappingContext(MatchInputMappingContext, 0);
			bInputMappingApplied = true;
		}
	}
}

void AMatch_PlayerController::ApplyGameInputMode()
{
	if (!IsLocalController()) return;

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	SetIgnoreLookInput(false);
	SetIgnoreMoveInput(false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	if (UGameViewportClient* GameViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr) {
		GameViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
		GameViewportClient->SetMouseLockMode(EMouseLockMode::LockAlways);
	}
}

void AMatch_PlayerController::ApplyUIInputMode()
{
	if (!IsLocalController()) return;

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	if (UGameViewportClient* GameViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr) {
		GameViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
		GameViewportClient->SetMouseLockMode(EMouseLockMode::LockAlways);
	}
}

void AMatch_PlayerController::OnPressedSpaceKey()
{
	if (!IsLocalController()) return;
	if (!bWaitingRespawn) return;
	if (!bCanRespawnNow) return;

	//리스폰 요청 시 즉시 상점 닫기
	if (bShopOpen) {
		CloseShop();
	}

	Server_RequestRespawn();
}

//Out 상태 UI 전환
void AMatch_PlayerController::SetOutWidget()
{
	if (!IsLocalPlayerController()) return;
	if (!GetWorld()) return;

	TArray<AActor*> AllPlayers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayer_Character::StaticClass(), AllPlayers);

	for (AActor* Actor : AllPlayers) {
		APlayer_Character* player = Cast<APlayer_Character>(Actor);
		if (!player) continue;

		player->SetPlayerWidgetVisibility(false);
	}
}
//플레이 상태 UI 전환
void AMatch_PlayerController::SetPlayWidget() {
	if (!IsLocalPlayerController()) return;
	if (!GetWorld()) return;

	TArray<AActor*> AllPlayers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayer_Character::StaticClass(), AllPlayers);

	for (AActor* Actor : AllPlayers) {
		APlayer_Character* player = Cast<APlayer_Character>(Actor);
		if (!player) continue;

		player->SetPlayerWidgetVisibility(true);
	}
}

void AMatch_PlayerController::OpenScoreBoard()
{
	if (!IsLocalController()) return;
	//상점이 열려있는 상태에서는 스코어보드 열람 불가
	if (bShopOpen) return;

	if (!ScoreWidget && Match_ScoreBoardWidget) {
		ScoreWidget = CreateWidget<UMatch_ScoreBoardWidget>(this, Match_ScoreBoardWidget);
		if (ScoreWidget) {
			ScoreWidget->AddToViewport(150);
			ScoreWidget->InitWidget(this);
		}
	}

	if (ScoreWidget) {
		ScoreWidget->UpdateScoreBoard();
		ScoreWidget->StartAutoUpdate();
		ScoreWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void AMatch_PlayerController::CloseScoreBoard()
{
	if (!IsLocalController()) return;

	if (ScoreWidget) {
		ScoreWidget->EndAutoUpdate();
		ScoreWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AMatch_PlayerController::BindShopButton()
{
	if (!IsLocalController()) return;
	if (!ScreenWidget) return;

	if (UButton* ShopButton = ScreenWidget->GetShopButton()) {
		ShopButton->OnClicked.RemoveAll(this);
		ShopButton->OnClicked.AddDynamic(this, &AMatch_PlayerController::OpenShop);
	}
}

void AMatch_PlayerController::OpenShop()
{
	if (!IsLocalController()) return;
	if (!IsWaitingRespawn()) return;
	if (bAlreadyPurchasedInShop) return;

	if (!ShopWidget && Match_ShopWidget) {
		ShopWidget = CreateWidget<UMatch_ShopWidget>(this, Match_ShopWidget);
		if (ShopWidget) {
			ShopWidget->AddToViewport(100);
			ShopWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (!ShopWidget) return;

	bool bVisible = ShopWidget->GetVisibility() == ESlateVisibility::Visible || ShopWidget->GetVisibility() == ESlateVisibility::HitTestInvisible;

	if (bVisible) {
		CloseShop();
		return;
	}
	//상점 열기 전에 스코어보드 닫기
	CloseScoreBoard();

	ShopWidget->SetVisibility(ESlateVisibility::Visible);
	bShopOpen = true;

	//Shop 버튼이 포커스를 먹지 않도록 Viewport에 포커스 이동
	UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

void AMatch_PlayerController::CloseShop()
{
	if (!IsLocalController()) return;
	if (!ShopWidget) return;

	ShopWidget->SetVisibility(ESlateVisibility::Collapsed);
	bShopOpen = false;

	UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

/*--------------RPC 모음 -------------------*/
void AMatch_PlayerController::Server_SubmitMatchData_Implementation(const FString& nickname, int32 portraitId)
{
	APlayer_State* Player_State = GetPlayerState<APlayer_State>();
	if (!Player_State) return;

	Player_State->SetNickName(nickname);
	Player_State->SetPortraitId(portraitId);
}

void AMatch_PlayerController::Server_ReportMatchLoaded_Implementation()
{
	APlayer_State* Player_State = GetPlayerState<APlayer_State>();
	if (!Player_State) return;

	Player_State->SetMatchLevelLoaded(true);

	//매치 시작 시 딜레이 설정
	if (APlayMode_Match* MatchMode = GetWorld()->GetAuthGameMode<APlayMode_Match>()) {
		MatchMode->CheckAllPlayersLoadedAndStartDelay();
	}
}

//서버에 수동 리스폰 요청
void AMatch_PlayerController::Server_RequestRespawn_Implementation()
{
	APlayMode_Match* Match = GetWorld()->GetAuthGameMode<APlayMode_Match>();
	if (Match) {
		Match->TryRespawn(this);
	}
}

//서버에 구매 요청 Server RPC
void AMatch_PlayerController::Server_Purchase_Implementation(EShopBoxs Box)
{
	APlayMode_Match* MatchMode = GetWorld() ? GetWorld()->GetAuthGameMode<APlayMode_Match>() : nullptr;
	if (!MatchMode) {
		Client_ShopPurchaseResult(false);
		return;
	}

	bool bSuccess = MatchMode->TryShoppingBox(this, Box);
	Client_ShopPurchaseResult(bSuccess);
}

//관전 시작 Client RPC
void AMatch_PlayerController::Client_StartSpectating_Implementation(AActor* SpectatorTarget)
{
	if (!IsLocalController()) return;

	bAutoManageActiveCameraTarget = false;

	if (SpectatorTarget) {
		SetViewTargetWithBlend(SpectatorTarget, 0.f);
	}

	SetOutWidget();
}
//UI 입력 모드 Client RPC
void AMatch_PlayerController::Client_ApplyUIInputMode_Implementation()
{
	ApplyUIInputMode();
}

//Game 입력 모드 Client RPC
void AMatch_PlayerController::Client_ApplyGameInputMode_Implementation()
{
	ApplyGameInputMode();
}

//현재 리스폰 데이터를 세팅 Client RPC
void AMatch_PlayerController::Client_SetRespawnState_Implementation(bool bWaiting, bool bCanRespawn)
{
	bWaitingRespawn = bWaiting;
	bCanRespawnNow = bCanRespawn;

	if (!bWaitingRespawn) {
		CloseShop();
	}
	else {
		bAlreadyPurchasedInShop = false;
	}

	if (!ScreenWidget) return;

	if (bWaitingRespawn) {
		ScreenWidget->SetUIState(EPlayerUIState::RespawnWaiting);

		if (bCanRespawnNow) ScreenWidget->ShowCanRespawnText();

		Client_ApplyUIInputMode();
	}
	else {
		AMatch_State* MS = GetWorld() ? GetWorld()->GetGameState<AMatch_State>() : nullptr;
		if (MS && MS->IsMatchStarted()) {
			ScreenWidget->SetUIState(EPlayerUIState::Playing);
			Client_ApplyGameInputMode();
		}
		else {
			ScreenWidget->SetUIState(EPlayerUIState::StartWaiting);
			Client_ApplyGameInputMode();
		}
	}

	UpdateShopButtonVisibility();
}

void AMatch_PlayerController::Client_StartSpectatingPlayer_Implementation(APlayer_Character* SpectatorTarget)
{
	if (!IsLocalController()) return;

	bAutoManageActiveCameraTarget = false;

	if (SpectatorTarget) {
		SetViewTargetWithBlend(SpectatorTarget, 0.f);
	}

	SetOutWidget();
}

void AMatch_PlayerController::Client_StartSpectatingDefaultCamera_Implementation()
{
	if (!IsLocalController()) return;

	bAutoManageActiveCameraTarget = false;

	TArray<AActor*> DefaultCameras;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Default Camera"), DefaultCameras);

	if (DefaultCameras.Num() > 0 && DefaultCameras[0]) {
		SetViewTargetWithBlend(DefaultCameras[0], 0.f);
		SetOutWidget();
		return;
	}

	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), FoundCameras);

	if (FoundCameras.Num() > 0 && FoundCameras[0]) {
		SetViewTargetWithBlend(FoundCameras[0], 0.f);
		SetOutWidget();
		return;
	}
	//없으면 큰일
	UE_LOG(LogTemp, Error, TEXT("[Spectate][Client] No Default Camera found in local world"));
}

void AMatch_PlayerController::Client_SetPreMatchDelay_Implementation()
{
	if (ScreenWidget) {
		ScreenWidget->SetUIState(EPlayerUIState::StartWaiting);
		UpdateShopButtonVisibility();
	}
}

void AMatch_PlayerController::Client_StartCountDown_Implementation()
{
	if (ScreenWidget) {
		ScreenWidget->SetUIState(EPlayerUIState::Countdown);
		ScreenWidget->SetCountdown(6);
	}
}

void AMatch_PlayerController::Client_UpdateCountDown_Implementation(int32 number)
{
	if (ScreenWidget) {
		ScreenWidget->SetUIState(EPlayerUIState::Countdown);
		ScreenWidget->SetCountdown(number);
	}
	// [사운드] 카운트다운 효과음 재생
	if (number == 6) {
		if (UAudioManagerSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAudioManagerSubsystem>()) {
			if (AudioSub->GetAudioData() && AudioSub->GetAudioData()->CountDownSound) {
				AudioSub->StartDucking(0.3f, 0.5f);	//BGM볼륨 30%로 조절
				AudioSub->PlayOneShotSFX(AudioSub->GetAudioData()->CountDownSound);
			}
		}
	}
	if (number == 1) {	//Start 나올 때
		if (UAudioManagerSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAudioManagerSubsystem>()) {
			AudioSub->StopDucking(0.5f);	//BGM볼륨 원복
		}
	}
}

void AMatch_PlayerController::Client_StartPlayingUI_Implementation()
{
	if (ScreenWidget)
	{
		ScreenWidget->SetUIState(EPlayerUIState::Playing);
		UpdateShopButtonVisibility();
	}
}

void AMatch_PlayerController::Client_SaveResultData_Implementation(const FMatchResultData& OwnerResult, const TArray<FMatchResultData>& AllResults)
{
	SaveResultData(OwnerResult, AllResults);
}

void AMatch_PlayerController::Client_EndMatch_Implementation()
{
	EndMatch();
}

//구매 성공 알림 Client RPC
void AMatch_PlayerController::Client_ShopPurchaseResult_Implementation(bool bSuccess)
{
	if (!IsLocalController()) return;
	if (!bSuccess) return;

	//구매 완료 상태로 변경
	bAlreadyPurchasedInShop = true;
	//상점 닫기
	CloseShop();
	//상점 버튼 숨김처리
	UpdateShopButtonVisibility();
}

void AMatch_PlayerController::Client_UpdateMatchEventCountdown_Implementation(FName EventName, int32 SecondsUntilEvent)
{
	if (ScreenWidget) {
		ScreenWidget->ShowMatchEventCountdown(EventName, SecondsUntilEvent);
	}
	// [사운드] 이벤트 시작 5초 전부터 경고음SFX 재생
	if (SecondsUntilEvent == 5) {
		if (UAudioManagerSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAudioManagerSubsystem>()) {
			AudioSub->StartDucking(1.5f, 0.3f);	//1.5초동안 30%로 볼륨 조절
			if (AudioSub->GetAudioData() && AudioSub->GetAudioData()->EventWarningSound) {
				AudioSub->PlayManagedSFX(AudioSub->GetAudioData()->EventWarningSound, 1.f);
			}

		}
	}
}

void AMatch_PlayerController::Client_ShowMatchEventActive_Implementation(FName EventName, int32 RemainSeconds)
{
	if (ScreenWidget) {
		ScreenWidget->ShowMatchEventActive(EventName, RemainSeconds);
	}
	// [사운드] 이벤트 시작시
	if (UAudioManagerSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAudioManagerSubsystem>()) {
		if (AudioSub->GetAudioData() && AudioSub->GetAudioData()->EventWarningSound) {
			AudioSub->StopManagedSFX(AudioSub->GetAudioData()->EventWarningSound);	// 경고음 정지
		}
		// BGM볼륨 원복
		AudioSub->StopDucking(1.5f);
		// BGM재생속도 증가
		AudioSub->SetBGMPitch(1.5f);
	}
}

void AMatch_PlayerController::Client_HideMatchEventUI_Implementation() {
	if (ScreenWidget) {
		ScreenWidget->HideMatchEventUI();
	}
	if (UAudioManagerSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAudioManagerSubsystem>()) {
		// BGM재생속도 원복
		AudioSub->SetBGMPitch(1.f);
	}
}

// [사운드] 매치 종료 시 bgm 종료 함수
void AMatch_PlayerController::Client_FadeOutBgm_Implementation()	//헤더에 언선 후 정의 한 함수 -> PlayMode_Match.Cpp에서 EndMatchLogic함수 안에 사용됨
{
	// 서브시스템을 불러와 3초(3.0f) 동안 페이드아웃 후 자동 종료되게 명령
	if (UAudioManagerSubsystem* AudioSub = GetGameInstance()->GetSubsystem<UAudioManagerSubsystem>()) {
		AudioSub->StopBGM(3.0f);
	}
}
