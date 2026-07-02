// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/Match_Event_Meteor.h"
#include "Object/Map/MapObjects_Meteor.h"
#include "MapConstructor.h"

void AMatch_Event_Meteor::StartEvent_Implementation(AMapConstructor* map, APlayMode_Match* matchMode, float duration)
{
	Super::StartEvent_Implementation(map, matchMode, duration);

	if (!MapObjects_Meteor || !NowMap) return;

	bMeteorEventRunning = true;

	NowDuration = EventDuration;
	GetWorldTimerManager().SetTimer(MeteorTimerHandle, this, &AMatch_Event_Meteor::SpawnMeteor, SpawnInterval, true);
}

void AMatch_Event_Meteor::StopEvent_Implementation()
{
	bMeteorEventRunning = false;

	GetWorldTimerManager().ClearTimer(MeteorTimerHandle);

	Super::StopEvent_Implementation();
}

void AMatch_Event_Meteor::SpawnMeteor() {
	if (!NowMap || !MapObjects_Meteor) return;
	if (NowMap->FloorBlocksData.Num() <= 0) return;
	if (NowDuration <= 3.f) return;

	NowDuration -= SpawnInterval;

	for (int32 i = 0; i < SpawnCountPerInterval; ++i)
	{
		TArray<FIntVector>& Candidates = NowMap->FloorBlocksData;
		int32 index = FMath::RandRange(0, Candidates.Num() - 1);
		FIntVector TargetGrid = Candidates[index];

		FVector TargetTopLocation = NowMap->GridToWorldCenter(TargetGrid.X, TargetGrid.Y, TargetGrid.Z);

		SpawnMeteorWarningActor(TargetTopLocation);

		FTimerHandle DelaySpawnHandle;
		GetWorldTimerManager().SetTimer(DelaySpawnHandle, FTimerDelegate::CreateWeakLambda(this, [this, TargetTopLocation]() {
			if (!bMeteorEventRunning) return;
			SpawnMeteorAtTargetTop(TargetTopLocation);
		}), WarningDelay, false);
	}
}

void AMatch_Event_Meteor::SpawnMeteorWarningActor(const FVector& TargetTopLocation)
{
	if (!NowMap || !MeteorWarningClass) return;
	if (!bMeteorEventRunning) return;

	float BS = NowMap->BlockSize;

	FVector WarningLocation = TargetTopLocation;
	WarningLocation.Z += WarningZOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* WarningActor = GetWorld()->SpawnActor<AActor>(MeteorWarningClass, WarningLocation, FRotator::ZeroRotator, SpawnParams);

	if (WarningActor)
	{
		float PlaneBaseSize = 100.f;
		float ScaleXY = BS / PlaneBaseSize;

		WarningActor->SetActorScale3D(FVector(ScaleXY, ScaleXY, 1.f));
		WarningActor->SetLifeSpan(WarningDelay + 0.05f);
	}
}

void AMatch_Event_Meteor::SpawnMeteorAtTargetTop(const FVector& TargetTopLocation)
{
	if (!NowMap || !MapObjects_Meteor) return;

	const float BS = NowMap->BlockSize;

	FVector SpawnLocation = TargetTopLocation;
	SpawnLocation.Z += BS * 30.f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FRotator SpawnRotation(FMath::RandRange(-30.f, 30.f), FMath::RandRange(-180.f, 180.f), FMath::RandRange(-30.f, 30.f));

	AMapObjects_Meteor* Meteor = GetWorld()->SpawnActor<AMapObjects_Meteor>(MapObjects_Meteor, SpawnLocation, SpawnRotation, SpawnParams);

	if (Meteor)
	{
		Meteor->NowMap = NowMap;
		Meteor->ApplyAdditionalSetting();
	}
}
