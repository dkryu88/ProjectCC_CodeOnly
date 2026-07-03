// Fill out your copyright notice in the Description page of Project Settings.


#include "AllPlayMode_SessionSubsystem.h"
#include "AllPlayMode_GameInstance.h"
#include "OnlineSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"
#include "Title/Title_PlayerController.h"

UAllPlayMode_SessionSubsystem::UAllPlayMode_SessionSubsystem()
{
	//각 상황 별 Session Delegate 바인딩
	CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAllPlayMode_SessionSubsystem::OnCreateSessionCompleted);
	FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAllPlayMode_SessionSubsystem::OnFindSessionsCompleted);
	JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UAllPlayMode_SessionSubsystem::OnJoinSessionCompleted);
	DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UAllPlayMode_SessionSubsystem::OnDestroySessionCompleted);

}
/*
* IOnlineSubsystem -> 온라인 관련 전체를 담당
* SessionInterface -> 세션 검색/생성/참여/파괴를 담당
*/


void UAllPlayMode_SessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEngine) GEngine->OnNetworkFailure().AddUObject(this, &UAllPlayMode_SessionSubsystem::HandleNetworkFailure);
}

void UAllPlayMode_SessionSubsystem::Deinitialize()
{
	if (GEngine) GEngine->OnNetworkFailure().RemoveAll(this);

	Super::Deinitialize();
}

void UAllPlayMode_SessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) {
	if (bIsNetworkError) return;
	bIsNetworkError = true;

	bool bShouldAutoRematch = false;
	int32 BannedTicket = 0;
	//입장중이나 매치완료상태에서 연결이 끊겼다면 자동으로 리매치 상태로 돌입
	if (LastUIState == ESessionUIState::Matched || LastUIState == ESessionUIState::Joining) {
		bShouldAutoRematch = true;
		//연결이 끊긴 방의 티켓번호 저장 후 블랙리스트 등록
		if (LastTriedJoinResult.IsValid()) {
			LastTriedJoinResult->Session.SessionSettings.Get(FName(TEXT("HostTicket")), BannedTicket);
		}
	}

	CancelQuickMatchLAN();
	bCancelRequested = false;

	if (BannedTicket != 0) {
		IgnoredHostTickets.Add(BannedTicket);
	}

	LastUIState = ESessionUIState::None;
	LastUIMessage = TEXT("");

	if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		GameInstance->SetMatchFlowState(EMatchFlowState::None);

		if (bShouldAutoRematch) GameInstance->bAutoRestartMatch = true;
	}

	if (bShouldAutoRematch) {
		if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(SessionName)) {
			bSearchAfterDestroy = true;
		}
		else {
			if (World) World->GetTimerManager().SetTimerForNextTick([this]() {
				FindLANSessions();
			});
		}
	}

	if (World) {
		World->GetTimerManager().SetTimerForNextTick([this]() {
			bIsNetworkError = false;
			});
	}
	else bIsNetworkError = false;
}

//플레이어의 세션 인터페이스가 준비되었는지 확인
bool UAllPlayMode_SessionSubsystem::EnsureSessionInterface() {
	if (SessionInterface.IsValid()) return true;

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) {
		BroadcastState(ESessionUIState::Failed, TEXT("OnlineSubsystem is null"));
		return false;
	}

	SessionInterface = OSS->GetSessionInterface();
	if (!SessionInterface.IsValid()) {
		BroadcastState(ESessionUIState::Failed, TEXT("SessionInterface is Invalid"));
		return false;
	}

	return true;
}

//현재 세션 상태를 전파 (Title Widget에서 사용)
void UAllPlayMode_SessionSubsystem::BroadcastState(ESessionUIState State, const FString& Message)
{
	if (bIsNetworkError) return;

	LastUIState = State;
	LastUIMessage = Message;

	OnSessionStateChanged.Broadcast(State, Message);

	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, Message);
	}
}

