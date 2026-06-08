// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Result_PlayerController.generated.h"

/**
 * 
 */
class UPlayer_ResultWidget;

UCLASS()
class PROJECTCC_API AResult_PlayerController : public APlayerController
{
	GENERATED_BODY()


protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void EnableEixtToTitle();
	void OnPressedExitToTitle();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayer_ResultWidget> Player_ResultWidget;

	UPROPERTY()
	TObjectPtr<UPlayer_ResultWidget> ResultWidget;

	UPROPERTY()
	bool bCanExitMatch = false;

	FTimerHandle EnableExitTimerHandle;
};
