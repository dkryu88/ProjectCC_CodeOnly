// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Match_Event.generated.h"

class AMapConstructor;
class APlayMode_Match;

UCLASS()
class PROJECTCC_API AMatch_Event : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMatch_Event();
	
	UFUNCTION(BlueprintNativeEvent)
	void StartEvent(AMapConstructor* map, APlayMode_Match* matchMode, float duration);

	UFUNCTION(BlueprintNativeEvent)
	void StopEvent();

	UFUNCTION(BlueprintCallable)
	bool IsEventRunning() const { return bEventRunning; }

	UFUNCTION(BlueprintNativeEvent)
	void ApplyEventToPlayer(APlayer_Character* Player);

	virtual void ApplyEventToPlayer_Implementation(APlayer_Character* Player);
	virtual void StartEvent_Implementation(AMapConstructor* map, APlayMode_Match* matchMode, float duration);
	virtual void StopEvent_Implementation();
protected:
	UPROPERTY()
	TObjectPtr<AMapConstructor> NowMap;

	UPROPERTY()
	TObjectPtr<APlayMode_Match> MatchMode;

	UPROPERTY(VisibleAnywhere)
	bool bEventRunning = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MatchEvent")
	float EventDuration = 30.f;

	FTimerHandle EventDurationTimerHandle;
};
