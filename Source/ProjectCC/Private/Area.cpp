// Fill out your copyright notice in the Description page of Project Settings.


#include "Area.h"
#include "MapConstructor.h"
#include "Player_Character.h"
#include "Objects.h"
#include "Item.h"
#include "Weapon.h"
#include "PlayMode_Match.h"
#include "EffectManagerComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Sets default values
AArea::AArea()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	AreaDetectCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectCollider"));
	SetRootComponent(AreaDetectCollider);
	AreaDetectCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaDetectCollider->SetGenerateOverlapEvents(true);
	AreaDetectCollider->SetCollisionObjectType(ECC_GameTraceChannel4);
	AreaDetectCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaDetectCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AreaDetectCollider->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	AreaDetectCollider->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);

	AreaDetectCollider->SetCanEverAffectNavigation(false);
	
	AreaDetectCollider->OnComponentBeginOverlap.AddDynamic(this, &AArea::OnAreaBeginOverlap);
	AreaDetectCollider->OnComponentEndOverlap.AddDynamic(this, &AArea::OnAreaEndOverlap);

	//차후에 이펙트로 실질적 처리 (이펙트 제작 후 매쉬 제거)
	MeshPivot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshPivot"));
	MeshPivot->SetupAttachment(AreaDetectCollider);
	TestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	TestMesh->SetSimulatePhysics(false);
	TestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestMesh->SetupAttachment(MeshPivot);

	//이펙트 담당 컴포넌트
	EffectManagerComp = CreateDefaultSubobject<UEffectManagerComponent>(TEXT("EffectManager"));
}

// Called when the game starts or when spawned
void AArea::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;

	if (AreaDuration > 0.f) SetLifeSpan(AreaDuration);
	GetWorldTimerManager().SetTimerForNextTick(this, &AArea::RegisterInitialOverlaps);

}

void AArea::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	if (HasAuthority()) {
		ClearGroupEffects();
	}

	if (UWorld* World = GetWorld()) {
		for (TPair<TWeakObjectPtr<AActor>, FTimerHandle>& Pair : ActorRefreshTimerHandles) {
			World->GetTimerManager().ClearTimer(Pair.Value);
		}
	}

	ActorRefreshTimerHandles.Empty();

	Super::EndPlay(EndPlayReason);
}

bool AArea::InitializeArea(int32 x, int32 y, int32 z) {
	if (!NowMap) return false;
	FIntVector CandidateLocation = FIntVector(x, y, z);
	CandidateLocation = CheckCanSpawnOnTheLocation(CandidateLocation);
	if (CandidateLocation == FIntVector(-1, -1, -1)) return false;
	GridLocation = CandidateLocation;
	return true;
}

FTransform AArea::GetSpawnTransform()
{
	if (!NowMap) return FTransform::Identity;
	FVector SpawnLocation = NowMap->GridToWorldCenter(GridLocation.X, GridLocation.Y, GridLocation.Z);
	SpawnLocation.Z += 2.f;
	
	return FTransform(FRotator::ZeroRotator, SpawnLocation, FVector::OneVector);
}

void AArea::ApplyAreaSizeFromGrid()
{
	if (!NowMap || !AreaDetectCollider) return;

	float BlockSize = NowMap->BlockSize;
	AreaDetectCollider->SetBoxExtent(FVector(BlockSize * 0.5f, BlockSize * 0.5f, BlockSize * 0.1f));
}

void AArea::ResetTransformFromGrid()
{
	if (!NowMap) return;
	SetActorTransform(GetSpawnTransform());
	ApplyAreaSizeFromGrid();
}
//Area Spawn 위치가 유효한지 확인
FIntVector AArea::CheckCanSpawnOnTheLocation(FIntVector CandidateLocation) {
	int32 x = CandidateLocation.X;
	int32 y = CandidateLocation.Y;
	int32 z = CandidateLocation.Z;
	if (!NowMap) return FIntVector(-1, -1, -1);
	if (x < 0 || x >= NowMap->Max_X || y < 0 || y >= NowMap->Max_Y) return FIntVector(-1, -1, -1);

	for (int32 i = z - 1; i <= z + 1; i++) {
		if (!NowMap->IsValidGrid(x, y, i)) continue;
		if (NowMap->IsEmptyBlock(x, y, i)) continue;
		if (NowMap->IsTopBlock(x, y, i)) return FIntVector(x, y, i);
	}
	return FIntVector(-1, -1, -1);
}

//Area 데이터 초기화
void AArea::SetAreaData(float Duration, float Interval)
{
	AreaDuration = Duration;
	AreaEffectInterval = Interval;
}


