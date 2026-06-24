// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_PunchGun.h"
#include "MapConstructor.h"
#include "Player_Character.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"


AWeapon_PunchGun::AWeapon_PunchGun(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UStaticMeshComponent>((TEXT("WeaponPhysicsCollider"))))
{
	GloveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Glove"));
	GloveMesh->SetupAttachment(Mesh);
	GloveMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GloveMesh->SetSimulatePhysics(false);

	SpringMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Spring"));
	SpringMesh->SetupAttachment(Mesh);
	SpringMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpringMesh->SetSimulatePhysics(false);
}

void AWeapon_PunchGun::BeginPlay() {
	Super::BeginPlay();

	if (Mesh) {
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetSimulatePhysics(false);
	}

	if (GloveMesh && GloveMesh->GetStaticMesh()) {
		float GloveMinY = GloveMesh->GetStaticMesh()->GetBoundingBox().Min.Y;
		float HandleMaxY = (Mesh && Mesh->GetStaticMesh()) ? Mesh->GetStaticMesh()->GetBoundingBox().Max.Y : 0.f;
		GloveBaseLocation = FVector(0.f, HandleMaxY - GloveMinY, 0.f);
	}

	if (SpringMesh) {
		SpringBaseScale = SpringMesh->GetRelativeScale3D();
		SpringBaseLocation = SpringMesh->GetRelativeLocation();
		if (SpringMesh->GetStaticMesh()) SpringMeshNaturalY = SpringMesh->GetStaticMesh()->GetBoundingBox().GetSize().Y;
		SpringMesh->SetVisibility(false);
	}

	if (GloveMesh) GloveMesh->SetRelativeLocation(GloveBaseLocation);
	if (bIsEquipped && EquippedPlayer) ApplyEquipState();
}

void AWeapon_PunchGun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsAttacking) return;

	AttackAlpha += DeltaTime / 0.2f;
	bool bFinished = AttackAlpha >= 1.f;
	if (bFinished) AttackAlpha = 1.f;

	const float PeakAlpha = 0.1f / 0.2f;
	float Value = (AttackAlpha <= PeakAlpha) ? AttackAlpha / PeakAlpha : 1.f - (AttackAlpha - PeakAlpha) / (1.f - PeakAlpha);
	OnTimelineUpdate(Value);

	if (bFinished) {
		bIsAttacking = false;
		OnTimelineFinished();
	}
}


bool AWeapon_PunchGun::BeforeAttackWeaponFunction()
{
	if (HasAuthority()) Multicast_PlaySpringAttack(GetGloveLaunchDistance());
	else Server_TriggerSpringAttack();

	return true;
}

void AWeapon_PunchGun::AdditionalUnEquipWeaponFunction()
{
	Super::AdditionalUnEquipWeaponFunction();

	bIsAttacking = false;
	AttackAlpha = 0.f;
	CachedLaunchDistance = 0.f;
	OnTimelineFinished();
}

void AWeapon_PunchGun::ApplyWorldState()
{
	Super::ApplyWorldState();
	bIsAttacking = false;
	AttackAlpha = 0.f;
	CachedLaunchDistance = 0.f;

	if (MeshPivot)
	{
		MeshPivot->SetRelativeLocation(MeshLocation);
		MeshPivot->SetRelativeRotation(MeshRotation);
	}

	if (GloveMesh)
	{
		GloveMesh->SetVisibility(true);
		GloveMesh->SetRelativeLocation(GloveBaseLocation);
	}

	if (SpringMesh)
	{
		SpringMesh->SetRelativeScale3D(SpringBaseScale);
		SpringMesh->SetRelativeLocation(SpringBaseLocation);
		SpringMesh->SetVisibility(false);
	}
}

void AWeapon_PunchGun::ApplyEquipState()
{
	Super::ApplyEquipState();
	if (MeshPivot) MeshPivot->SetRelativeRotation(EquipRotation);
}

void AWeapon_PunchGun::PlaySpringAttack(float LaunchDistance)
{
	CachedLaunchDistance = LaunchDistance;
	bIsAttacking = true;
	AttackAlpha = 0.f;
}


