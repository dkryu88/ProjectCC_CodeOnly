// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Ready_GameState.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AReady_GameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AReady_GameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//현재 매칭된 플레이어 수
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 ConnectedPlayers = 0;
	//선택된 맵 이름 (플레이어에게 보여줌)
	UPROPERTY(Replicated, BlueprintReadOnly)
	FString SelectedMapDisplayName;
	//선택된 맵 LV 이름
	UPROPERTY(Replicated, BlueprintReadOnly)
	FName SelectedMapLevelName;
	//플레이어 전체가 로딩이 완료된 경우
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bAllPlayersReadyToTravel = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Ready")
	FString SelectedMapPath = TEXT("/Game/Levels/");

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Ready")
	float MatchStartServerTime = -1.f;
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Ready")
	float StartCountDonwDuration = 3.f;

public:
	void SetConnectedPlayers(int32 NewConnectedPlayers);
	void SetSelectedMapInfo(const FString& InDisplayName);
	void SetAllPlayersReadyToTravel(bool bReady);
	void SetMatchStartServerTime(float ServerTime);

	float GetCountdownRemaining();
	bool HasCountdownStarted() { return MatchStartServerTime > 0.f; }
};