// Area 생성 시 이미 안에 있는 액터 검색 및 등록
void AArea::RegisterInitialOverlaps()
{
	if (!HasAuthority()) return;
	if (!AreaDetectCollider) return;

	AreaDetectCollider->UpdateOverlaps();

	TArray<AActor*> OverlappingActors;
	AreaDetectCollider->GetOverlappingActors(OverlappingActors);

	AArea* Root = RootArea ? RootArea.Get() : this;
	if (!Root) return;

	for (AActor* Actor : OverlappingActors) {
		if (!Actor || Actor == this) continue;
		//BeginOverlap은 RootArea만 실행 시켜 같은 그룹 Area의 중복 효과 적용 방지
		Root->HandleGroupBeginOverlapFromArea(this ,Actor);
	}
}

//Area 내의 특정 Player의 Stay Effect 발동 타이머 시작 (Begin Overlap시 발동)
void AArea::StartActorRefreshTimer(AActor* OtherActor) {
	if (!HasAuthority())return;
	if (!OtherActor) return;
	if (AreaEffectInterval <= 0.f) return;

	UWorld* World = GetWorld();
	if (!World) return;

	TWeakObjectPtr<AActor> ActorKey = OtherActor;

	//이미 대상 Actor의 타이머가 작동 중일 경우 종료
	if (FTimerHandle* ExistingHandle = ActorRefreshTimerHandles.Find(ActorKey)) {
		if (World->GetTimerManager().IsTimerActive(*ExistingHandle)) return;
	}

	FTimerHandle& NewHandle = ActorRefreshTimerHandles.FindOrAdd(ActorKey);

	//타이머가 작동 중일 때 RefreshActorEffect를 호출
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &AArea::RefreshActorEffect, ActorKey);

	//TimerDelegate 타이머에 바인딩 된 RefreshActorEffect를 AreaEffectInterval마다 실행
	World->GetTimerManager().SetTimer(NewHandle, TimerDelegate, AreaEffectInterval, true);
}

//대상 Actor에 대해 StayAreaEffect 적용 / 장판에서 벗어난 Actor를 정리
void AArea::RefreshActorEffect(TWeakObjectPtr<AActor> ActorPtr)
{
	if (!HasAuthority()) return;

	//ActorPtr가 비유효시 즉시 목록에서 제거
	if (!ActorPtr.IsValid()) {
		GroupActorsInArea.Remove(ActorPtr);
		StopActorRefreshTimerByKey(ActorPtr);
		return;
	}

	//ActorPtr가 존재하지 않는다면 즉시 타이머 정리
	if (!GroupActorsInArea.Contains(ActorPtr)) {
		StopActorRefreshTimerByKey(ActorPtr);
		return;
	}
	ApplyStayAreaEffect(ActorPtr.Get());
}

//특정 Player의 stay Effect 발동 타이머 종료 
void AArea::StopActorRefreshTimer(AActor* OtherActor)
{
	if (!OtherActor) return;
	StopActorRefreshTimerByKey(TWeakObjectPtr<AActor>(OtherActor));
}

//ActorKey를 사용하여 해당 Actor 타이머 정지
void AArea::StopActorRefreshTimerByKey(TWeakObjectPtr<AActor> ActorKey)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (FTimerHandle* Handle = ActorRefreshTimerHandles.Find(ActorKey)) {
		World->GetTimerManager().ClearTimer(*Handle);
		ActorRefreshTimerHandles.Remove(ActorKey);
	}
}

//각 Area의 Overlap 검사
void AArea::OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!OtherActor || OtherActor == this) return;

	AArea* Root = RootArea ? RootArea.Get() : this;
	if (!Root) return;

	Root->HandleGroupBeginOverlapFromArea(this, OtherActor);
}

void AArea::OnAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;
	if (!OtherActor || OtherActor == this) return;

	AArea* Root = RootArea ? RootArea.Get() : this;
	if (!Root) return;

	Root->HandleGroupEndOverlapFromArea(this, OtherActor);
}

//실제 Overlap 처리 (Root Area가 직접 처리)
void AArea::HandleGroupBeginOverlapFromArea(AArea* area, AActor* OtherActor) {
	if (!HasAuthority()) return;
	if (!OtherActor || OtherActor == this) return;
	APlayer_Character* Player = Cast<APlayer_Character>(OtherActor);
	if (!Player) return;

	TWeakObjectPtr<AActor> ActorKey = OtherActor;
	TWeakObjectPtr<AArea> AreaKey = area;

	//대상 Actor가 같은 RootArea 그룹 내에서 어떤 Area들과 겹치는지, 몇개의 Area들과 겹치는지 확인
	TSet<TWeakObjectPtr<AArea>>& OverlapAreas = GroupOverlapAreaSet.FindOrAdd(ActorKey);
	int32 PrevNum = OverlapAreas.Num();

	OverlapAreas.Add(AreaKey);

	//같은 RootArea 그룹에 최초 진입 시 진입 효과 발동
	if (PrevNum == 0 && OverlapAreas.Num() == 1) {
		GroupActorsInArea.Add(ActorKey);
		ApplyInAreaEffect(OtherActor);
		StartActorRefreshTimer(OtherActor);
	}
}

