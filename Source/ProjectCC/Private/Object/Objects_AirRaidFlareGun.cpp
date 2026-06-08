// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_AirRaidFlareGun.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "ObjectsDataAsset.h"
#include "Engine/OverlapResult.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"

#include "DrawDebugHelpers.h"

AObjects_AirRaidFlareGun::AObjects_AirRaidFlareGun(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bReplicates = true;
	SetReplicateMovement(true);
}

void AObjects_AirRaidFlareGun::ApplyAdditionalSetting()
{
	Super::ApplyAdditionalSetting();

	if (!HasAuthority()) return;
	if (!PhysicsCollider) return;

	PhysicsCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Ignore);

	PhysicsCollider->SetSimulatePhysics(true);
	PhysicsCollider->SetEnableGravity(false);
	PhysicsCollider->WakeAllRigidBodies();
}

void AObjects_AirRaidFlareGun::InitAirRaid(APlayer_Character* ownPlayer, const FVector& AttackForward)
{
	if (!HasAuthority()) return;

	OwnPlayer = ownPlayer;

	if (!NowMap) {
		NowMap = OwnPlayer->NowMap;
		if (!NowMap) {
			Destroy();
			return;
		}
	}

	ForwardGridDirection = NowMap->WorldDirectionToGridDirection(AttackForward);
	RightGridDirection = NowMap->GetRightGridDirectionFromForward(ForwardGridDirection);

	CapturedForward = NowMap->GridDirectionToWorldDirection(ForwardGridDirection);
	CapturedYaw = NowMap->GridDirectionYaw(ForwardGridDirection);
}

void AObjects_AirRaidFlareGun::BeginAirRaidFlare()
{
	if (!HasAuthority()) return;

	if (!OwnPlayer) {
		Destroy();
		return;
	}

	if (!NowMap) {
		NowMap = OwnPlayer->NowMap;
		if (!NowMap) {
			Destroy();
			return;
		}
	}

	FlareSpawnLocation = StartLocation;

	if (PhysicsCollider && ObjectsData) {
		PhysicsCollider->SetPhysicsLinearVelocity(FVector(0.f, 0.f, ObjectsData->Move_Speed));
		PhysicsCollider->WakeAllRigidBodies();
	}

	float SafeLifeSpan = ExplosionDelay + RowCount * RowInterval + 3.f;
	SetLifeSpan(SafeLifeSpan);

	GetWorldTimerManager().ClearTimer(ExplosionDelayTimerhandle);
	GetWorldTimerManager().SetTimer(ExplosionDelayTimerhandle, this, &AObjects_AirRaidFlareGun::StartAirRaid, ExplosionDelay, false);
}

bool AObjects_AirRaidFlareGun::BuildAirRaidStartLocation()
{
	if (!NowMap) return false;
	float BS = NowMap->BlockSize;

	int32 SpawnGridX = 0;
	int32 SpawnGridY = 0;
	int32 SpawnGridZ = 0;

	NowMap->WorldToMapGrid(FlareSpawnLocation, SpawnGridX, SpawnGridY, SpawnGridZ);
	AirRaidStartGrid = FIntPoint(SpawnGridX + ForwardGridDirection.X * 2, SpawnGridY + ForwardGridDirection.Y * 2);

	return true;
}

void AObjects_AirRaidFlareGun::StartAirRaid()
{
	if (!HasAuthority()) return;

	if (!OwnPlayer || !NowMap) {
		Destroy();
		return;
	}

	CurrentRow = 0;

	if (!BuildAirRaidStartLocation()) {
		Destroy();
		return;
	}

	ExplodeNextRow();
}

