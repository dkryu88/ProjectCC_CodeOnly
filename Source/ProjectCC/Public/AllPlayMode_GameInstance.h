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

	// [사운드] BGM 재생 관련 함수============================================================
	// BGM 재생
	UFUNCTION(BlueprintCallable, Category="Sound")
	void PlayBgm(USoundBase* NewBgm);

	// 현재 재생중인 BGM 정지
	UFUNCTION(BlueprintCallable, Category="Sound")
	void StopBgm();

	// BGM 재생길이 초 단위로 변환
	UFUNCTION(BlueprintPure,Category="Sound")
	float GetCurrnetBgmDuration() const;

	// 지정한 변화시간동안 볼륨 조절시킴(유지시간이 아니라 변화되는 시간, 타켓볼륨 : 20%로 줄이고 싶다면 0.2f 입력)
	UFUNCTION(BlueprintCallable, Category="Sound")
	void AdjustBgmVolume(float FadeTime, float TargetVolum);

	// bgm 재생속도(pitch)를 변경
	UFUNCTION(BlueprintCallable, Category = "Sound")
	void SetBgmPitch(float NewPitch);


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

	// [사운드] =============================================================================
	// 현재 재생중인 오디오 기억하고 제어하기 위한 컴포넌트 포인터
	UPROPERTY(BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* CurrentBgmComponent;

	// 다음 재생할 BGM을 임시로 기억할 변수
	UPROPERTY()
	USoundBase* NextBgm;

	// Fade Out 대기를 위한 타이머 핸들
	FTimerHandle BgmTransitionTimer;

	// 1초 뒤 실제로 Fade In을 시작할 내부 함수
	void PlayNextBgm();

	// BGM 전환 시 사용할 페이드 및 타이머 시간 (기본값 1초)
	UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	float BgmFadeDuration = 1.f;

};

//BlueprintPure : 실행 핀 없이 단순히 값만 Return하는 함수
//meta = (AllowPrivateAccess = "true" : protected 변수를 자식 블루프린트가 꺼내 쓸 수 있게 강제하는 언리얼 규칙
