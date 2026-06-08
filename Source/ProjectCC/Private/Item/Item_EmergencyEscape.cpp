// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_EmergencyEscape.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "MapConstructor.h"
#include "PlayMode_Match.h"
#include "Weapon.h"
#include "Kismet/GameplayStatics.h"

bool AItem_EmergencyEscape::UseEffect_Implementation(APlayer_Character* Player)
{
	if (!HasAuthority() || !Player || !InvincibilityDataAsset) return false;

	UWorld* World = GetWorld();
	APlayMode_Match* MatchMode = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
	AMapConstructor* Map = MatchMode ? MatchMode->GetCurrentMap() : nullptr;

	TArray<AActor*> AllPlayers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),APlayer_Character::StaticClass(), AllPlayers);

	//위치 교환 대상 선정 (탈락되었거나 본인은 제외)
	TArray<APlayer_Character*> ValidTargets;
	for (AActor* Actor : AllPlayers) {
		APlayer_Character* Target = Cast<APlayer_Character>(Actor);
		if (Target && Target != Player && !Target->IsOut()) {
			ValidTargets.Add(Target);
		}
	}

	if (ValidTargets.Num() > 0) {
		int32 RandomIndex = FMath::RandRange(0, ValidTargets.Num() - 1);
		APlayer_Character* TargetPlayer = ValidTargets[RandomIndex];

		FVector MyLocation = Player->GetActorLocation();
		FVector TargetLocation = TargetPlayer->GetActorLocation();

		//대상 플레이어가 도착할 안전 블럭 위치를 획득
		FVector SafeDestinationForTarget = GetSafeSwapLocation(MyLocation, Map, Player, TargetPlayer);

		Player->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
		TargetPlayer->SetActorLocation(SafeDestinationForTarget, false, nullptr, ETeleportType::TeleportPhysics);

		Player->AddInputBlockController(FName("EmergencyEscape"), true, false, true, false);
		TargetPlayer->AddInputBlockController(FName("EmergencyEscape"), true, false, true, false);

		FTimerHandle BlockTimerHandle;
		World->GetTimerManager().SetTimer(BlockTimerHandle, [Player, TargetPlayer]() {
			if (IsValid(Player)) {
				Player->RemoveInputBlockController(FName("EmergencyEscape"));
			}
			if (IsValid(TargetPlayer)) {
				TargetPlayer->RemoveInputBlockController(FName("EmergencyEscape"));
			}
		}, InputBlockDuration, false);

		if (Player->ConditionComp) {
			Player->ConditionComp->ApplyCondition(InvincibilityDataAsset, Player, InvincibilityDuration);
		}

		return true;
	}

	return false;
}

FVector AItem_EmergencyEscape::GetSafeSwapLocation(const FVector& TargetLocation, AMapConstructor* Map, AActor* IgnoreItemUser, AActor* IgnoreTarget)
{
	if (!Map) return TargetLocation;

	TArray<AActor*> IgnoreList;
	IgnoreList.Add(IgnoreItemUser);
	IgnoreList.Add(IgnoreTarget);
	IgnoreList.Add(this);

	APlayer_Character* User = Cast<APlayer_Character>(IgnoreItemUser);
	APlayer_Character* Target = Cast<APlayer_Character>(IgnoreTarget);
	if (User && User->NowWeapon) IgnoreList.Add(User->NowWeapon);
	if (User && User->NowItem) IgnoreList.Add(User->NowItem);
	if (User && User->NowObjects) IgnoreList.Add(User->NowObjects);
	if (User && User->NowSupport) IgnoreList.Add(User->NowSupport);
	if (Target && Target->NowWeapon) IgnoreList.Add(Target->NowWeapon);
	if (Target && Target->NowItem) IgnoreList.Add(Target->NowItem);
	if (Target && Target->NowObjects) IgnoreList.Add(Target->NowObjects);
	if (Target && Target->NowSupport) IgnoreList.Add(Target->NowSupport);

	//대상 목적지가 안전한 위치인지 확인
	float HalfSize = Map->BlockSize * 0.5f;
	int32 X, Y, Z;
	if (Map->WorldToGridTopBlock(TargetLocation, X, Y, Z)) {
		if (Map->IsNormalBlock(X, Y, Z) && Map->IsTopBlock(X, Y, Z)) {
			FIntVector Grid(X, Y, Z);
			if (Map->IsEmptyOnFloorBlock(Grid, IgnoreList)) {
				return Map->GridToWorldCenter(X, Y, Z) + FVector(0.f, 0.f, HalfSize);
			}
		}
	}

	//대상 목적지가 안전한 위치가 아니라면 근처 안전 위치 탐색
	FVector BestLocation = TargetLocation;
	float MinDistSq = FMath::Square(MaxSafeSearchDistance);
	bool bFound = false;

	for (const FIntVector& Grid : Map->FloorBlocksData) {
		FVector BlockPosition = Map->GridToWorldCenter(Grid.X, Grid.Y, Grid.Z);
		float DistSq = FVector::DistSquared(TargetLocation, BlockPosition);

		if (DistSq < MinDistSq) {
			FIntVector TempGrid = Grid;
			if (Map->IsEmptyOnFloorBlock(TempGrid, IgnoreList)) {
				MinDistSq = DistSq;
				BestLocation = BlockPosition + FVector(0.f, 0.f, HalfSize);
				bFound = true;
			}
		}
	}

	return BestLocation;
}
