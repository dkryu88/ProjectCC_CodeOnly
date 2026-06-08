// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Ready_PlayerController.generated.h"

/**
 * 
 */
class UReady_LoadingWidget;

UCLASS()
class PROJECTCC_API AReady_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UFUNCTION()
	void TryNotifyReadyToTravel();

	UFUNCTION(Server, Reliable)
	void Server_NotifyReadyScreenLoaded();

	UFUNCTION(Server, Reliable)
	void Server_NotifyReadyToTravel();

	//매치 모드 Player_State에 데이터 전송
	UFUNCTION(Server, Reliable)
	void Server_SubmitData(const FString& nickname, int32 portraitId);

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UReady_LoadingWidget> Ready_LoadingWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UReady_LoadingWidget> Ready_LoadingWidget;
	
protected:
	bool bDataSubmitted = false;
	bool bReadyScreenLoadedSent = false;
	bool bReadyToTravelSent = false;

	FTimerHandle TravelCheckTimerHandle;
};
