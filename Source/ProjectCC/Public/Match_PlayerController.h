// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AllPlayMode_GameInstance.h"
#include "Match_PlayerController.generated.h"

/**
 * 각 플레이어가 사용하는 player controller
 */

class UInputMappingContext;
class UPlayer_ControllerWidget;
class UPlayer_ResultWidget;
class UMatch_ScoreBoardWidget;
class UMatch_ShopWidget;

UCLASS()
class PROJECTCC_API AMatch_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetPawn(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	void SetSpectatingDefaultCamera(bool bInValue) { bSpectatingDefaultCamera = bInValue; }
	bool IsSpectatingDefaultCamera() { return bSpectatingDefaultCamera; }

	//리스폰 대기중인지 확인
	bool IsWaitingRespawn() { return bWaitingRespawn; }
	//상점이 열려있는지 확인
	bool IsShopOpen() const { return bIsShopOpen; }
	//상점 버튼 활성화/비활성화 결정
	void UpdateShopButtonVisibility();

	void SaveResultData(const FMatchResultData& OwnerResult, const TArray<FMatchResultData>& AllResults);
	void EndMatch();

	void SetCurrentSpectatingTarget(AActor* Target) { CurrentSpectatingTarget = Target; }
	AActor* GetCurrentSpectatingTarget() const { return CurrentSpectatingTarget; }
	UPlayer_ControllerWidget* GetPlayerScreenWidget() const { return ScreenWidget; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MatchInputMappingContext;
	//플레이어 화면 UI Widget 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayer_ControllerWidget> Player_ControllerWidget;
	//플레이어 화면 UI Widget
	UPROPERTY()
	TObjectPtr<UPlayer_ControllerWidget> ScreenWidget;
	//플레이어 스코어보드 UI Widget 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMatch_ScoreBoardWidget> Match_ScoreBoardWidget;
	//플레이어 스코어보드 UI WIdget
	UPROPERTY()
	TObjectPtr<UMatch_ScoreBoardWidget> ScoreWidget;
	//플레이어 상점 UI Widget 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMatch_ShopWidget> Match_ShopWidget;
	//플레이어 상점 UI Widget
	UPROPERTY()
	TObjectPtr<UMatch_ShopWidget> ShopWidget;

	//현재 관전중은 플레이어
	UPROPERTY()
	TObjectPtr<AActor> CurrentSpectatingTarget = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float SetupRetryInterval = 0.1f;

	UPROPERTY()
	bool bShopOpen = false;
	UPROPERTY()
	bool bAlreadyPurchasedInShop = false;

	FTimerHandle SetupRetryTimerHandle;

	bool bInputMappingApplied = false;
	bool bReportedMatchLoaded = false;

	bool bWaitingRespawn = false;
	bool bCanRespawnNow = false;

	bool bSpectatingDefaultCamera = false;

	bool bIsShopOpen = false;

	void TryFinishLocalSetup();
	void ApplyLocalInputMapping();

	void ApplyGameInputMode();
	void ApplyUIInputMode();

	void OnPressedSpaceKey();

	void SetOutWidget();
	void SetPlayWidget();

	//스코어보드 열기
	void OpenScoreBoard();
	//스코어보드 닫기
	void CloseScoreBoard();

	//상점 버튼 바인딩
	UFUNCTION()
	void BindShopButton();
	//상점 팝업 열기
	UFUNCTION()
	void OpenShop();
	//상점 팝업 닫기
	UFUNCTION()
	void CloseShop();

	// [사운드] =================================================
	// [사운드] 카운트다운 효과음
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<class USoundBase> CountdownSound;

	// [사운드] 이벤트 시작 전 경고음
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<class USoundBase> EventWarningSound;

	// [사운드] 경고음 재생정지를 조종할 컴포넌트
	UPROPERTY()
	TObjectPtr<class UAudioComponent> WarningAudioComponent;
public:
	// [사운드] 매치 종료시 bgm 정지 함수
	UFUNCTION(Client,Reliable)
	void Client_FadeOutBgm();

public:
	//서버에 Player_State 데이터를 전송
	UFUNCTION(Server, Reliable)
	void Server_SubmitMatchData(const FString& nickname, int32 portraitId);

	//서버에 게임 시작 준비가 완료됨을 전송
	UFUNCTION(Server, Reliable)
	void Server_ReportMatchLoaded();
	//서버에 수동 리스폰 요청
	UFUNCTION(Server, Reliable)
	void Server_RequestRespawn();
	UFUNCTION(Server, Reliable)
	void Server_Purchase(EShopBoxs Box);
public:
	//Game 입력 모드 Client RPC
	UFUNCTION(Client, Reliable)
	void Client_ApplyGameInputMode();
	//UI 입력 모드 Client RPC
	UFUNCTION(Client, Reliable)
	void Client_ApplyUIInputMode();
	//관전 시작 Client RPC
	UFUNCTION(Client, Reliable)
	void Client_StartSpectating(AActor* SpectatorTarget);
	//현재 리스폰 데이터를 세팅 Client RPC
	UFUNCTION(Client, Reliable)
	void Client_SetRespawnState(bool bWaiting, bool bCanRespawn);
	UFUNCTION(Client, Reliable)
	void Client_StartSpectatingPlayer(APlayer_Character* SpectatorTarget);
	UFUNCTION(Client, Reliable)
	void Client_StartSpectatingDefaultCamera();
	UFUNCTION(Client, Reliable)
	void Client_SetPreMatchDelay();
	UFUNCTION(Client, Reliable)
	void Client_StartCountDown();
	UFUNCTION(Client, Reliable)
	void Client_UpdateCountDown(int32 number);
	UFUNCTION(Client, Reliable)
	void Client_StartPlayingUI();
	UFUNCTION(Client, Reliable)
	void Client_ShopPurchaseResult(bool bSuccess);
	UFUNCTION(Client, Reliable)
	void Client_EndMatch();
	UFUNCTION(Client, Reliable)
	void Client_SaveResultData(const FMatchResultData& OwnerResult, const TArray<FMatchResultData>& AllResults);

	UFUNCTION(Client, Reliable)
	void Client_UpdateMatchEventCountdown(FName EventName, int32 SecondsUntilEvent);
	UFUNCTION(Client, Reliable)
	void Client_ShowMatchEventActive(FName EventName, int32 RemainSeconds);
	UFUNCTION(Client, Reliable)
	void Client_HideMatchEventUI();

};
