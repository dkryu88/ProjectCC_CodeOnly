// Fill out your copyright notice in the Description page of Project Settings.


#include "KillPlane.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Effect/GameEffectManagerComponent.h"
#include "Effect/FGameEffectData.h"

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

	//상시 이펙트 담당 컴포넌트
	LifeTimeEffectComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Effect"));
	LifeTimeEffectComp->SetupAttachment(KillCollider);
	LifeTimeEffectComp->SetUsingAbsoluteScale(true);
	LifeTimeEffectComp->SetWorldScale3D(FVector::OneVector);
	LifeTimeEffectComp->bAutoActivate = true;
	LifeTimeEffectComp->SetAutoDestroy(false);

	EffectManagerComp = CreateDefaultSubobject<UGameEffectManagerComponent>(TEXT("EffectManager"));
	if (EffectManagerComp) EffectManagerComp->SetIsReplicated(true);
}

// Called when the game starts or when spawned
void AKillPlane::BeginPlay()
{
	Super::BeginPlay();
	KillCollider->OnComponentBeginOverlap.AddDynamic(this, &AKillPlane::OnKillPlaneBeginOverlap);

	ApplyLifeTimeEffectParams();
}

void AKillPlane::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);
	if (KillCollider) {
		SetSizeofKillColliderwithMesh();
		ApplyLifeTimeEffectParams();
	}

}