float AWeapon_PunchGun::GetGloveLaunchDistance()
{
	if (!WeaponData || !EquippedPlayer || !EquippedPlayer->NowMap) return 100.f;

	float AttackRealRange = EquippedPlayer->AStat.AttackRange * EquippedPlayer->NowMap->BlockSize;
	if (!GloveMesh || !Mesh || !GetWorld()) return AttackRealRange;

	FVector Dir = EquippedPlayer->BuildAttackAimPointForCurrentState();
	Dir.Z = 0.f;
	if (Dir.IsNearlyZero()) {
		Dir = EquippedPlayer->GetActorForwardVector();
		Dir.Z = 0.f;
	}
	Dir = Dir.GetSafeNormal();

	FVector AttackOrigin = EquippedPlayer->GetActorLocation();
	AttackOrigin.Z = GloveMesh->GetComponentLocation().Z;
	
	float ARadius = FMath::Max(1.f, EquippedPlayer->AStat.AttackRadius);
	FVector TraceStart = AttackOrigin + Dir * ARadius;
	const float AdjustedRange = FMath::Max(0.f, AttackRealRange - (ARadius * 2.f));
	FVector TraceEnd = TraceStart + Dir * AdjustedRange;
	

	FCollisionQueryParams Params(NAME_None, false);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(EquippedPlayer);
	Params.bFindInitialOverlaps = false;

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params)) TraceEnd = Hit.ImpactPoint - Dir * 5.f;
	TraceEnd -= Dir * 30.f;


	FVector GloveTipStart = GetForwardMostPointOnMesh(GloveMesh, Dir);

	float MaxDist = FVector::DotProduct(TraceEnd - GloveTipStart, Dir);
	MaxDist = FMath::Max(MaxDist, 0.f);

	return MaxDist;
}

FVector AWeapon_PunchGun::GetForwardMostPointOnMesh(UStaticMeshComponent* MeshComp, const FVector& ForwardDir)
{
	if (!MeshComp || !MeshComp->GetStaticMesh())
	{
		return MeshComp ? MeshComp->GetComponentLocation() : FVector::ZeroVector;
	}

	const FBox Box = MeshComp->GetStaticMesh()->GetBoundingBox();
	const FTransform Transform = MeshComp->GetComponentTransform();

	FVector BestPoint = MeshComp->GetComponentLocation();
	float BestDot = -FLT_MAX;

	for (int32 X = 0; X < 2; ++X)
	{
		for (int32 Y = 0; Y < 2; ++Y)
		{
			for (int32 Z = 0; Z < 2; ++Z)
			{
				const FVector LocalPoint(
					X == 0 ? Box.Min.X : Box.Max.X,
					Y == 0 ? Box.Min.Y : Box.Max.Y,
					Z == 0 ? Box.Min.Z : Box.Max.Z
				);

				const FVector WorldPoint = Transform.TransformPosition(LocalPoint);
				const float Dot = FVector::DotProduct(WorldPoint, ForwardDir);

				if (Dot > BestDot)
				{
					BestDot = Dot;
					BestPoint = WorldPoint;
				}
			}
		}
	}

	return BestPoint;
}

void AWeapon_PunchGun::OnTimelineUpdate(float Value)
{
	float Delta = Value * CachedLaunchDistance;
	float AxisWorldScale = 1.f;

	if (Mesh) {
		AxisWorldScale = Mesh->GetComponentTransform().TransformVector(FVector(0.f, 1.f, 0.f)).Size();
		AxisWorldScale = FMath::Max(AxisWorldScale, 0.001f);
	}

	float LocalDelta = Delta / AxisWorldScale;

	if (GloveMesh) GloveMesh->SetRelativeLocation(GloveBaseLocation + FVector(0.f, LocalDelta, 0.f));
	if (SpringMesh && SpringMeshNaturalY > 0.f) {
		if (LocalDelta < 0.01f) SpringMesh->SetVisibility(false);
		else {
			SpringMesh->SetVisibility(true);
			FVector Scale = SpringBaseScale;
			Scale.Y = LocalDelta / SpringMeshNaturalY;
			SpringMesh->SetRelativeScale3D(Scale);
			SpringMesh->SetRelativeLocation(SpringBaseLocation + FVector(0.f, LocalDelta / 2.f, 0.f));
		}
	}
}

void AWeapon_PunchGun::OnTimelineFinished()
{
	if (GloveMesh) GloveMesh->SetRelativeLocation(GloveBaseLocation);
	if (SpringMesh) {
		SpringMesh->SetRelativeScale3D(SpringBaseScale);
		SpringMesh->SetRelativeLocation(SpringBaseLocation);
		SpringMesh->SetVisibility(false);
	}
}

void AWeapon_PunchGun::Multicast_PlaySpringAttack_Implementation(float LaunchDistance)
{
	PlaySpringAttack(LaunchDistance);
}

void AWeapon_PunchGun::Server_TriggerSpringAttack_Implementation()
{
	Multicast_PlaySpringAttack(GetGloveLaunchDistance());
}

