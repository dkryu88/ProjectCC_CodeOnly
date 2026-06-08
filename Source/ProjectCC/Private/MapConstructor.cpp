// Fill out your copyright notice in the Description page of Project Settings.


#include "MapConstructor.h"
#include "BlockType.h"
#include "KillPlane.h"
#include "Match_EventListDataAsset.h"
#include "Match_Event.h"
#include "Map/DamageBlock.h"
#include "Engine/OverlapResult.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Components/InstancedStaticMeshComponent.h"

// Sets default values
AMapConstructor::AMapConstructor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	//각 블럭의 Instance Map 생성 후 Root에 부착
	Normal_ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Normal_ISM"));
	Normal_ISM->SetupAttachment(Root);
	Transparency_ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Transparency_ISM"));
	Transparency_ISM->SetupAttachment(Root);
	//각 블럭의 Collision 설정
	Normal_ISM->SetCollisionProfileName(TEXT("BlockAll"));
	Transparency_ISM->SetCollisionProfileName(TEXT("BlockAll"));
	//노멀 블럭의 UV 데이터 설정
	Normal_ISM->SetNumCustomDataFloats(3);
	//Destroy 방지 액터로 지정
	Tags.Add(TEXT("NonDestroy"));
}

// Called when the game starts or when spawned
void AMapConstructor::BeginPlay()
{
	Super::BeginPlay();
	BuildMap();
	
}

// Called every frame
void AMapConstructor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//맵 데이터 초기화
void AMapConstructor::OnConstruction(const FTransform& Transform) {
	LoadFromDataAsset();
}

//맵 데이터 기반 블록 생성
void AMapConstructor::BuildMap() {
	Normal_ISM->ClearInstances();
	Transparency_ISM->ClearInstances();

	ClearDamageBlocks();

	bool bCanSpawnDamageActors = (GetWorld() && GetWorld()->IsGameWorld());

	for (int32 _x = 0; _x < Max_X; _x++) {
		for (int32 _y = 0; _y < Max_Y; _y++) {
			for (int32 _z = 0; _z < Max_Z; _z++) {
				FVector Location((_x + 0.5f) * BlockSize, (_y + 0.5f) * BlockSize, _z * BlockSize);
				Location += GetActorLocation();
				FTransform Transform(Location);
				switch (MapData[Index(_x, _y, _z)]) {
				case (EBlockType::Normal):
					AddNormalBlockWithRandomUV(Normal_ISM, Transform, _x, _y, _z);
					break;
				case (EBlockType::Transparency):
					Transparency_ISM->AddInstance(Transform);
					break;
				case(EBlockType::Damage):
					if (bCanSpawnDamageActors && DamageBlock) {
						FActorSpawnParameters SpawnParams;
						SpawnParams.Owner = this;
						SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

						ADamageBlock* SpawnedBlock = GetWorld()->SpawnActor<ADamageBlock>(DamageBlock, Location, FRotator::ZeroRotator, SpawnParams);

						if (SpawnedBlock) {
							SpawnedBlock->InitializeDamageBlock(BlockSize, this ,FIntVector(_x, _y, _z));
							Damage_ISM.Add(SpawnedBlock);
						}
					}
					break;
				default:
					break;
				}
				
			}
		}
	}
	//노멀블록 랜더 상태 갱신
	Normal_ISM->MarkRenderStateDirty();
}

//BP에서 맵 수정
void AMapConstructor::SetBlock(int32 _x, int32 _y, int32 _z, EBlockType Type) {
	if (_x < 0 || _x >= Max_X || _y < 0 || _y >= Max_Y || _z < 0 || _z >= Max_Z) {
		return;
	}
	MapData[Index(_x, _y, _z)] = Type;
	GetAllFloorBlockLocation();
	BuildMap();

	UE_LOG(LogTemp, Warning, TEXT("SetBlock Called : %d %d %d -> %d"), _x, _y, _z, (int32)Type);
}

