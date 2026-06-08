// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PlayMode_Ready.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API APlayMode_Ready : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	APlayMode_Ready();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	void NotifyPlayerProfileSynced(APlayerController* PC);
	void NotifyReadyScreenLoaded(APlayerController* PC);
	void NotifyReadyToTravel(APlayerController* PC);

	FTimerHandle MatchStartTimerHandle;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Ready")
	int32 MaxPlayers = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Ready")
	TArray<FName> AvailableMatchLevels;

	bool bTravelingToMatch = false;

	//플레이할 맵을 랜덤으로 선정
	void ChooseRandomMatchLevel();
	//현재 접속 인원 수를 GameState에 반영
	void UpdateConnectedPlayers();
	//게임 시작 판정
	void CheckAutoStartMatch();
	//플레이어 초상화 배정
	void AssignPortraitId();

	void StartMatchTravel();
};