//매치 시작 시 세션 상태 갱신
void UAllPlayMode_SessionSubsystem::MarkSessionInGame()
{
	if (!EnsureSessionInterface()) return;

	FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(SessionName);
	if (!NamedSession) return;

	FOnlineSessionSettings NewSettings = NamedSession->SessionSettings;
	NewSettings.bAllowJoinInProgress = false;
	NewSettings.Set(FName(TEXT("SessionPhase")), FString(TEXT("InGame")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	NewSettings.Set(FName(TEXT("CanQuickMatch")), false, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	SessionInterface->UpdateSession(SessionName, NewSettings, true);
}

//세션이 가득찼을 때 세션 상태 갱신
void UAllPlayMode_SessionSubsystem::MarkSessionInFullLoby()
{
	if (!EnsureSessionInterface()) return;

	FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(SessionName);
	if (!NamedSession) return;

	FOnlineSessionSettings NewSettings = NamedSession->SessionSettings;

	//이미 게임이 시작된 방에 난입 금지 설정
	NewSettings.bAllowJoinInProgress = false;
	NewSettings.Set(FName(TEXT("SessionPhase")), FString(TEXT("LV_Title")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	NewSettings.Set(FName(TEXT("CanQuickMatch")), false, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	SessionInterface->UpdateSession(SessionName, NewSettings, true);
}

void UAllPlayMode_SessionSubsystem::ReturnToTitle()
{
	//UI 팝업 음소거 모드 켜기 (이후 발생하는 세션 파괴 방송을 UI가 듣지 못하게 함)
	bIsNetworkError = true;

	//세션 및 찌꺼기 변수 초기화
	CancelQuickMatchLAN();
	bCancelRequested = false;

	LastUIState = ESessionUIState::None;
	LastUIMessage = TEXT("");

	//GameInstance 상태 초기화
	if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		GameInstance->SetMatchFlowState(EMatchFlowState::None);
	}

	//아주 짧은 시간 뒤에 음소거 해제 (비동기 파괴 신호가 지나간 후)
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().SetTimerForNextTick([this]() {
			bIsNetworkError = false;
		});
	}
	else {
		bIsNetworkError = false;
	}
}

//Play를 위한 매칭 시작
void UAllPlayMode_SessionSubsystem::QuickMatchLAN()
{
	if (!EnsureSessionInterface()) return;
	bCancelRequested = false;
	bIsHostingSession = false;
	bJoinAfterDestroy = false;
	bJoinInProgress = false;
	bSearchAfterDestroy = false;
	bFindInProgress = false;
	bPendingHostAfterFindComplete = false;
	bPendingStartHostMergeCheck = false;
	bGuestJoinedWhenHost = false;
	bHostMatchedBroadCasted = false;

	FindRetryCount = 0;

	PendingJoinResult.Reset();
	LastTriedJoinResult.Reset();
	SessionSearch.Reset();

	UWorld* World = GetWorld();

	if (World) {
		World->GetTimerManager().ClearTimer(DelayedHostTimerHandle);
		World->GetTimerManager().ClearTimer(HostMergeCheckTimerHandle);
		//매칭 시작과 동시에 주기적으로 BlackList 초기화
		World->GetTimerManager().ClearTimer(ResetBlackListTimerHandle);
		World->GetTimerManager().SetTimer(ResetBlackListTimerHandle, this, &UAllPlayMode_SessionSubsystem::ResetSessionBlackList, ResetBlackListTerm, true, 0.f);
	}

	//이전 세션이 남아있는 경우 정리
	if (SessionInterface->GetNamedSession(SessionName)) {
		BroadcastState(ESessionUIState::Searching, TEXT("Destroy existing session first"));

		//[자동매칭버그] 추가
		bSearchAfterDestroy = true;

		LeaveCurrentSession();
		return;
	}

	
	FindLANSessions();
}

void UAllPlayMode_SessionSubsystem::CancelQuickMatchLAN()
{
	if (!EnsureSessionInterface()) return;
	bCancelRequested = true;
	bPendingHostAfterFindComplete = false;
	bPendingStartHostMergeCheck = false;
	bJoinAfterDestroy = false;
	bSearchAfterDestroy = false;
	bJoinInProgress = false;
	bFindInProgress = false;
	bIsHostingSession = false;

	LastUIState = ESessionUIState::None;	//[추가]자동매칭버그
	LastUIMessage = TEXT("");
	if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		GameInstance->SetMatchFlowState(EMatchFlowState::None);
		GameInstance->bPendingCreateLANSession = false;
		GameInstance->bAutoRestartMatch = false;
	}

	PendingJoinResult.Reset();
	IgnoredHostTickets.Reset();
	LastTriedJoinResult.Reset();
	SessionSearch.Reset();

	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(DelayedHostTimerHandle);
		World->GetTimerManager().ClearTimer(HostMergeCheckTimerHandle);
		World->GetTimerManager().ClearTimer(ResetBlackListTimerHandle);
	}

	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}
	
	if (SessionInterface->GetNamedSession(SessionName)) {
		LeaveCurrentSession();
	}
	else {
		BroadcastState(ESessionUIState::None, TEXT("Matching Cancelled!"));
	}
}

//LAN 세션(로컬 멀티플레이) 찾기 시작
void UAllPlayMode_SessionSubsystem::FindLANSessions() {
	if (!EnsureSessionInterface()) return;
	if (bFindInProgress) return;

	BroadcastState(ESessionUIState::Searching, TEXT("Searching LAN sessions..."));

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 20;

	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	bFindInProgress = true;

	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef())) {
		bFindInProgress = false;
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		BroadcastState(ESessionUIState::Failed, TEXT("FindSessions call failed"));
	}
}