void AObjects_AirRaidFlareGun::ExplodeNextRow()
{
	if (!HasAuthority()) return;

	if (!OwnPlayer || !NowMap) {
		Destroy();
		return;
	}

	float BS = NowMap->BlockSize;
	FRotator ExplosionRotation(0.f, CapturedYaw, 0.f);
	FQuat ExplosionQuat = ExplosionRotation.Quaternion();

	FCollisionObjectQueryParams PawnQueryParams;
	PawnQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TSet<APlayer_Character*> DamagedPlayersThisRow;
	TSet<FIntVector> ProcessedCells;
	
	//각 Row의 폭격 중심 위치 (RangeInBlocks가 5라면 중심은 2가 되며 <왼쪽2(ColMin), 가운데1, 오른쪽2(ColMax)>를 폭격)
	int32 CenterOffset = RangeInBlocks / 2;
	//폭격 Col 범위 최소/최대 값 <- 이 범위 밖의 블록은 폭격 x
	float ColMin = -CenterOffset;
	float ColMax = RangeInBlocks - (CenterOffset + 1);

	//현재 폭격 Row의 중심 좌표 (시작점이 <20,20>이고 방향이 <1,1>이며 CurrentRow가 3이면 3번째(현재) 폭격 줄의 중심은 <20 + 1 * 3, 20 + 1 * 3>
	int32 RowCenterX = AirRaidStartGrid.X + ForwardGridDirection.X * CurrentRow;
	int32 RowCenterY = AirRaidStartGrid.Y + ForwardGridDirection.Y * CurrentRow;

	//Forward/Right 기저 행렬 Determinant
	//Forward 벡터와 Right 벡터가 만드는 영역의 넓이 비율
	//Start가 <0,0>이고 Forward가 <1,1>, Right가 <-1,1>이면 (1 * 1) - (1 * -1) = 2
	float Determinant = ForwardGridDirection.X * RightGridDirection.Y - ForwardGridDirection.Y * RightGridDirection.X;

	//Determinant가 0이면 Forward와 Right가 같은 선 위에 있다는 뜻(Error)이므로 Return
	if (FMath::Abs(Determinant) <= 0) {
		Destroy();
		return;
	}

	//대각선 사이 빈칸도 포함하기 위해 현재 줄 기준으로 바로 근처 Row까지 확인
	float RowMin = CurrentRow - 0.5f;
	float RowMax = CurrentRow + 0.5f;

	//전체 폭격 Row 크기
	float GlobalRowMin = 0.f;
	float GlobalRowMax = RowCount - 1.f;

	//후보 검색 범위 (실제 포함 여부는 LocalRow/LocalCol로 재검사)
	int32 SearchRadius = RangeInBlocks + 2;

	//현재 대상 줄 중심에서 SearchRadius 내의 주변 Grid 좌표를 전부 검사 (폭격 범위 내면 폭격 대상 범위에 추가)
	for (int32 X = RowCenterX - SearchRadius; X <= RowCenterX + SearchRadius; ++X) {
		for (int32 Y = RowCenterY - SearchRadius; Y <= RowCenterY + SearchRadius; ++Y) {
			if (X < 0 || X >= NowMap->Max_X || Y < 0 || Y > NowMap->Max_Y) continue;

			//폭격 시작점 기준으로 현재 검사중인 칸이 얼마나 떨어져있는지 확인
			//Dx = Forward.X * LocalRow + Right,X * LocalCol과 같음
			//Dy = Forward.Y * LocalRow + Right.Y * LocalCol과 같음
			float DX = X - AirRaidStartGrid.X;
			float DY = Y - AirRaidStartGrid.Y;

			//현재 검사중인 칸이 Forward/Right 방향으로 몇 번째 줄에 있는지 확인
			float LocalRow = (DX * RightGridDirection.Y - DY * RightGridDirection.X) / Determinant;
			float LocalCol = (ForwardGridDirection.X * DY - ForwardGridDirection.Y * DX) / Determinant;

			//전체 폭격 범위 밖이면 제외
			if (LocalRow < GlobalRowMin || LocalRow > GlobalRowMax) continue;
			//현재 Row의 검사 범위 밖이면 제외
			if (LocalRow <= RowMin || LocalRow > RowMax) continue;
			//현재 폭격 범위(Col) 밖이면 제외
			if (LocalCol < ColMin || LocalCol > ColMax) continue;

			//최상단 블록이 없으면 제외
			int32 GridZ = -1;
			if (!NowMap->FindTopBlockZAtXY(X, Y, GridZ)) continue;
			if (GridZ == -1) continue;

			//같은 칸 중복 처리 방지
			FIntVector CellGrid(X, Y, GridZ);
			if (ProcessedCells.Contains(CellGrid)) continue;

			ProcessedCells.Add(CellGrid);
			FVector CellCenter = NowMap->GridToWorldCenter(X, Y, GridZ);

			TArray<FOverlapResult> Overlaps;
			GetWorld()->OverlapMultiByObjectType(Overlaps, CellCenter, ExplosionQuat, PawnQueryParams, FCollisionShape::MakeBox(FVector(BS * 0.5f, BS * 0.5f, BS * 2.f)));

			for (const FOverlapResult& Result : Overlaps) {
				APlayer_Character* Target = Cast<APlayer_Character>(Result.GetActor());

				if (!Target) continue;
				if (Target->IsOut()) continue;
				if (DamagedPlayersThisRow.Contains(Target)) continue;

				FHitResult LOSHit;
				FCollisionQueryParams LOSParams;
				LOSParams.AddIgnoredActor(this);
				LOSParams.AddIgnoredActor(Target);

				FVector LOSOrigin = CellCenter + FVector(0.f, 0.f, BS * 0.5f);
				bool bBlocked = GetWorld()->LineTraceSingleByChannel(LOSHit, LOSOrigin, Target->GetActorLocation(), ECC_WorldStatic, LOSParams);

				if (bBlocked) continue;

				Target->ApplyDamageInternal(ExplosionDamage, OwnPlayer, this, false, false, false);
				DamagedPlayersThisRow.Add(Target);
			}

			Multicast_PlayExplosionCell(CellCenter, ExplosionRotation);
		}
	}

	++CurrentRow;

	if (CurrentRow < RowCount) {
		GetWorldTimerManager().SetTimer(ExplosionIntervalTimerHandle, this, &AObjects_AirRaidFlareGun::ExplodeNextRow, RowInterval, false);
	}
	else Destroy();

	/*Code 난이도가 많이 어렵;;*/
}


void AObjects_AirRaidFlareGun::Multicast_PlayExplosionCell_Implementation(FVector Center, FRotator Rotation)
{
	const float BS = NowMap ? NowMap->BlockSize : 100.f;

	// 디버그 드로우도 블록 중앙에 맞춰서 그려줍니다.
	DrawDebugBox(GetWorld(), Center, FVector(BS * 0.5f, BS * 0.5f, BS * 0.5f), FQuat::Identity, FColor::Red, false, 3.f, 0, 3.f);
	DrawDebugSphere(GetWorld(), Center, 20.f, 12, FColor::Green, false, 3.0f);

	if (!ExplosionFX) return;

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionFX, Center, Rotation, FVector(0.1f));
}