void AKillPlane::OnKillPlaneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!OtherActor || OtherActor == this) return;
	if (OtherActor->ActorHasTag(TEXT("NonDestroy"))) return;

	if (ProcessedActors.Contains(OtherActor)) return;

	ProcessedActors.Add(OtherActor);

	if (APlayer_Character* Player = Cast<APlayer_Character>(OtherActor)) {
		PlayDestroyEffectForActor(Player, OtherComp);
		Player->SinkSpeed = 15.f;
		Player->AddInputBlockController("KillPlane", true, true, true, true);
		Player->Multicast_StopPhysicsOnKillPlane();
		Player->ApplyDamageInternal(200.0f, nullptr, nullptr, false, false, false, true);
		return;
	}
	
	else {
		PlayDestroyEffectForActor(OtherActor, OtherComp);
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

void AKillPlane::ApplyLifeTimeEffectParams()
{
	if (!LifeTimeEffectComp || !KillCollider) return;

	FVector HalfExtent = KillCollider->GetScaledBoxExtent();

	//실제 월드 크기
	FVector FullSize = HalfExtent * 2.f;

	//이펙트가 가장자리보다 살짝 안쪽에 보이도록 축소
	FVector SizePadding = FVector(10.f, 10.f, 0.f);
	FVector EffectSize = FullSize - SizePadding;

	//음수 방지
	EffectSize.X = FMath::Max(1.f, EffectSize.X);
	EffectSize.Y = FMath::Max(1.f, EffectSize.Y);
	EffectSize.Z = 0.5f;

	float EffectScale = FMath::Max(0.01f, EffectSize.GetMax() / 100.f);

	LifeTimeEffectComp->SetVariableVec3(TEXT("User.Size"), EffectSize);
	LifeTimeEffectComp->SetVariableFloat(TEXT("User.Scale"), EffectScale);
}

void AKillPlane::PlayDestroyEffectForActor(AActor* TargetActor, UPrimitiveComponent* TargetComp)
{
	if (!HasAuthority()) return;
	if (!TargetActor) return;
	if (!EffectManagerComp) return;
	//이미 아웃된 플레이어면 중복 방지 리턴
	if (APlayer_Character* Character = Cast<APlayer_Character>(TargetActor)) {
		if (Character->bIsOut) return;
	}
	if (!KillPlaneDestroyEffect.NiagaraEffect && !KillPlaneDestroyEffect.Sound) return;

	FKillPlaneSurfaceInfo SurfaceInfo = GetEffectSurfaceInfo(TargetActor);
	FVector SpawnLocation = GetEffectLocationFromSurface(TargetActor, SurfaceInfo);
	FRotator SpawnRotation = GetEffectRotationFromSurface(SurfaceInfo);
	FVector PlaneScale = GetEffectPlaneScaleForShapeLocation(TargetActor, TargetComp, SurfaceInfo.SurfaceAxis);

	float EffectLifeTime = TargetActor->IsA<APlayer_Character>() ? 3.f : 1.f;

	FGameEffectData* EffectData = &KillPlaneDestroyEffect;
	EffectData->SpawnLocationType = EGameEffectSpawnLocationType::WorldLocation;
	EffectData->SpawnRotationType = EGameEffectSpawnRotationType::WorldRotation;

	FGameEffectContext Context;
	Context.SourceActor = this;
	Context.SourceComponent = KillCollider;
	Context.WorldLocation = SpawnLocation;
	Context.WorldRotation = SpawnRotation;

	FGameEffectRuntimeParams Params;
	Params.AddFloatParam(TEXT("User.PlaneScaleX"), PlaneScale.X);
	Params.AddFloatParam(TEXT("User.PlaneScaleY"), PlaneScale.Y);
	Params.AddFloatParam(TEXT("User.LifeTime"), EffectLifeTime);

	EffectManagerComp->PlayGameEffect_Multicast(*EffectData, Context, Params);
}

FKillPlaneSurfaceInfo AKillPlane::GetEffectSurfaceInfo(AActor* TargetActor)
{
	FKillPlaneSurfaceInfo Info;

	if (!KillCollider || !TargetActor) return Info;

	FTransform ColliderTransform = KillCollider->GetComponentTransform();
	FVector BoxExtent = KillCollider->GetUnscaledBoxExtent();

	FVector LocalTargetLocation = ColliderTransform.InverseTransformPosition(TargetActor->GetActorLocation());

	float AbsX = FMath::Abs(LocalTargetLocation.X);
	float AbsY = FMath::Abs(LocalTargetLocation.Y);
	float AbsZ = FMath::Abs(LocalTargetLocation.Z);

	float XRatio = BoxExtent.X > 0.f ? AbsX / BoxExtent.X : 0.f;
	float YRatio = BoxExtent.Y > 0.f ? AbsY / BoxExtent.Y : 0.f;
	float ZRatio = BoxExtent.Z > 0.f ? AbsZ / BoxExtent.Z : 0.f;

	if (XRatio >= YRatio && XRatio >= ZRatio) {
		Info.SurfaceAxis = EKillPlaneEffectSurfaceAxis::X;
		Info.SurfaceValue = LocalTargetLocation.X >= 0.f ? 1.f : -1.f;
		Info.LocalNormal = FVector(Info.SurfaceValue, 0.f, 0.f);
	}
	else if (YRatio >= XRatio && YRatio >= ZRatio) {
		Info.SurfaceAxis = EKillPlaneEffectSurfaceAxis::Y;
		Info.SurfaceValue = LocalTargetLocation.Y >= 0.f ? 1.f : -1.f;
		Info.LocalNormal = FVector(0.f, Info.SurfaceValue, 0.f);
	}
	else if (ZRatio >= YRatio && ZRatio >= XRatio) {
		Info.SurfaceAxis = EKillPlaneEffectSurfaceAxis::Z;
		Info.SurfaceValue = LocalTargetLocation.Z >= 0.f ? 1.f : -1.f;
		Info.LocalNormal = FVector(0.f, 0.f, Info.SurfaceValue);
	}

	Info.WorldNormal = ColliderTransform.TransformVectorNoScale(Info.LocalNormal.GetSafeNormal());

	return Info;
}

FVector AKillPlane::GetEffectLocationFromSurface(AActor* TargetActor, const FKillPlaneSurfaceInfo& SurfaceInfo)
{
	if (!KillCollider) return TargetActor ? TargetActor->GetActorLocation() : GetActorLocation();

	FTransform ColliderTransform = KillCollider->GetComponentTransform();
	FVector BoxExtent = KillCollider->GetUnscaledBoxExtent();

	FVector LocalLocation = FVector::ZeroVector;

	if (ActorHasTag(TEXT("Block"))) {
		switch (SurfaceInfo.SurfaceAxis) {
		case EKillPlaneEffectSurfaceAxis::X:
			LocalLocation = FVector(SurfaceInfo.SurfaceValue * (BoxExtent.X + EffectSurfaceOffsetZ), 0.f, 0.f);
			break;
		case EKillPlaneEffectSurfaceAxis::Y:
			LocalLocation = FVector(0.f, SurfaceInfo.SurfaceValue * (BoxExtent.Y + EffectSurfaceOffsetZ), 0.f);
			break;
		case EKillPlaneEffectSurfaceAxis::Z:
		default:
			LocalLocation = FVector(0.f, 0.f, SurfaceInfo.SurfaceValue * (BoxExtent.Z + EffectSurfaceOffsetZ));
			break;
		}

		return ColliderTransform.TransformPosition(LocalLocation);
	}

	if (TargetActor) LocalLocation = ColliderTransform.InverseTransformPosition(TargetActor->GetActorLocation());

	LocalLocation.X = FMath::Clamp(LocalLocation.X, -BoxExtent.X, BoxExtent.X);
	LocalLocation.Y = FMath::Clamp(LocalLocation.Y, -BoxExtent.Y, BoxExtent.Y);
	LocalLocation.Z = FMath::Clamp(LocalLocation.Z, -BoxExtent.Z, BoxExtent.Z);

	switch (SurfaceInfo.SurfaceAxis){
	case EKillPlaneEffectSurfaceAxis::X:
		LocalLocation.X = SurfaceInfo.SurfaceValue * (BoxExtent.X + EffectSurfaceOffsetZ);
		break;

	case EKillPlaneEffectSurfaceAxis::Y:
		LocalLocation.Y = SurfaceInfo.SurfaceValue * (BoxExtent.Y + EffectSurfaceOffsetZ);
		break;

	case EKillPlaneEffectSurfaceAxis::Z:
	default:
		LocalLocation.Z = SurfaceInfo.SurfaceValue * (BoxExtent.Z + EffectSurfaceOffsetZ);
		break;
	}

	return ColliderTransform.TransformPosition(LocalLocation);
}

FRotator AKillPlane::GetEffectRotationFromSurface(const FKillPlaneSurfaceInfo& SurfaceInfo)
{
	return FRotationMatrix::MakeFromZ(SurfaceInfo.WorldNormal).Rotator();
}


FVector AKillPlane::GetEffectPlaneScaleForShapeLocation(AActor* TargetActor, UPrimitiveComponent* TargetComp, EKillPlaneEffectSurfaceAxis SurfaceAxis)
{
	if (!TargetActor) return FVector(EffectMinPlaneSizeForShapeLocation, EffectMinPlaneSizeForShapeLocation, 0.f);

	FVector Extent = FVector::ZeroVector;

	if (TargetComp) Extent = TargetComp->Bounds.BoxExtent;
	if (Extent.IsNearlyZero()) {
		FVector Origin;
		TargetActor->GetActorBounds(false, Origin, Extent);
	}

	float SizeA = 0.f;
	float SizeB = 0.f;

	switch (SurfaceAxis) {
	case EKillPlaneEffectSurfaceAxis::X:
		SizeA = Extent.Y * 2.f * EffectPlaneSizeScale;
		SizeB = Extent.Z * 2.f * EffectPlaneSizeScale;
		break;
	case EKillPlaneEffectSurfaceAxis::Y:
		SizeA = Extent.X * 2.f * EffectPlaneSizeScale;
		SizeB = Extent.Z * 2.f * EffectPlaneSizeScale;
		break;
	case EKillPlaneEffectSurfaceAxis::Z:
	default:
		SizeA = Extent.X * 2.f * EffectPlaneSizeScale;
		SizeB = Extent.Y * 2.f * EffectPlaneSizeScale;
		break;
	}

	float ScaleX = FMath::CeilToFloat(SizeA / 25.f);
	float ScaleY = FMath::CeilToFloat(SizeB / 25.f);

	ScaleX = FMath::Max(1, ScaleX);
	ScaleY = FMath::Max(1, ScaleY);

	return FVector(ScaleX, ScaleY, 0.f);
}