//Session 검색 직후 랜덤한 시간이 지난 다음 한번 더 Session 검색
void UAllPlayMode_SessionSubsystem::ScheduleDelayedHost()
{
	if (UWorld* World = GetWorld()) {
		float Delay = FMath::FRandRange(HostCreateDelayMin, HostCreateDelayMax);

		BroadcastState(ESessionUIState::Searching, FString::Printf(TEXT("No Session. Search again after %.2f seconds ..."), Delay));

		World->GetTimerManager().ClearTimer(DelayedHostTimerHandle);
		World->GetTimerManager().SetTimer(DelayedHostTimerHandle, this, &UAllPlayMode_SessionSubsystem::DelayedHostAfterSecondSearch, Delay, false);
	}
}


//Host Session 생성 전 한번 더 Session 검색 
void UAllPlayMode_SessionSubsystem::DelayedHostAfterSecondSearch()
{
	FindLANSessions();
}

//자신이 Host가 됨 (세션 검색 실패시/Join 실패시 호출)
void UAllPlayMode_SessionSubsystem::HostLANSession() {
	if (!EnsureSessionInterface()) return;

	if (bFindInProgress) {
		bPendingHostAfterFindComplete = true;
		return;
	}

	ResetFindStateForHosting(TEXT("HostLANSession"));

	//Listen 맵으로 이동하므로 기존 Timer들 정리
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(DelayedHostTimerHandle);
		World->GetTimerManager().ClearTimer(HostMergeCheckTimerHandle);
		World->GetTimerManager().ClearTimer(ResetBlackListTimerHandle);
	}

	bJoinInProgress = false;
	bSearchAfterDestroy = false;

	BroadcastState(ESessionUIState::Hosting, TEXT("Opening Listen Map first ..."));

	if (UAllPlayMode_GameInstance* gameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		gameInstance->bPendingCreateLANSession = true;
	}

	UGameplayStatics::OpenLevel(GetWorld(), FName("/Game/Levels/LV_Title"), true, TEXT("listen"));

}