void AArea::HandleGroupEndOverlapFromArea(AArea* area, AActor* OtherActor) {
	if (!HasAuthority()) return;
	if (!area || !OtherActor || OtherActor == this) return;

	TWeakObjectPtr<AActor> ActorKey = OtherActor;
	TSet<TWeakObjectPtr<AArea>>* OverlapAreas = GroupOverlapAreaSet.Find(ActorKey);
	if (!OverlapAreas) return;
	
	//대상 Actor(Player)가 빠져나간 Area를 확인 및 목록에서 제거
	OverlapAreas->Remove(TWeakObjectPtr<AArea>(area));

	//모든 RootArea 소속 Area와 겹치지 않는 경우 처리
	if (OverlapAreas->Num() <= 0) {
		GroupActorsInArea.Remove(ActorKey);
		StopActorRefreshTimer(OtherActor);
		ApplyOutAreaEffect(OtherActor);
	}
}

//Area 내의 모든 유효 Actor에게 Out Effect 적용 (Area가 사라질 때 호출)
void AArea::ClearGroupEffects() {
	for (const TWeakObjectPtr<AActor>& ActorPtr : GroupActorsInArea) {
		if (ActorPtr.IsValid()) {
			ApplyOutAreaEffect(ActorPtr.Get());
		}

		StopActorRefreshTimerByKey(ActorPtr);
	}

	GroupActorsInArea.Empty();
	ActorRefreshTimerHandles.Empty();
}

//Area 그룹의 RootArea 획득
AArea* AArea::GetRootArea()
{
	if (RootArea) {
		return RootArea.Get();
	}
	return this;
}
//Area가 같은 Grid 위치에 있는지 확인
bool AArea::IsSameGrid(const FIntVector& grid) {
	return GridLocation == grid;
}
//같은 위치의 Area를 제거
void AArea::DestroyOnlySameGridArea()
{
	if (!HasAuthority()) return;

	AArea* Root = GetRootArea();

	//Root가 자신이 아니라면 그냥 자신을 제거
	if (Root && Root != this) {
		Destroy();
		return;
	}
	//Root가 자신이라면 Root를 같은 그룹 Area에 넘겨주고 제거
	UWorld* World = GetWorld();
	if (!World) {
		Destroy();
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AArea::StaticClass(), FoundActors);

	AArea* NewRoot = nullptr;

	for (AActor* Actor : FoundActors) {
		AArea* Area = Cast<AArea>(Actor);
		if (!Area || Area == this) continue;
		if (Area->GetRootArea() == this) {
			NewRoot = Area;
			break;
		}
	}

	if (NewRoot) {
		TransferRootToOtherArea(NewRoot);
	}

	Destroy();
}
//Grid 위치에 Area가 있는지 없는지 확인
AArea* AArea::FindExistingAreaAtGrid(UWorld* World, AMapConstructor* Map, const FIntVector& Grid, AArea* IgnoreArea) {
	if (!World || !Map) return nullptr;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AArea::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors) {
		AArea* Area = Cast<AArea>(Actor);
		if (!Area || Area == IgnoreArea) continue;
		if (Area->NowMap != Map) continue;
		if (Area->GetGridLocation() == Grid) return Area;
	}

	return nullptr;
}
//Area 그룹의 Root Area를 소속된 다른 Area로 변경
void AArea::TransferRootToOtherArea(AArea* NewRoot) {
	if (!HasAuthority() || !NewRoot || NewRoot == this) return;

	NewRoot->GroupActorsInArea = MoveTemp(GroupActorsInArea);
	NewRoot->GroupOverlapAreaSet = MoveTemp(GroupOverlapAreaSet);
	NewRoot->ActorRefreshTimerHandles = MoveTemp(ActorRefreshTimerHandles);

	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AArea::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors) {
		AArea* Area = Cast<AArea>(Actor);
		if (!Area) continue;
		if (Area == this) continue;
		if (Area->GetRootArea() == this) {
			Area->RootArea = NewRoot;
			Area->bIsRoot = false;
		}
	}

	NewRoot->RootArea = NewRoot;
	NewRoot->bIsRoot = true;
}

//영역으로 진입 시 효과
void AArea::ApplyInAreaEffect_Implementation(AActor* OtherActor) {}
//영역에서 벗어날 시 효과
void AArea::ApplyOutAreaEffect_Implementation(AActor* OtherActor) {}
//영역 내에 계속 있을 때 적용되는 효과
void AArea::ApplyStayAreaEffect_Implementation(AActor* OtherActor) {}

/*TObjectPtr - TWeakObjectPtr 차이*/
/*
* TObjectPtr : 직접 객체를 참조하며 보관 중 (소유권 O)
* TWeakObjectPtr : 이 객체가 유효하든 안하든 추적 (소유권 X)
*/