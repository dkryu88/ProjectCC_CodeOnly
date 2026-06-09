// Fill out your copyright notice in the Description page of Project Settings.


#include "KillPlane.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Player_Character.h"
#include "EffectManagerComponent.h"
#include "MapConstructor.h"

// Sets default values
AKillPlane::AKillPlane()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	//Plane Overlap Collision
	KillCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("Kill Collider"));
	SetRootComponent(KillCollider);
	KillCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	KillCollider->SetCollisionResponseToAllChannels(ECR_Overlap);
	KillCollider->SetGenerateOverlapEvents(true);
	//Plane 매쉬
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetupAttachment(KillCollider);

	//이펙트 담당 컴포넌트
	EffectManagerComp = CreateDefaultSubobject<UEffectManagerComponent>(TEXT("EffectManager"));
}

// Called when the game starts or when spawned
void AKillPlane::BeginPlay()
{
	Super::BeginPlay();
	KillCollider->OnComponentBeginOverlap.AddDynamic(this, &AKillPlane::OnKillPlaneBeginOverlap);
}

void AKillPlane::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);
	if (KillCollider) {
		SetSizeofKillColliderwithMesh();
	}

}

void AKillPlane::OnKillPlaneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!OtherActor || OtherActor == this) return;
	if (APlayer_Character* Player = Cast<APlayer_Character>(OtherActor)) {
		Player->SinkSpeed = 10.f;
		Player->AddInputBlockController("KillPlane", true, true, true, true);
		Player->ApplyDamageInternal(200.0f, nullptr, nullptr, false, false, true);
		return;
	}
	else if (OtherActor->ActorHasTag(TEXT("NonDestroy"))) {
		return;
	}
	else {
		OtherActor->Destroy();
	}
	
}

// Called every frame
void AKillPlane::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//Kill Collider 크기를 계산
void AKillPlane::SetSizeofKillColliderwithMesh()
{
	if (!Mesh || !Mesh->GetStaticMesh() || !KillCollider) return;
	FBoxSphereBounds MeshBounds = Mesh->GetStaticMesh()->GetBounds();
	FVector MeshSize = Mesh->GetRelativeScale3D().GetAbs();
	//Mesh의 각 크기값을 획득
	FVector BaseSize = MeshBounds.BoxExtent;
	BaseSize.X *= MeshSize.X;
	BaseSize.Y *= MeshSize.Y;
	BaseSize.Z *= MeshSize.Z;
	//최종 Collider 크기 계산
	FVector ColliderSize;
	ColliderSize.X = BaseSize.X * SizeMagnification.X + ColliderOffset.X;
	ColliderSize.Y = BaseSize.Y * SizeMagnification.Y + ColliderOffset.Y;
	ColliderSize.Z = BaseSize.Z * SizeMagnification.Z + ColliderOffset.Z;
	//최솟값 설정(음수 방지)
	ColliderSize.X = FMath::Max(ColliderSize.X, 0.01f);
	ColliderSize.Y = FMath::Max(ColliderSize.Y, 0.01f);
	ColliderSize.Z = FMath::Max(ColliderSize.Z, 0.01f);
	//최종 Collider 크기 적용
	KillCollider->SetBoxExtent(ColliderSize);
}