void UAllPlayMode_SessionSubsystem::CreateLANSessionInternal()
{
	if (!EnsureSessionInterface()) return;

	UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	if (GameInstance) {
		GameInstance->bPendingCreateLANSession = false;
	}

	ResetFindStateForHosting(TEXT("CreateLANSessionInternal"));

	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(ResetBlackListTimerHandle);
		World->GetTimerManager().SetTimer(ResetBlackListTimerHandle, this, &UAllPlayMode_SessionSubsystem::ResetSessionBlackList, ResetBlackListTerm, true, 0.f);
	}

	BroadcastState(ESessionUIState::Hosting, TEXT("Hosting LAN Session..."));

	LocalHostTicket = FMath::RandRange(1, 10000000);

	//매칭 최대 인원수 및 매칭 모드 정보 획득
	int32 MaxPlayers = GameInstance ? GameInstance->GetMaxPlayersByMode() : 2;
	int32 MatchModeInt = GameInstance ? (int32)GameInstance->GetSelectedMatchMode() : (int32)EMatchMode::TwoPlayers;

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = true;
	Settings.NumPublicConnections = MaxPlayers;
	//세션의 검색 허용 유무
	Settings.bShouldAdvertise = true;
	//게임 진행 중에도 세션에 빈자리가 있으면 참여 허용 유무
	Settings.bAllowJoinInProgress = true;
	//Presence/Lobby 방식 사용 유무
	Settings.bUsesPresence = false;

	//각 플레이어의 세션을 세팅 (검색 필터 추가)
	Settings.Set(FName(TEXT("MatchType")), FString(TEXT("ProjectCC_LAN")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FName(TEXT("HostTicket")), LocalHostTicket, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FName(TEXT("SessionPhase")), FString(TEXT("LV_Title")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FName(TEXT("CanQuickMatch")), true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	//매칭 모드 세션 세팅 (2인 4인 구별용)
	Settings.Set(FName(TEXT("MatchMode")), MatchModeInt, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	SessionInterface->CreateSession(0, SessionName, Settings);
}

void UAllPlayMode_SessionSubsystem::ResetFindStateForHosting(const TCHAR* Reason) {
	bFindInProgress = false;
	bPendingStartHostMergeCheck = false;
	bPendingHostAfterFindComplete = false;

	SessionSearch.Reset();
	//호스트로 전환하면서 블랙리스트 초기화 (모종의 이유로 정상 Session에 연결 실패 가능성 우려)
	IgnoredHostTickets.Reset();

	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(DelayedHostTimerHandle);
		World->GetTimerManager().ClearTimer(ResetBlackListTimerHandle);
	}
}

//다른 Host 세션에 참여
void UAllPlayMode_SessionSubsystem::JoinLANSession(const FOnlineSessionSearchResult& result)
{
	if (!EnsureSessionInterface()) return;
	if (bJoinInProgress) return;
	if (SessionInterface->GetNamedSession(SessionName)) {
		PendingJoinResult = MakeShared<FOnlineSessionSearchResult>(result);
		bJoinAfterDestroy = true;
		LeaveCurrentSession();
		return;
	}

	bJoinInProgress = true;
	
	//Join Delegate 등록
	BroadcastState(ESessionUIState::Joining, TEXT("Joining Session..."));
	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	SessionInterface->JoinSession(0, SessionName, result);
}

//현재 세션에서 벗어남 (세션이 비정상적으로 꼬인 경우)
void UAllPlayMode_SessionSubsystem::LeaveCurrentSession()
{
	if (!EnsureSessionInterface()) return;
	if (!SessionInterface->GetNamedSession(SessionName)) {
		BroadcastState(ESessionUIState::None, TEXT("No Session to Destroy"));
		return;
	}

	// [자동매칭버그] 중복 방지 위해 이전 델리게이트 끊음
	SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);

	DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
	SessionInterface->DestroySession(SessionName);
}

void UAllPlayMode_SessionSubsystem::NotifyHostPlayerJoin()
{
	if (!bIsHostingSession) return;
	if (bHostMatchedBroadCasted) return;

	bGuestJoinedWhenHost = true;

	UAllPlayMode_GameInstance* GI = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	int32 MaxPlayers = GI ? GI->GetMaxPlayersByMode() : 2;

	FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(SessionName);
	if (!NamedSession) return;

	int32 OpenSlots = NamedSession->NumOpenPublicConnections;
	int32 CurrentPlayers = MaxPlayers - OpenSlots;

	if (CurrentPlayers < MaxPlayers) {
		BroadcastState(ESessionUIState::Hosting, FString::Printf(TEXT("Player Joined! Wait for More (%d, %d)"), CurrentPlayers, MaxPlayers));
		return;
	}

	bHostMatchedBroadCasted = true;
	
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(HostMergeCheckTimerHandle);
		World->GetTimerManager().ClearTimer(ResetBlackListTimerHandle);
	}

	MarkSessionInFullLoby();
	BroadcastState(ESessionUIState::Matched, TEXT("Matching Complete!"));
}

//Host에 플레이어가 Join중임을 알림 (UI 반영)
void UAllPlayMode_SessionSubsystem::NotifyHostPlayerJoining()
{
	if (!bIsHostingSession) return;
	if (bHostMatchedBroadCasted) return;

	bGuestJoinedWhenHost = true;

	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(HostMergeCheckTimerHandle);
	}

	UAllPlayMode_GameInstance* GI = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
	int32 MaxPlayers = GI ? GI->GetMaxPlayersByMode() : 2;

	if (MaxPlayers > 2) BroadcastState(ESessionUIState::Hosting, TEXT("Player Joined! Wait for More"));
	else BroadcastState(ESessionUIState::Matched, TEXT("Matching Complete!"));
}

