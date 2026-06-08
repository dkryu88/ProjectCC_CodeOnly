// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Match_Event.h"
#include "Match_Event_KillPlaneWave.generated.h"

/**
 * 
 */

class AKillPlane;

UCLASS()
class PROJECTCC_API AMatch_Event_KillPlaneWave : public AMatch_Event
{
	GENERATED_BODY()
	
public:
	AMatch_Event_KillPlaneWave();

	virtual void StartEvent_Implementation(AMapConstructor* map, APlayMode_Match* matchMode, float duration) override;
	virtual void StopEvent_Implementation() override;
	virtual void Tick(float DeltaTime) override;

protected:
	AKillPlane* FindMainKillPlane() const;

	UPROPERTY()
	TObjectPtr<AKillPlane> TargetKillPlane;

	UPROPERTY()
	FVector OriginalLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category="KillPlane Event")
	float ReturnKillPlaneDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="KillPlane Event")
	FName MainKillPlaneTag = TEXT("MainKillPlane");

	//블록단위 KillPlane 이동 높이
	UPROPERTY(EditDefaultsOnly, Category="KillPlane Event")
	float MoveHeightBlockUnit = 1.f;

	//KillPlane 사이클
	UPROPERTY(EditDefaultsOnly, Category="KillPlane Event")
	float MoveCycleTime = 10.f;

	float ElapsedTime = 0.f;

	FVector ReturnStartLocation = FVector::ZeroVector;
	bool bReturningToOriginal = false;
	float ReturningElapsedTime = 0.f;
};
