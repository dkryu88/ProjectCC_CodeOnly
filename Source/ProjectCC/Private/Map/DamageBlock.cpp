// Fill out your copyright notice in the Description page of Project Settings.


#include "Map/DamageBlock.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "BlockType.h"
#include "Objects.h"

// Sets default values
ADamageBlock::ADamageBlock()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockMesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);

	BlockCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicsCollider"));
	BlockCollision->SetupAttachment(Root);
	BlockCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockCollision->SetCollisionProfileName(TEXT("BlockAll"));
	BlockCollision->SetGenerateOverlapEvents(false);

	DamageCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageTrigger"));
	DamageCollider->SetupAttachment(Root);
	DamageCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageCollider->SetGenerateOverlapEvents(true);
	DamageCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap);

	BlockCollision->SetRelativeLocation(FVector(0.f, 0.f, BlockSize * 0.5f));
	BlockCollision->SetBoxExtent(FVector(BlockSize * 0.5f, BlockSize * 0.5f, BlockSize * 0.5f));
	DamageCollider->SetRelativeLocation(FVector(0.f, 0.f, BlockSize * 0.5f));
	DamageCollider->SetBoxExtent(FVector(BlockSize * 0.5f + TriggerPadding, BlockSize * 0.5f + TriggerPadding, BlockSize * 0.5f + TriggerPadding));

	DamageCollider->OnComponentBeginOverlap.AddDynamic(this, &ADamageBlock::OnDamageTriggerBeginOverlap);
	DamageCollider->OnComponentEndOverlap.AddDynamic(this, &ADamageBlock::OnDamageTriggerEndOverlap);

}

// Called when the game starts or when spawned
void ADamageBlock::BeginPlay()
{
	Super::BeginPlay();
	
	if (!HasAuthority()) return;

	GetWorldTimerManager().SetTimer(DamageTickTimerHandle, this, &ADamageBlock::UpdateOverlappingDamageTargets, DamageCheckPeriod, true);
}

void ADamageBlock::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	GetWorldTimerManager().ClearTimer(DamageTickTimerHandle);
	ActorsDamageIntervals.Empty();

	Super::EndPlay(EndPlayReason);
}

void ADamageBlock::InitializeDamageBlock(float InBlockSize, AMapConstructor* Map, const FIntVector& Grid) {
	BlockSize = InBlockSize;
	NowMap = Map;
	GridLocation = Grid;
	DamageCollider->SetRelativeLocation(FVector(0.f, 0.f, BlockSize * 0.5f));
	DamageCollider->SetBoxExtent(FVector(BlockSize * 0.5f + TriggerPadding, BlockSize * 0.5f + TriggerPadding, BlockSize * 0.5f + TriggerPadding));

	Mesh->SetRelativeLocation(FVector::ZeroVector);
}

bool ADamageBlock::CanDamageActor(AActor* TargetActor) {
	if (!TargetActor) return false;

	if (Cast<APlayer_Character>(TargetActor)) {
		return true;
	}
	if (AObjects* Object = Cast<AObjects>(TargetActor)) {
		return Object->HP != -10.f;
	}
	return false;
}

void ADamageBlock::ApplyDamageToActor(AActor* OtherActor) {
	if (!OtherActor) return;
	UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, nullptr, this, UDamageType::StaticClass());
}

void ADamageBlock::OnDamageTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (!HasAuthority()) return;
	if (!CanDamageActor(OtherActor)) return;
	if (!CanDamageActor(OtherActor)) return;

	if (ActorsDamageIntervals.Contains(OtherActor)) return;

	float NowTime = GetWorld()->GetTimeSeconds();

	ApplyDamageToActor(OtherActor);
	ActorsDamageIntervals.Add(OtherActor, NowTime + DamageInterval);

}

void ADamageBlock::OnDamageTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;
	ActorsDamageIntervals.Remove(OtherActor);
}
	
void ADamageBlock::UpdateOverlappingDamageTargets() {
	if (!HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	float NowTime = World->GetTimeSeconds();

	for (auto IT = ActorsDamageIntervals.CreateIterator(); IT; ++IT) {
		AActor* TargetActor = IT.Key().Get();

		if (!IsValid(TargetActor)) {			
			IT.RemoveCurrent();
			continue;
		}

		if (!DamageCollider->IsOverlappingActor(TargetActor)) {
			IT.RemoveCurrent();
			continue;
		}

		if (NowTime >= IT.Value()) {
			ApplyDamageToActor(TargetActor);
			IT.Value() = NowTime + DamageInterval;
		}
	}
}

void ADamageBlock::RemoveFromMap()
{
	if (!NowMap.IsValid()) return;

	NowMap->SetBlock(GridLocation.X, GridLocation.Y, GridLocation.Z, EBlockType::Empty);
}
	
	
	
	