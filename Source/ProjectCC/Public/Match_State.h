// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Match_State.generated.h"

/**
 * 플레이어가 GameMode로 직접 접근이 불가능하므로
 * GameState를 통해 순위나 남은 시간을 획득
 */

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMatchTimeChanged, int32);

UCLASS()
class PROJECTCC_API AMatch_State : public AGameState
{
	GENERATED_BODY()

public:
	AMatch_State();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_MatchTime, BlueprintReadOnly, Category = "Match")
	int32 MatchTime = 0;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	bool bMatchStarted = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	bool bMatchEnded = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	float DelayEndServerTime = -1.f;

	UFUNCTION()
	void OnRep_MatchTime();

public:
	void SetDelayEndServerTime(float Time);
	void SetMatchTime(int32 NewMatchTime);
	void SetMatchStarted(bool bMatchState);
	void SetMatchEnded(bool bMatchState);

	float GetDelayEndServerTime(){ return DelayEndServerTime; }
	int32 GetMatchTime() const { return MatchTime; }
	bool IsMatchStarted() const { return bMatchStarted; }
	bool IsMatchEnded() const { return bMatchEnded; }

	FOnMatchTimeChanged OnMatchTimeChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

/*알아두기
Replicated <- 서버에서 값이 바뀌면 클라이언트들에게 자동으로 변경을 전파하는 것
*/