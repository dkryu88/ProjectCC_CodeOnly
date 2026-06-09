// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/Match_Event_Meteor.h"
#include "Object/Map/MapObjects_Meteor.h"
#include "MapConstructor.h"

void AMatch_Event_Meteor::StartEvent_Implementation(AMapConstructor* map, APlayMode_Match* matchMode, float duration)
{
	Super::StartEvent_Implementation(map, matchMode, duration);

	if (!MapObjects_Meteor || !NowMap) return;

	NowDuration = EventDuration;
	GetWorldTimerManager().SetTimer(MeteorTimerHandle, this, &AMatch_Event_Meteor::SpawnMeteor, SpawnInterval, true);
}

void AMatch_Event_Meteor::StopEvent_Implementation()
{
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
		TArray<FIntVector> Candidates = NowMap->FloorBlocksData;
		int32 index = FMath::RandRange(0, Candidates.Num() - 1);

		float BS = NowMap->BlockSize; // 블록 크기 (월드 단위)

		FVector SpawnLocation = NowMap->GridToWorldCenter(Candidates[index].X, Candidates[index].Y, Candidates[index].Z);
		SpawnLocation.Z += BS * 30.f; // 지형 위 10칸 높이에서 낙하 시작

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FRotator SpawnRotation = FRotator(FMath::RandRange(-30.f, 30.f), FMath::RandRange(-180.f, 180.f), FMath::RandRange(-30.f, 30.f));

		AMapObjects_Meteor* Meteor = GetWorld()->SpawnActor<AMapObjects_Meteor>(MapObjects_Meteor, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
		if (Meteor) {
			Meteor->NowMap = NowMap;          // 맵 참조 전달
			Meteor->ApplyAdditionalSetting(); // 물리·충돌 설정 및 낙하 시작
		}
	}
}