//클릭 시 좌표 획득 및 변환
bool AMapConstructor::WorldToMapGrid(const FVector& WorldLocation, int32& outX, int32& outY, int32& outZ) {
	FVector MapLocation = WorldLocation - GetActorLocation();
	outX = FMath::FloorToInt(MapLocation.X / BlockSize);
	outY = FMath::FloorToInt(MapLocation.Y / BlockSize);
	outZ = FMath::FloorToInt(MapLocation.Z / BlockSize);

	return (outX >= 0 && outX < Max_X && outY >= 0 && outY < Max_Y && outZ >= 0 && outZ < Max_Z);
}

//월드 좌표를 맵 단위의 최상단 블록 좌표로 변환
bool AMapConstructor::WorldToGridTopBlock(const FVector& WorldLocation, int32& outX, int32& outY, int32& outZ) {
	FVector MapLocation = WorldLocation - GetActorLocation();
	outX = FMath::FloorToInt(MapLocation.X / BlockSize);
	outY = FMath::FloorToInt(MapLocation.Y / BlockSize);
	for (int32 i = Max_Z; i >= 0; --i) {
		if (IsTopBlock(outX, outY, i) && !IsEmptyBlock(outX, outY, i)) {
			outZ = i;
			return true;
		}
	}
	return false;
}

//블록 생성 위치 조정
bool AMapConstructor::HitToMapGrid(const FHitResult& HitLocation, bool bIsCreateBlock, int32& outx, int32& outy, int32& outz)
{
	UE_LOG(LogTemp, Warning, TEXT("Hit Actor : %s"),
		HitLocation.GetActor() ? *HitLocation.GetActor()->GetClass()->GetName() : TEXT("None"));

	if (BlockSize <= 0.f) return false;

	if (!bIsCreateBlock) {
		if (ADamageBlock* HitDamageBlock = Cast<ADamageBlock>(HitLocation.GetActor())) {
			FIntVector Grid = HitDamageBlock->GetGridLocation();
			outx = Grid.X;
			outy = Grid.Y;
			outz = Grid.Z;
			UE_LOG(LogTemp, Warning, TEXT("DamageBlock Remove Grid : %d %d %d"), outx, outy, outz);
			return IsValidGrid(outx, outy, outz);
		}
	}

	//클릭 위치 획득
	FVector HitPlace = HitLocation.ImpactPoint;
	//블럭위에 블럭 새로 생성 시, 클릭한 블럭의 노멀에 대해 생성 위치 계산
	FVector BlockNormal = HitLocation.ImpactNormal.GetSafeNormal();
	if (bIsCreateBlock) {
		HitPlace += (BlockNormal * (BlockSize * 0.5f));
	}
	else {
		HitPlace -= (BlockNormal * (BlockSize * 0.5f));
	}

	FVector LocalLocation = HitPlace - GetActorLocation();
	outx = FMath::FloorToInt(LocalLocation.X / BlockSize);
	outy = FMath::FloorToInt(LocalLocation.Y / BlockSize);
	outz = FMath::FloorToInt(LocalLocation.Z / BlockSize);
	UE_LOG(LogTemp, Warning, TEXT("loc = %f %f %f"), HitPlace.X, HitPlace.Y, HitPlace.Z);
	UE_LOG(LogTemp, Warning, TEXT("Grid = %d %d %d"), outx, outy, outz);
	return (outx >= 0 && outx < Max_X && outy >= 0 && outy < Max_Y && outz >= 0 && outz < Max_Z);
}

//맵을 데이터 에셋에 저장
void AMapConstructor::SaveToDataAsset() {
	if (!MapInfo) return;
	MapInfo->X = Max_X;
	MapInfo->Y = Max_Y;
	MapInfo->Z = Max_Z;
	MapInfo->BlockSize = BlockSize;
	MapInfo->MapData = MapData;
	GetAllFloorBlockLocation();
	MapInfo->FloorBlocksData = FloorBlocksData;
	UE_LOG(LogTemp, Warning, TEXT("Map Saved"));
}