//세션 검색에 성공하였을 경우
void UAllPlayMode_SessionSubsystem::OnFindSessionsCompleted(bool bWasSuccessful)
{
	bFindInProgress = false;

	//Find Delegate해제
	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}
	//Host 예약 상태라면 검색이 끝나면 Host가 됨
	if (bPendingHostAfterFindComplete) {
		bPendingHostAfterFindComplete = false;
		HostLANSession();
		return;
	}

	//Host Merge 예약 상태라면 여기서 Host Merge
	if (bPendingStartHostMergeCheck) {
		bPendingStartHostMergeCheck = false;
		StartHostMergeCheck();
		return;
	}

	//검색 실패 시 일정 시간 대기 후 재검색
	if (!bWasSuccessful || !SessionSearch.IsValid()) {
		if (bIsHostingSession) {
			BroadcastState(ESessionUIState::Failed, TEXT("Host merge check find failed"));
		}
		else {
			BroadcastState(ESessionUIState::Failed, TEXT("FindSessions failed"));
		}
		ScheduleDelayedHost();
		return;
	}

	//자신이 Host인 경우 다른 Host와 충돌 체크
	if (bIsHostingSession) {
		//현재 자신의 Session 상태 파악
		FNamedOnlineSession* MySession = SessionInterface->GetNamedSession(SessionName);
		if (!MySession) return;

		UAllPlayMode_GameInstance* GI = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
		int32 MaxPlayers = GI ? GI->GetMaxPlayersByMode() : 2;

		int32 MyOpenSlots = MySession->NumOpenPublicConnections;
		int32 MyPlayerCount = MaxPlayers - MyOpenSlots;

		if (MyOpenSlots <= 0) return;

		const FOnlineSessionSearchResult* BestOtherHost = nullptr;
		int32 BestOtherTicket = MAX_int32;
		int32 BestOtherPlayerCount = -1;

		//검색에 성공하면 결과 목록 확인
		for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults) {
			//검색 목록 중 같은 게임의 세션만 등록
			FString MatchType;
			Result.Session.SessionSettings.Get(FName(TEXT("MatchType")), MatchType);
			if (MatchType != TEXT("ProjectCC_LAN")) continue;

			//검색 목록 중 세션의 현재 게임 상태 확인 (Title만 후보에 등록)
			FString SessionPhase;
			Result.Session.SessionSettings.Get(FName(TEXT("SessionPhase")), SessionPhase);
			if (SessionPhase != TEXT("LV_Title")) continue;

			//검색 목록 중 세션의 현재 상태가 퀵매치 가능 상태인지 확인
			bool bCanQuickMatch = false;
			Result.Session.SessionSettings.Get(FName(TEXT("CanQuickMatch")), bCanQuickMatch);
			if (!bCanQuickMatch) continue;

			//호스트끼리 방을 합치기 위한 티켓 비교 전 인원수 매치모드 검사 (2인 이상)
			int32 SessionMatchMode = 0;
			Result.Session.SessionSettings.Get(FName(TEXT("MatchMode")), SessionMatchMode);
			int32 MyMatchMode = GI ? (int32)GI->GetSelectedMatchMode() : (int32)EMatchMode::TwoPlayers;
			if (SessionMatchMode != MyMatchMode) continue;

			//검색 목록 중 다른 플레이어의 HostTicket 확인
			int32 OtherHostTicket = 0;
			Result.Session.SessionSettings.Get(FName(TEXT("HostTicket")), OtherHostTicket);
			if (OtherHostTicket == 0 || OtherHostTicket == LocalHostTicket) continue;

			int32 OtherOpenSlots = Result.Session.NumOpenPublicConnections;
			int32 OtherPlayerCount = MaxPlayers - OtherOpenSlots;

			//빈자리가 하나도 없으면 무시
			if (OtherOpenSlots < 1) continue;

			bool bShouldSurrender = false;

			if (OtherPlayerCount > MyPlayerCount) bShouldSurrender = true;
			else if (OtherPlayerCount == MyPlayerCount) {
				if (OtherHostTicket < LocalHostTicket) bShouldSurrender = true;
			}

			if (bShouldSurrender) {
				if (OtherPlayerCount > BestOtherPlayerCount || (OtherPlayerCount == BestOtherPlayerCount && OtherHostTicket < BestOtherTicket)) {
					BestOtherPlayerCount = OtherPlayerCount;
					BestOtherTicket = OtherHostTicket;
					BestOtherHost = &Result;
				}
			}

		}
		//합병 대상 Host가 있다면 그쪽으로 Session 이동
		if (BestOtherHost) {
			BroadcastState(ESessionUIState::Joining, TEXT("Go To Other Player's Room"));
			bIsHostingSession = false;

			if (UWorld* World = GetWorld()) {
				World->GetTimerManager().ClearTimer(HostMergeCheckTimerHandle);

				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
					ATitle_PlayerController* PC = Cast<ATitle_PlayerController>(It->Get());
					if (PC && PC->IsLocalController() == false) PC->Client_ExitedByHost();
				}
			}

			PendingJoinResult = MakeShared<FOnlineSessionSearchResult>(*BestOtherHost);
			bJoinAfterDestroy = true;
			LeaveCurrentSession();
			return;
		}

		StartHostMergeCheck();
		return;
	}

	//이미 Join 중이라면 중복 방지
	if (bJoinInProgress) {
		return;
	}

	//자신이 Host가 아닌 경우 필터 적용 Session 검색 후 Join
	for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults) {
		//검색 목록 중 같은 게임의 세션만 등록
		FString MatchType;
		Result.Session.SessionSettings.Get(FName(TEXT("MatchType")), MatchType);
		if (MatchType != TEXT("ProjectCC_LAN")) continue;

		//검색 목록 중 세션의 현재 게임 상태 확인 (Title만 후보에 등록)
		FString SessionPhase;
		Result.Session.SessionSettings.Get(FName(TEXT("SessionPhase")), SessionPhase);
		if (SessionPhase != TEXT("LV_Title")) continue;

		//검색 목록 중 세션의 현재 상태가 퀵매치 가능 상태인지 확인
		bool bCanQuickMatch = false;
		Result.Session.SessionSettings.Get(FName(TEXT("CanQuickMatch")), bCanQuickMatch);
		if (!bCanQuickMatch) continue;

		//매칭 모드가 서로 다르면 무시
		int32 SessionMatchMode = 0;
		Result.Session.SessionSettings.Get(FName(TEXT("MatchMode")), SessionMatchMode);
		UAllPlayMode_GameInstance* GI = Cast<UAllPlayMode_GameInstance>(GetGameInstance());
		int32 MyMatchMode = GI ? (int32)GI->GetSelectedMatchMode() : (int32)EMatchMode::TwoPlayers;
		if (SessionMatchMode != MyMatchMode) continue;

		int32 OpenSlots = Result.Session.NumOpenPublicConnections;
		if (OpenSlots <= 0) continue;

		int32 HostTicket = 0;
		Result.Session.SessionSettings.Get(FName(TEXT("HostTicket")), HostTicket);
		if (IgnoredHostTickets.Contains(HostTicket)) continue;

		LastTriedJoinResult = MakeShared<FOnlineSessionSearchResult>(Result);
		JoinLANSession(Result);
		return;
	}

	//검색 실패 시 최대 검색 횟수가 될 때 까지 재검색
	if (FindRetryCount < MaxFindRetryCount) {
		FindRetryCount++;
		ScheduleDelayedHost();
		return;
	}

	HostLANSession();
}
//세션 생성에 성공하였을 경우
void UAllPlayMode_SessionSubsystem::OnCreateSessionCompleted(FName sessionName, bool bWasSuccessful)
{
	//Create Delegate 해제
	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}

	if (UAllPlayMode_GameInstance* GameInstance = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		GameInstance->bPendingCreateLANSession = false;
	}

	if (!bWasSuccessful) {
		BroadcastState(ESessionUIState::Failed, TEXT("CreateSession failed"));
		return;
	}
	bIsHostingSession = true;

	//Hosting 상태를 BroadCast
	BroadcastState(ESessionUIState::Hosting, TEXT("Session created, Waiting for the match to complete"));

	if (bFindInProgress) {
		bPendingStartHostMergeCheck = true;
	}
	else {
		StartHostMergeCheck();
	}

	if (bWasSuccessful) {
		LastHostStateChangeTime = FPlatformTime::Seconds();
		HostMergeMissCount = 0;
	}
}

