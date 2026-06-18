// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/Match_Event_RandomTeleport.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "PlayerTransformationComponent.h"
#include "PlayerTransformationDataAsset.h"
#include "PlayerConditionDataAsset.h"
#include "MapConstructor.h"
#include "GameFramework/PlayerController.h"

void AMatch_Event_RandomTeleport::StartEvent_Implementation(AMapConstructor* Map, APlayMode_Match* InMatchMode, float duration)
{
	Super::StartEvent_Implementation(Map, InMatchMode, duration);
	if (!NowMap) return;

	TeleportAllPlayers();

	// TeleportInterval마다 TeleportAllPlayers 반복 호출
	GetWorldTimerManager().SetTimer(TeleportTimerHandle, this, &AMatch_Event_RandomTeleport::TeleportAllPlayers, TeleportInterval, true);
}

void AMatch_Event_RandomTeleport::StopEvent_Implementation()
{
	GetWorldTimerManager().ClearTimer(TeleportTimerHandle);
	
	Super::StopEvent_Implementation();
}

void AMatch_Event_RandomTeleport::TeleportAllPlayers()
{
	// 서버에 접속된 모든 PlayerController 순회
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT)
	{
		APlayerController* PC = IT->Get();
		if (!PC) continue;               
		APlayer_Character* Player = Cast<APlayer_Character>(PC->GetPawn());
		if (!Player || Player->IsOut()) continue;

		FVector NewLocation;
		if (FindRandomLocation(NewLocation)) {
			Player->SetActorLocation(NewLocation);
			//텔레포트 후 일시적으로 입력 방지
			Player->AddInputBlockController(FName("SpeedUpEvent"), true, false, true, false);
			FTimerHandle BlockTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(BlockTimerHandle, [Player]() {
				if (IsValid(Player)) {
					Player->RemoveInputBlockController(FName("SpeedUpEvent"));
				}
				
			}, 2.f, false);
			//텔레포트 후 플레이어에게 일시적인 무적 효과 부여
			if (Player->ConditionComp) {
				Player->ConditionComp->ApplyCondition(InvincibilityDataAsset, Player, 2.f);
			}
			//텔레포트 후 플레이어에게 은신 부여
			if (Player->TransformationComp){
				Player->TransformationComp->StartTransformation(StealthTransformationData, Player, 10.f);
			}
		}	
	}
}

bool AMatch_Event_RandomTeleport::FindRandomLocation(FVector& OutLocation)
{
	if (!NowMap) return false;

	NowMap->GetAllFloorBlockLocation();
	float BS = NowMap->BlockSize;
	//맵 내의 안전 블록 위치를 받아서 그 중 랜덤한 곳을 타겟으로 설정
	TArray<FIntVector> Candidates = NowMap->FloorBlocksData;
	if (Candidates.Num() <= 0) return false;

	int32 index = FMath::RandRange(0, Candidates.Num() - 1);

	FIntVector TargetGrid = Candidates[index];

	//최종 위치를 Target 위치로 지정
	OutLocation = NowMap->GridToWorldCenter(TargetGrid.X, TargetGrid.Y, TargetGrid.Z);
	OutLocation.Z += NowMap->BlockSize;

	return true;
}