//저장된 맵 불러오기
void AMapConstructor::LoadFromDataAsset() {
	if (!MapInfo) return;
	Max_X = MapInfo->X;
	Max_Y = MapInfo->Y;
	Max_Z = MapInfo->Z;
	BlockSize = MapInfo->BlockSize;

	int32 Size = Max_X * Max_Y * Max_Z;
	MapData = MapInfo->MapData;
	if (MapData.Num() != Size) {
		MapData.Init(EBlockType::Empty, Size);
		for (int32 _x = 0; _x < Max_X; _x++) {
			for (int32 _y = 0; _y < Max_Y; _y++) {
				if (_x >= 10 && _x < 40 && _y >= 10 && _y < 40)
				{
					MapData[Index(_x, _y, 0)] = EBlockType::Normal;
				}
			}
		}
		GetAllFloorBlockLocation();
	}
	else {
		FloorBlocksData = MapInfo->FloorBlocksData;

		if (FloorBlocksData.Num() == 0) {
			GetAllFloorBlockLocation();
			MapInfo->FloorBlocksData = FloorBlocksData;
		}
	}
	BuildMap();
	UE_LOG(LogTemp, Warning, TEXT("Map Loaded"));
}

//맵 데이터 파일 백업
void AMapConstructor::ExportMapToFile() {
	FString SaveFile;
	for (int32 i = 0; i < MapData.Num(); i++) {
		SaveFile += FString::FromInt((int32)MapData[i]);
		if (i < MapData.Num() - 1) {
			SaveFile += TEXT(" ");
		}
	}
	FString Directory = FPaths::ProjectDir() + TEXT("Content/DataAssets/Map/MapBackup/");
	IFileManager::Get().MakeDirectory(*Directory, true);
	FString File = Directory + FileName + TEXT(".txt");
	bool bSuccess = FFileHelper::SaveStringToFile(SaveFile, *File);
	UE_LOG(LogTemp, Warning, TEXT("Map Exported"));
}

//맵 데이터 파일 로드
void AMapConstructor::ImportMapFromFile() {
	FString Directory = FPaths::ProjectDir() + TEXT("Content/DataAssets/Map/MapBackup/");
	FString File = Directory + FileName + TEXT(".txt");
	FString LoadText;
	if (!FFileHelper::LoadFileToString(LoadText, *File)) return;
	
	TArray<FString> Lines;
	LoadText.ParseIntoArrayLines(Lines);
	if (Lines.Num() < 1) return;

	TArray<FString> MapDataInts;
	Lines[0].ParseIntoArray(MapDataInts, TEXT(" "), true);
	MapData.Empty();
	for (const FString& Data : MapDataInts) {
		MapData.Add((EBlockType)FCString::Atoi(*Data));
	}
	BuildMap();
	UE_LOG(LogTemp, Warning, TEXT("Map Imported"));
}

//맵에 존재하는 바닥 블럭(최상단 안전 블럭이면서 NormalType 블럭)들을 확인 및 저장 <MinSafeFloor가 0이면 전체 검색>
void AMapConstructor::GetAllFloorBlockLocation(int32 MinSafeFloor) {
	FloorBlocksData.Empty();

	for (int32 x = 0; x < Max_X; ++x) {
		for (int32 y = 0; y < Max_Y; ++y) {
			for (int32 z = 0; z < Max_Z; ++z) {
				if (IsNormalBlock(x, y, z) && IsTopBlock(x, y, z) && IsSafeBlock(FIntVector(x, y ,z))) {
					FloorBlocksData.Add(FIntVector(x, y, z));
				}
			}
		}
	}
}

bool AMapConstructor::IsValidGrid(int32 InX, int32 InY, int32 InZ)
{
	return (InX >= 0 && InX < Max_X && InY >= 0 && InY < Max_Y && InZ >= 0 && InZ < Max_Z);
}

