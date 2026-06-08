// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/Match_Event_KillPlaneWave.h"
#include "KillPlane.h"
#include "MapConstructor.h"
#include "PlayMode_Match.h"
#include "Kismet/GameplayStatics.h"

AMatch_Event_KillPlaneWave::AMatch_Event_KillPlaneWave()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.SetTickFunctionEnable(false);
}

void AMatch_Event_KillPlaneWave::StartEvent_Implementation(AMapConstructor* map, APlayMode_Match* matchMode, float duration)
{
	Super::StartEvent_Implementation(map, matchMode, duration);

	if (!HasAuthority()) return;
	if (!NowMap) return;

	TargetKillPlane = FindMainKillPlane();

	if (!TargetKillPlane) {
		StopEvent();
		return;
	}

	NowMap->GetAllFloorBlockLocation(FMath::FloorToInt32(MoveHeightBlockUnit));
	
	OriginalLocation = TargetKillPlane->GetActorLocation();
	ElapsedTime = 0.f;

	if (MatchMode) {
		MatchMode->SetUseRespawnPointOver(true);
	}
	SetActorTickEnabled(true);
}

void AMatch_Event_KillPlaneWave::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority()) return;
	if (!TargetKillPlane || !NowMap) return;

	//이벤트 종료 시 원위치로 이동 (순간이동 x)
	if (bReturningToOriginal) {
		ReturningElapsedTime += DeltaTime;

		float SafeReturnDuration = FMath::Max(ReturnKillPlaneDuration, 0.01f);
		float ReturningRateProperty = FMath::Clamp(ReturningElapsedTime / SafeReturnDuration, 0.f, 1.f);
	
		float SmoothRateProperty = FMath::SmoothStep(0.f, 1.f, ReturningRateProperty);

		FVector NewLocation = FMath::Lerp(ReturnStartLocation, OriginalLocation, SmoothRateProperty);

		TargetKillPlane->SetActorLocation(NewLocation);

		if (ReturningRateProperty >= 1.f) {
			TargetKillPlane->SetActorLocation(OriginalLocation);

			bReturningToOriginal = false;
			SetActorTickEnabled(false);

			if (MatchMode) {
				MatchMode->SetUseRespawnPointOver(false);
			}

			Super::StopEvent_Implementation();
		}
	}

	else {
		ElapsedTime += DeltaTime;

		float BlockSize = NowMap->BlockSize;
		float MoveHeight = BlockSize * MoveHeightBlockUnit;
		float CycleTime = FMath::Max(MoveCycleTime, 0.1f);

		float HeightProperty = (FMath::Sin((ElapsedTime / CycleTime) * PI * 2.f) + 1.f) * 0.5f;

		FVector NewLocation = OriginalLocation;
		NewLocation.Z += MoveHeight * HeightProperty;

		TargetKillPlane->SetActorLocation(NewLocation);
	}
	
}

void AMatch_Event_KillPlaneWave::StopEvent_Implementation()
{
	if (!HasAuthority()) return;

	if (!TargetKillPlane) {
		if (MatchMode) {
			MatchMode->SetUseRespawnPointOver(false);
		}
		Super::StopEvent_Implementation();
		return;
	}

	if (bReturningToOriginal) return;

	NowMap->GetAllFloorBlockLocation();
	ReturnStartLocation = TargetKillPlane->GetActorLocation();
	ReturningElapsedTime = 0.f;
	bReturningToOriginal = true;

	SetActorTickEnabled(true);
}


AKillPlane* AMatch_Event_KillPlaneWave::FindMainKillPlane() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AKillPlane::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors) {
		if (!Actor) continue;

		if (Actor->ActorHasTag(MainKillPlaneTag)) {
			AKillPlane* MainKillPlane = Cast<AKillPlane>(Actor);
			return MainKillPlane;
		}
	}

	return nullptr;
}
