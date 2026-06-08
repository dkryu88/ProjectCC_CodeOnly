// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "AllPlayMode_SessionSubsystem.generated.h"

/**
 * 멀티 플레이
 */

//현재 플레이어 멀티플레이 상태
UENUM(BlueprintType)
enum class ESessionUIState : uint8 {
	None,
	Searching,
	Hosting,
	Joining,
	Matched,
	Failed,
	InLobby
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionStateChanged, ESessionUIState, NewState, const FString&, Message);

UCLASS()
class PROJECTCC_API UAllPlayMode_SessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UAllPlayMode_SessionSubsystem();
	//Play를 위한 매칭 시작
	UFUNCTION(BlueprintCallable)
	void QuickMatchLAN();
	//매칭 취소
	UFUNCTION(BlueprintCallable)
	void CancelQuickMatchLAN();

	UFUNCTION(BlueprintCallable)
	void LeaveCurrentSession();

	//플레이어가 Join 했음을 알림(Host)
	UFUNCTION()
	void NotifyHostPlayerJoin();
	//플레이어가 Join중임을 알림(Host)
	UFUNCTION()
	void NotifyHostPlayerJoining();

	UPROPERTY(BlueprintAssignable)
	FOnSessionStateChanged OnSessionStateChanged;

	//블랙리스트 초기화 주기
	UPROPERTY(BlueprintReadWrite)
	float ResetBlackListTerm = 180.f;

	UPROPERTY(BlueprintReadOnly)
	int32 TotalPlayers = 0;

	UFUNCTION(BlueprintCallable)
	void CreateLANSessionInternal();

	void ResetFindStateForHosting(const TCHAR* Reason);

	void SetTotalMatchPlayers(int32 Players) { TotalPlayers = Players; }
	int32 GetTotalMatchPlayers() { return TotalPlayers; }

	void MarkSessionInGame();
	void MarkSessionInFullLoby();

	ESessionUIState LastUIState = ESessionUIState::None;
	FString LastUIMessage;

protected:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FName SessionName = NAME_GameSession;

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteHandle;

	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteHandle;

	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteHandle;

	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteHandle;

	TSharedPtr<FOnlineSessionSearchResult> PendingJoinResult;

	//Session 블랙리스트 (실패한 Session HostTicket 모음)
	TSet<int32> IgnoredHostTickets;
	TSharedPtr<FOnlineSessionSearchResult> LastTriedJoinResult;
	FTimerHandle ResetBlackListTimerHandle;

	//매칭 취소 요청이 들어왔는지 확인
	bool bCancelRequested = false;
	//Host Session인지 확인
	bool bIsHostingSession = false;
	//정해진 Host가 존재해서 Host Session제거 후 참여할 것인지 확인
	bool bJoinAfterDestroy = false;
	bool bFindInProgress = false;
	bool bJoinInProgress = false;
	bool bSearchAfterDestroy = false;
	bool bPendingHostAfterFindComplete = false;
	bool bPendingStartHostMergeCheck = false;

	//호스트에 다른 플레이어가 Join했는지 여부
	bool bGuestJoinedWhenHost = false;
	//매칭 완료 알림 중복 방지
	bool bHostMatchedBroadCasted = false;

	//재검색 횟수
	int32 FindRetryCount = 0;
	//재검색 최대 횟수
	int32 MaxFindRetryCount = 1;
	//플레이어의 HostTicket
	int32 LocalHostTicket = 0;

	int32 HostMergeMissCount = 0;
	float SoloHostRefindMaxSeconds = 8.f;
	float SoloHostRefindMinSeconds = 3.f;
	double LastHostStateChangeTime = 0.f;


	//Host Session 생성 딜레이 최솟값
	float HostCreateDelayMin = 0.2f;
	//Host Session 생성 딜레이 최댓값
	float HostCreateDelayMax = 1.5f;

	//Host Session 생성 딜레이 타이머
	FTimerHandle DelayedHostTimerHandle;
	//Host 병합 체크 타이머
	FTimerHandle HostMergeCheckTimerHandle;



protected:
	//플레이어의 세션 시스템이 준비되었는지 확인
	bool EnsureSessionInterface();
	//현재 플레이어 세션 상태를 전파
	void BroadcastState(ESessionUIState State, const FString& Message);

	//LAN 세션(로컬 멀티플레이) 찾기 시작
	void FindLANSessions();
	void HostLANSession();
	void JoinLANSession(const FOnlineSessionSearchResult& result);

	void OnCreateSessionCompleted(FName sessionName, bool bWasSuccessful);
	void OnFindSessionsCompleted(bool bWasSuccessful);
	void OnJoinSessionCompleted(FName sessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionCompleted(FName sessionName, bool bWasSuccessful);

	void ScheduleDelayedHost();
	void DelayedHostAfterSecondSearch();
	void StartHostMergeCheck();
	void HostMergeCheckTick();
	void ResetSessionBlackList();
};