bool AMapConstructor::IsNormalBlock(int32 InX, int32 InY, int32 InZ)
{
	if (!IsValidGrid(InX, InY, InZ)) {
		return false;
	}
	return MapData[Index(InX, InY, InZ)] == EBlockType::Normal; 
}
//좌표의 블럭이 최상단 블럭인지 확인(위에 다른 블럭이 없음)
bool AMapConstructor::IsTopBlock(int32 InX, int32 InY, int32 InZ)
{
	if (!IsValidGrid(InX, InY, InZ)) return false;
	if (IsEmptyBlock(InX, InY, InZ)) return false;
	//맵의 최상단인 경우 맵 위가 비어있는 것으로 취급 (거의 탐색할 일 없음)
	if (InZ == Max_Z - 1) return true;
	return IsEmptyBlock(InX, InY, InZ + 1);
}
bool AMapConstructor::IsSafeBlock(const FIntVector& Grid)
{
	if (!IsValidGrid(Grid.X, Grid.Y, Grid.Z)) return false;
	if (!IsNormalBlock(Grid.X, Grid.Y, Grid.Z)) return false;
	if (!IsTopBlock(Grid.X, Grid.Y, Grid.Z)) return false;
	if (!GetWorld()) return false;

	FVector FloorTop = GridToWorldCenter(Grid.X, Grid.Y, Grid.Z);
	FVector CheckCenter = FloorTop + FVector(0.f, 0.f, BlockSize * 0.5f);
	FCollisionShape CheckShape = FCollisionShape::MakeBox(FVector(BlockSize * 0.45f, BlockSize * 0.45f, BlockSize * 0.5f));

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllObjects);

	TArray<FOverlapResult> Results;
	bool bOverlapped = GetWorld()->OverlapMultiByObjectType(Results, CheckCenter, FQuat::Identity, ObjectQueryParams, CheckShape, QueryParams);

	if (!bOverlapped) return true;

	for (const FOverlapResult& Result : Results) {
		AActor* HitActor = Result.GetActor();
		if (!HitActor) continue;

		if (HitActor->IsA(AKillPlane::StaticClass())) return false;
	}

	return true;
}
//좌표의 블럭이 빈 블럭인지 확인
bool AMapConstructor::IsEmptyBlock(int32 InX, int32 InY, int32 InZ)
{
	if (!IsValidGrid(InX, InY, InZ))	return false;
	return MapData[Index(InX, InY, InZ)] == EBlockType::Empty;
}

bool AMapConstructor::FindNearestTopBlock(const FVector& WorldLocation, int32& OutX, int32& OutY, int32& OutZ, int32 XYSearchRadius)
{
	if (BlockSize <= 0.f) return false;
	FVector LocalLocation = WorldLocation - GetActorLocation();

	int32 BaseX = FMath::FloorToInt(LocalLocation.X / BlockSize);
	int32 BaseY = FMath::FloorToInt(LocalLocation.Y / BlockSize);
	
	if (BaseX < 0 || BaseX > Max_X || BaseY < 0 || BaseY > Max_Y) return false;

	FIntVector BestGrid(-1, -1, -1);
	float BestDistSq = 15000.f;

	for (int32 X = BaseX - XYSearchRadius; X <= BaseX + XYSearchRadius; ++X) {
		for (int32 Y = BaseY - XYSearchRadius; Y <= BaseY + XYSearchRadius; ++Y) {
			if (X < 0 || X >= Max_X || Y < 0 || Y >= Max_Y) continue;

			int32 TopZ = -1;

			if (!FindTopBlockZAtXY(X, Y, TopZ)) continue;
			if (!IsValidGrid(X, Y, TopZ)) continue;
			if (IsEmptyBlock(X, Y, TopZ)) continue;
			if (!IsTopBlock(X, Y, TopZ)) continue;

			FVector CandidateWorld = GridToWorldCenter(X, Y, TopZ);
			float DistSq = FVector::DistSquared2D(CandidateWorld, WorldLocation);

			if (DistSq < BestDistSq) {
				BestDistSq = DistSq;
				BestGrid = FIntVector(X, Y, TopZ);
			}
		}
	}

	if (BestGrid == FIntVector(-1, -1, -1)) return false;

	OutX = BestGrid.X;
	OutY = BestGrid.Y;
	OutZ = BestGrid.Z;

	return true;
}

