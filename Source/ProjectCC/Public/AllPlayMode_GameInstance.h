// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AllPlayMode_GameInstance.generated.h"

/**
 * 게임이 실행되는 동안 계속 살아있는 클래스
 * 게임 전체에 영향을 미칠 것들을 저장
 */

USTRUCT(BlueprintType)
struct FMatchResultData {
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Nickname;

	UPROPERTY(BlueprintReadWrite)
	int32 Rank = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Coin = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Eliminate = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Out = 0;

	UPROPERTY(BlueprintReadWrite)
	FName MapName;
};

UENUM(BlueprintType)
enum class EMatchFlowState : uint8 {
	None,
	Idle,
	Searching,
	Hosting,
	Joining,
	InLobby,
	PreMatch
};

UCLASS()
class PROJECTCC_API UAllPlayMode_GameInstance : public UGameInstance
{
	GENERATED_BODY()
	

public:
	//각 플레이어가 사용할 닉네임 설정
	UFUNCTION(BlueprintCallable, Category = "Title")
	void SetLocalPlayerNickname(const FString& NewNickName);

	//각 플레이어가 사용하는 닉네임 획득
	UFUNCTION(BlueprintPure, Category = "Title")
	FString GetPlayerLocalNickname();
	//각 플레이어의 초상화 번호 획득/설정
	UFUNCTION(BlueprintCallable)
	void SetLocalPortraitId(int32 PortraitId) { LocalPortraitId = PortraitId; }
	UFUNCTION(BlueprintPure)
	int32 GetLocalPortraitId() { return LocalPortraitId; }
	//플레이어의 현재 게임 흐름 상태 획득/설정
	UFUNCTION(BlueprintCallable)
	void SetMatchFlowState(EMatchFlowState State) { MatchFlowState = State; }
	UFUNCTION(BlueprintPure)
	EMatchFlowState GetMatchFlowState() { return MatchFlowState; }

	void SetMatchResult(const FMatchResultData& Data) { MatchResult = Data; }
	void SetMatchResults(const TArray<FMatchResultData>& Datas) { MatchResults = Datas; }
	FMatchResultData& GetMatchResult() { return MatchResult; }
	TArray<FMatchResultData>& GetMatchResults() { return MatchResults; }

	UPROPERTY()
	bool bPendingCreateLANSession = false;

protected:
	//각 플레이어들의 닉네임
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Title")
	FString LocalPlayerNickName;
	//각 플레이어의 초상화 번호
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 LocalPortraitId = 0;
	//플레이어의 현재 게임 흐름 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EMatchFlowState MatchFlowState = EMatchFlowState::Idle;

	UPROPERTY()
	FMatchResultData MatchResult;

	UPROPERTY()
	TArray<FMatchResultData> MatchResults;
};

//BlueprintPure : 실행 핀 없이 단순히 값만 Return하는 함수