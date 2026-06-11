// Fill out your copyright notice in the Description page of Project Settings.


#include "Match_Event.h"
#include "MapConstructor.h"
#include "PlayMode_Match.h"

// Sets default values
AMatch_Event::AMatch_Event()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void AMatch_Event::StartEvent_Implementation(AMapConstructor* map, APlayMode_Match* matchMode, float duration)
{
	NowMap = map;
	MatchMode = matchMode;
	bEventRunning = true;
	EventDuration = duration;

	GetWorldTimerManager().ClearTimer(EventDurationTimerHandle);
	if (EventDuration > 0.f) {
		GetWorldTimerManager().SetTimer(EventDurationTimerHandle, this, &AMatch_Event::StopEvent, EventDuration, false);
	}
}

void AMatch_Event::StopEvent_Implementation()
{
	GetWorldTimerManager().ClearTimer(EventDurationTimerHandle);
	bEventRunning = false;
	Destroy();
}

void AMatch_Event::ApplyEventToPlayer_Implementation(APlayer_Character* Player) {}