//Host 충돌 확인
void UAllPlayMode_SessionSubsystem::StartHostMergeCheck()
{
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(HostMergeCheckTimerHandle);
		//Host 충돌이 발생했는지 짧게 재검색
		World->GetTimerManager().SetTimer(HostMergeCheckTimerHandle, this, &UAllPlayMode_SessionSubsystem::HostMergeCheckTick, FMath::RandRange(0.5f, 2.5f), false);
	}
}

void UAllPlayMode_SessionSubsystem::HostMergeCheckTick()
{
	if (!EnsureSessionInterface() || !bIsHostingSession) return;
	if (bFindInProgress) return;

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 20;

	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	bFindInProgress = true;

	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef())) {
		bFindInProgress = false;
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}
	
}

//Join 실패 블랙리스트 초기화
void UAllPlayMode_SessionSubsystem::ResetSessionBlackList()
{
	IgnoredHostTickets.Reset();
}

//세션 참여에 성공하였을 경우
void UAllPlayMode_SessionSubsystem::OnJoinSessionCompleted(FName sessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}

	bJoinInProgress = false;

	if (Result != EOnJoinSessionCompleteResult::Success) {
		if (LastTriedJoinResult.IsValid()) {
			int32 HostTicket = 0;
			LastTriedJoinResult->Session.SessionSettings.Get(FName(TEXT("HostTicket")), HostTicket);
			if (HostTicket != 0) {
				IgnoredHostTickets.Add(HostTicket);
			}
		}
		BroadcastState(ESessionUIState::Failed, TEXT("JoinSession Failed"));
		if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(SessionName)) {
			bSearchAfterDestroy = true;
			LeaveCurrentSession();
		}
		else {
			FindLANSessions();
		}

		return;
	}

	FString ConnectString;
	bool bResolved = SessionInterface->GetResolvedConnectString(sessionName, ConnectString);

	if (!bResolved || ConnectString.IsEmpty() || ConnectString.EndsWith(":0")) {
		if (LastTriedJoinResult.IsValid()) {
			int32 HostTicket = 0;
			LastTriedJoinResult->Session.SessionSettings.Get(FName(TEXT("HostTicket")), HostTicket);
			if (HostTicket != 0) {
				IgnoredHostTickets.Add(HostTicket);
			}
		}

		BroadcastState(ESessionUIState::Failed, TEXT("Invalid host Connect String"));

		if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(SessionName)) {
			bSearchAfterDestroy = true;
			LeaveCurrentSession();
		}
		else {
			bSearchAfterDestroy = false;
			FindLANSessions();
		}
		return;
	}

	BroadcastState(ESessionUIState::Matched, TEXT("Matching Complete!"));

	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(DelayedHostTimerHandle);
		World->GetTimerManager().ClearTimer(HostMergeCheckTimerHandle);
		World->GetTimerManager().ClearTimer(ResetBlackListTimerHandle);
	}

	if (APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr) {
		PC->ClientTravel(ConnectString, TRAVEL_Absolute);
	}
}