FVector AMapConstructor::GridToWorldCenter(int32 InX, int32 InY, int32 InZ)
{
	return GetActorLocation() + FVector((InX + 0.5f) * BlockSize, (InY + 0.5f) * BlockSize, (InZ+1) * BlockSize);
}

FVector AMapConstructor::GetTopBlockLocationFromWorld(FVector WorldLocation)
{
	if (BlockSize <= 0.f) return FVector(-1.f, -1.f, -1.f);

	FVector BlockLocation = WorldLocation - GetActorLocation();
	FVector TargetLocation = FVector::ZeroVector;
	int32 GridX = FMath::FloorToInt(BlockLocation.X / BlockSize);
	int32 GridY = FMath::FloorToInt(BlockLocation.Y / BlockSize);

	if (GridX < 0 || GridX >= Max_X || GridY < 0 || GridY >= Max_Y) {
		return FVector(-1.f, -1.f, -1.f);
	}

	for (int32 GridZ = Max_Z - 1; GridZ >= 0; --GridZ) {
		if (MapData[Index(GridX, GridY, GridZ)] != EBlockType::Empty) {
			TargetLocation = GetActorLocation() + FVector(
				(GridX + 0.5f) * BlockSize,
				(GridY + 0.5f) * BlockSize,
				(GridZ + 1) * BlockSize
			);
			return TargetLocation;
		}
	}

	return FVector(-1.f, -1.f, -1.f);
}

int32 AMapConstructor::AddNormalBlockWithRandomUV(UInstancedStaticMeshComponent* ISMComp, const FTransform& Transform, int32 GridX, int32 GridY, int32 GridZ)
{
	if (!ISMComp) return -1;

	//노멀 블럭의 UV 데이터 설정 (누락 방지)
	if (ISMComp->NumCustomDataFloats < 3)
	{
		ISMComp->SetNumCustomDataFloats(3);
	}

	int32 InstanceIndex = ISMComp->AddInstance(Transform);

	float RotationIndex = FMath::RandRange(0, 3);
	float FlipU = FMath::RandRange(0, 1) == 1 ? 1.f : 0.f;
	float FlipV = FMath::RandRange(0, 1) == 1 ? 1.f : 0.f;

	ISMComp->SetCustomDataValue(InstanceIndex, 0, RotationIndex, false);
	ISMComp->SetCustomDataValue(InstanceIndex, 1, FlipU, false);
	ISMComp->SetCustomDataValue(InstanceIndex, 2, FlipV, false);

	return InstanceIndex;
}

//바닥 블럭 위에 무언가가 있는지 확인
bool AMapConstructor::IsEmptyOnFloorBlock(FIntVector& Grid, const TArray<AActor*> ActorsToIgnore) {
	if (!GetWorld()) return false;

	FVector FloorTop = GridToWorldCenter(Grid.X, Grid.Y, Grid.Z);
	FVector CheckCenter = FloorTop + FVector(0.f, 0.f, BlockSize * 0.5f);

	FCollisionShape BoxShape = FCollisionShape::MakeBox(FVector(BlockSize * 0.4, BlockSize * 0.4, BlockSize * 0.5f));

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllObjects);
	TArray<FOverlapResult> Results;

	bool bOverlapped = GetWorld()->OverlapMultiByObjectType(Results, CheckCenter, FQuat::Identity, ObjectQueryParams, BoxShape, QueryParams);

	if (!bOverlapped) return true;

	for (FOverlapResult& Result : Results) {
		AActor* HitActor = Result.GetActor();
		if(HitActor && !ActorsToIgnore.Contains(HitActor)) {
			return false;
		}
	}
	
	return true;
}

