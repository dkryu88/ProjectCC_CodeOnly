// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_CoinBag.h"
#include "Player_Character.h"
#include "Coin.h"
#include "MapConstructor.h"
#include "Components/ShapeComponent.h"

AObjects_CoinBag::AObjects_CoinBag(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bReplicates = true;
}

void AObjects_CoinBag::BeginPlay()
{
	Super::BeginPlay();

	if (PhysicsCollider)
	{
		PhysicsCollider->BodyInstance.bLockXRotation = true;
		PhysicsCollider->BodyInstance.bLockYRotation = true;
		PhysicsCollider->BodyInstance.bLockZRotation = true;
	}
}

void AObjects_CoinBag::Func_AttackedByPlayer_Implementation(APlayer_Character* Player) {
	Super::Func_AttackedByPlayer_Implementation(Player);
	if (HasAuthority()) {
		SpawnCoin();
	}
}

void AObjects_CoinBag::Func_Destroy_Implementation() {
	Super::Func_Destroy_Implementation();
	if (HasAuthority()) {
		//코인 3개 소환
		for (int32 i = 0; i < 3; ++i) {
			SpawnCoin();
		}
	}
}

void AObjects_CoinBag::ApplyAdditionalSetting()
{
	if (!PhysicsCollider || !HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	PhysicsCollider->SetSimulatePhysics(true);
	PhysicsCollider->SetEnableGravity(true);

	//밀렸을 때 쉽게 밀리지 않도록 Damping 증가
	PhysicsCollider->SetLinearDamping(0.85f);
	//질량 증가
	PhysicsCollider->SetMassOverrideInKg(NAME_None, 100.f, true);
	//무게 중심을 아래로 이동
	PhysicsCollider->SetCenterOfMass(FVector(0.f, 0.f, -25.f), NAME_None);
	//회전 관성 증가
	PhysicsCollider->BodyInstance.InertiaTensorScale = FVector(2.f, 2.f, 1.f);
	PhysicsCollider->BodyInstance.UpdateMassProperties();
}

void AObjects_CoinBag::SpawnCoin() {
	if (!HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	TSubclassOf<ACoin> SelectedCoinClass = CoinClass;
	if (MidCoinClass && FMath::RandRange(1, 100) < 20) {
		SelectedCoinClass = MidCoinClass;
	}
	if (!SelectedCoinClass) return;

	// 목적지 탐색
	TArray<FVector> SafeBlockLocations;
	CollectNearbySafeBlocksFromMap(SafeBlockLocations, SearchRadius, SearchHeight);

	// 스폰 시작 위치 지정
	FVector ObjectLocation = GetActorLocation();
	float SpawnGridOffsetZ = 50.f;
	if (PhysicsCollider) {
		SpawnGridOffsetZ = PhysicsCollider->Bounds.BoxExtent.Z + 15.f;	// 여유공간
	}
	FVector CoinStartLocation = ObjectLocation + FVector(FMath::FRandRange(-5.f, 5.f), FMath::FRandRange(-5.f, 5.f), SpawnGridOffsetZ);

	// 타겟 목적지 지점
	FVector CoinTargetLocation;
	if (SafeBlockLocations.Num() > 0) {
		int32 RandIndex = FMath::RandRange(0, SafeBlockLocations.Num() - 1);
		CoinTargetLocation = SafeBlockLocations[RandIndex];

		// 블록 정중앙이 아닌 랜덤 오프셋
		FVector2D Offset2D = FMath::RandPointInCircle(20.f);
		CoinTargetLocation += FVector(Offset2D.X, Offset2D.Y, 0.f);
	}
	else {
		// 맵 범우를 벗어난 예외 상황 시 제자리 주변으로 떨어짐
		FVector2D FallbackOffset = FMath::RandPointInCircle(80.f);
		CoinTargetLocation = ObjectLocation + FVector(FallbackOffset.X, FallbackOffset.Y, -20.f);
	}

	// 스폰 설정
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = nullptr;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 코인 생성
	ACoin* SpawnedCoin = World->SpawnActor<ACoin>(SelectedCoinClass, CoinStartLocation, FRotator::ZeroRotator, SpawnParams);

	if (SpawnedCoin) {
		// 코인이 스폰 직후 주머니와 부딪혀 튕겨 나가는 현상 방지
		if (UPrimitiveComponent* CoinComp = SpawnedCoin->GetObjectPhysicsCollider()) {
			CoinComp->IgnoreActorWhenMoving(this, true);
		}
		// Coin.cpp의 Multicast 발사 함수 호출
		SpawnedCoin->LaunchToTargetLocation(CoinStartLocation, CoinTargetLocation);
		SpawnedCoin->ForceNetUpdate();	// 동기화
	}
}

bool AObjects_CoinBag::CollectNearbySafeBlocksFromMap(TArray<FVector>& SafeBlockLocations, int32 instanceSearchRadius, int32 instanceSearchHeight) {
	SafeBlockLocations.Reset();

	if (!NowMap) return false;

	int32 CenterX = 0, CenterY = 0, CenterZ = 0;

	// 오브젝트 바닥을 기준으로 맵 탐색
	float BottomOffset = PhysicsCollider ? PhysicsCollider->Bounds.BoxExtent.Z : 0.f;
	bool bGridFound = NowMap->WorldToMapGrid(GetActorLocation() - FVector(0.f, 0.f, BottomOffset), CenterX, CenterY, CenterZ);

	if (!bGridFound) return false;

	// 맵 블록 검색 로직
	for (int32 GridOffsetZ = instanceSearchHeight; GridOffsetZ >= -instanceSearchHeight; --GridOffsetZ) {
		for (int32 GridOffsetX = -instanceSearchRadius; GridOffsetX <= instanceSearchRadius; ++GridOffsetX) {
			for (int32 GridOffsetY = -instanceSearchRadius; GridOffsetY <= instanceSearchRadius; ++GridOffsetY) {
				int32 CheckX = CenterX + GridOffsetX;
				int32 CheckY = CenterY + GridOffsetY;
				int32 CheckZ = CenterZ + GridOffsetZ;

				// 맵의 유효한 상단 블록만 허용
				if (!NowMap->IsValidGrid(CheckX, CheckY, CheckZ)) continue;
				if (!NowMap->IsNormalBlock(CheckX, CheckY, CheckZ) || !NowMap->IsTopBlock(CheckX, CheckY, CheckZ)) continue;

				FIntVector CheckLocation(CheckX, CheckY, CheckZ);
				if (!NowMap->IsEmptyOnFloorBlock(CheckLocation, {})) continue;

				// 코인주머니 자신의 위치 제외
				if (CheckX == CenterX && CheckY == CenterY && CheckZ == CenterZ) continue;

				FVector BlockWorld = NowMap->GridToWorldCenter(CheckX, CheckY, CheckZ);
				BlockWorld.Z += 1.f;
				SafeBlockLocations.Add(BlockWorld);
			}
		}
	}

	// 가까운 순으로 정렬하여 너무 멀리 날아가지 않도록 함
	if (SafeBlockLocations.Num() > 0) {
		FVector ObjectLocation = GetActorLocation();
		SafeBlockLocations.Sort([ObjectLocation](const FVector& A, const FVector& B) {
			return FVector::DistSquared2D(ObjectLocation, A) < FVector::DistSquared2D(ObjectLocation, B);
			});
	}

	return SafeBlockLocations.Num() > 0;
}