//세션이 종료되었을 경우 (참여 세션이 비정상적으로 꼬였을 경우)
void UAllPlayMode_SessionSubsystem::OnDestroySessionCompleted(FName sessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}

	if (bCancelRequested) {
		bCancelRequested = false;
		BroadcastState(ESessionUIState::None, TEXT("Matching Cancelled!"));
		return;
	}

	if (!bJoinAfterDestroy) {
		BroadcastState(ESessionUIState::None, TEXT("Session Destroyed"));
	}

	BroadcastState(ESessionUIState::None, TEXT("Session destroyed"));

	//Host가 HostTicket으로 정해졌을 경우 정해진 Host로 즉시 Join
	if (bJoinAfterDestroy && PendingJoinResult.IsValid()) {
		bJoinAfterDestroy = false;
		FOnlineSessionSearchResult SavedResult = *PendingJoinResult;
		PendingJoinResult.Reset();
		JoinLANSession(SavedResult);
		
		return;
	}

	if (bSearchAfterDestroy) {
		bSearchAfterDestroy = false;
		FindLANSessions();
		return;
	}

	if (UAllPlayMode_GameInstance* GI = Cast<UAllPlayMode_GameInstance>(GetGameInstance())) {
		if (GI->bAutoRestartMatch) return;
	}

	//새로운 세션을 검색
	//FindLANSessions();	//[자동매칭버그] 주석처리
}


