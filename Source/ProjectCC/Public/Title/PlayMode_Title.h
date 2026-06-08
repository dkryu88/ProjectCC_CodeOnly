// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OnlineSubsystemTypes.h"
#include "PlayMode_Title.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API APlayMode_Title : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	APlayMode_Title();

	virtual void BeginPlay() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

protected:
	UPROPERTY(EditDefaultsOnly,Category="Match")
	int32 MaxMatchPlayers = 2;

	UPROPERTY(EditDefaultsOnly, Category="Match")
	FString ReadyLevelPath = TEXT("/Game/Levels/LV_Ready");

	bool bTravelingToReady = false;

	void CheckAutoStartReadyLevel();
};