FIntPoint AMapConstructor::WorldDirectionToGridDirection(const FVector& Direction)
{
	FVector Dir = Direction;
	Dir.Z = 0.f;

	if (Dir.IsNearlyZero()) return FIntPoint(1, 0);

	Dir.Normalize();

	float AngleDegree = FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)));

	int32 Sector = FMath::RoundToInt(AngleDegree / 45.f);
	int32 WrappedSector = (Sector + 8) % 8;

	switch (WrappedSector) {
	case 0: return FIntPoint(1, 0);
	case 1: return FIntPoint(1, 1);
	case 2: return FIntPoint(0, 1);
	case 3: return FIntPoint(-1, 1);
	case 4: return FIntPoint(-1, 0);
	case 5: return FIntPoint(-1, -1);
	case 6: return FIntPoint(0, -1);
	case 7: return FIntPoint(1, -1);
	case 8: return FIntPoint(1, -1);
	default: return FIntPoint(1, 0);
	}
}

FVector AMapConstructor::GridDirectionToWorldDirection(const FIntPoint& GridDirection)
{
	FVector Dir((float)GridDirection.X, (float)GridDirection.Y, 0.f);
	if (Dir.IsNearlyZero()) return FVector::ForwardVector;

	return Dir.GetSafeNormal();
}

FIntPoint AMapConstructor::GetRightGridDirectionFromForward(const FIntPoint& GridForward)
{
	return FIntPoint(-GridForward.Y, GridForward.X);
}

float AMapConstructor::GridDirectionYaw(const FIntPoint& GridDirection)
{
	return GridDirectionToWorldDirection(GridDirection).Rotation().Yaw;
}

bool AMapConstructor::FindTopBlockZAtXY(int32 InX, int32 InY, int32& OutZ)
{
	for (int32 Z = Max_Z - 1; Z >= 0; --Z) {
		if (IsTopBlock(InX, InY, Z) && !IsEmptyBlock(InX, InY, Z)) {
			OutZ = Z;
			return true;
		}
	}

	OutZ = -1;
	return false;
}

void AMapConstructor::GetTopBlocksInRange(const FVector& WorldOrigin, float Range, TArray<FIntVector>& OutGrids)
{
	OutGrids.Reset();
	if (BlockSize <= 0 || Range <= 0.f) return;

	FVector LocalOrigin = WorldOrigin - GetActorLocation();
	
	int32 CenterX = FMath::FloorToInt(LocalOrigin.X / BlockSize);
	int32 CenterY = FMath::FloorToInt(LocalOrigin.Y / BlockSize);

	int32 SearchRadius = FMath::CeilToInt((Range + BlockSize) / BlockSize);
	float MaxDistSq = FMath::Square(Range + BlockSize);

	for (int32 x = CenterX - SearchRadius; x <= CenterX + SearchRadius; x++) {
		for (int32 y = CenterY - SearchRadius; y <= CenterY + SearchRadius; y++) {
			if (x < 0 || x >= Max_X || y < 0 || y >= Max_Y) continue;
			int32 TopZ = -1;

			if (!FindTopBlockZAtXY(x, y, TopZ)) continue;
			if (!IsValidGrid(x, y, TopZ)) continue;
			if (IsEmptyBlock(x, y, TopZ)) continue;
			if (!IsTopBlock(x, y, TopZ)) continue;

			FVector TopCenter = GridToWorldCenter(x, y, TopZ);
			FVector Diff = TopCenter - WorldOrigin;
			Diff.Z = 0.f;

			if (Diff.SizeSquared() > MaxDistSq) continue;

			OutGrids.Add(FIntVector(x, y, TopZ));
		}
	}
}

void AMapConstructor::ClearDamageBlocks()
{
	for (ADamageBlock* DamageBlocks : Damage_ISM) {
		if (IsValid(DamageBlocks)) DamageBlocks->Destroy();
	}

	UE_LOG(LogTemp, Warning, TEXT("ClearDamageBlocks Count : %d"), Damage_ISM.Num());

	Damage_ISM.Empty();
}


