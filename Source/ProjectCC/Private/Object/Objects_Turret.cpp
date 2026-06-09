// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_Turret.h"
#include "Components/BoxComponent.h"
#include "Player_Character.h"
#include "ObjectsDataAsset.h"
#include "MapConstructor.h"
#include "Kismet/GameplayStatics.h"
#include "Match_PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "PlayMode_Match.h"
//디버그드로우
#include "DrawDebugHelpers.h"

AObjects_Turret::AObjects_Turret(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UStaticMeshComponent>(TEXT("PhysicsCollider"))) {

	Type = EObjectsType::Install;

	AttackRangeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackRangeBox"));
	AttackRangeBox->SetupAttachment(RootComponent);

	AttackRangeBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AttackRangeBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackRangeBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AttackRangeBox->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap);

	AttackRangeBox->SetBoxExtent(FVector(250.f, 250.f, 150.f));

	TurretHeadPivot = CreateDefaultSubobject<USceneComponent>(TEXT("TurretHeadPivot"));
	TurretHeadPivot->SetupAttachment(RootComponent);

	TurretHeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretHeadMesh"));
	TurretHeadMesh->SetupAttachment(TurretHeadPivot);
	TurretHeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TurretHeadMesh->SetSimulatePhysics(false);

	PrimaryActorTick.bCanEverTick = true;
}

void AObjects_Turret::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UStaticMeshComponent* StaticCollider = Cast<UStaticMeshComponent>(PhysicsCollider);
	if (!StaticCollider) return;

	if (TurretCollisionMesh) {
		StaticCollider->SetStaticMesh(TurretCollisionMesh);
	}
	StaticCollider->SetHiddenInGame(true);
	StaticCollider->SetVisibility(true);

	if (TurretHeadMesh) TurretHeadMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
}

void AObjects_Turret::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AObjects_Turret, TurretHeadYaw);
}

void AObjects_Turret::ApplyAdditionalSetting() {
	Super::ApplyAdditionalSetting();

	if (HasAuthority() && !NowMap) {
		if (APlayMode_Match* GM = GetWorld()->GetAuthGameMode<APlayMode_Match>()) {
			NowMap = GM->GetCurrentMap();
		}
	}

	if (HasAuthority() && NowMap && Type == EObjectsType::Install) {
		float BoxExtent = (NowMap->BlockSize * 5.f) / 2.f;
		AttackRangeBox->SetBoxExtent(FVector(BoxExtent, BoxExtent, NowMap->BlockSize * 2.f));

		// 바인딩 되지 않았다면, 바인딩
		if (!AttackRangeBox->OnComponentBeginOverlap.IsAlreadyBound(this, &AObjects_Turret::OnAttackBoxBeginOverlap)) {
			AttackRangeBox->OnComponentBeginOverlap.AddDynamic(this, &AObjects_Turret::OnAttackBoxBeginOverlap);
		}

		if (PhysicsCollider) {
			PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel4);
		}

		AttackRangeBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		TryAttackTarget();
	}

}

void AObjects_Turret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority()) return;

	if (!CurrentTarget) {
		TryAttackTarget();
		return;
	}

	if (!IsValidTurretTarget(CurrentTarget)) {
		CurrentTarget = nullptr;
		TryAttackTarget();
		return;
	}

	UpdateHeadAim(DeltaTime);

	if (IsHeadFacingTarget(CurrentTarget)) FireAtTarget(CurrentTarget);

}

void AObjects_Turret::Func_Persist_Implementation(float DeltaTime)
{
	TryAttackTarget();
}

void AObjects_Turret::UpdateHeadAim(float DeltaTime)
{
	if (!TurretHeadPivot || !CurrentTarget) return;

	FVector ToTarget = CurrentTarget->GetActorLocation() - TurretHeadPivot->GetComponentLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero()) return;

	FRotator ToTargetRotation = ToTarget.Rotation();
	FRotator CurrentRotation = TurretHeadPivot->GetComponentRotation();

	FRotator TargetRotation = CurrentRotation;
	TargetRotation.Yaw = ToTargetRotation.Yaw;

	FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, DeltaTime, HeadTurnSpeed);
	TurretHeadYaw = NewRotation.Yaw;

	ApplyTurretHeadYaw(TurretHeadYaw);
	ForceNetUpdate();
}

bool AObjects_Turret::IsHeadFacingTarget(AActor* Target)
{
	if (!TurretHeadPivot || !Target) return false;

	FVector Forward = TurretHeadPivot->GetForwardVector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();

	FVector ToTarget = Target->GetActorLocation() - TurretHeadPivot->GetComponentLocation();
	ToTarget.Z = 0.f;
	ToTarget = ToTarget.GetSafeNormal();

	if (Forward.IsNearlyZero() || ToTarget.IsNearlyZero()) return false;

	float Dot = FVector::DotProduct(Forward, ToTarget);
	float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(AttackAimHalfAngleDegree));

	return Dot >= CosHalfAngle;
}

void AObjects_Turret::FireAtTarget(AActor* Target)
{
	if (!HasAuthority() || !Target) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastPassiveFunctionTime < FunctionInterval) return;
	if (!IsValidTurretTarget(Target)) {
		CurrentTarget = nullptr;
		return;
	}

	if (APlayer_Character* TargetPlayer = Cast<APlayer_Character>(Target)) {
		TargetPlayer->ApplyDamageInternal(Damage, OwnPlayer, this, true, true, false);
		LastPassiveFunctionTime = CurrentTime;
	}
	else if (AObjects* TargetObject = Cast<AObjects>(Target)) {
		TargetObject->ApplyDamageInternal(Damage, OwnPlayer, this, true, false);
		LastPassiveFunctionTime = CurrentTime;
	}
}

void AObjects_Turret::ApplyTurretHeadYaw(float NewYaw)
{
	if (!TurretHeadPivot) return;

	TurretHeadPivot->SetWorldRotation(FRotator(0.f, NewYaw, 0.f));
}

bool AObjects_Turret::IsValidTurretTarget(AActor* Target)
{
	if (!Target) return false;

	if (APlayer_Character* TargetPlayer = Cast<APlayer_Character>(Target)) {
		if (TargetPlayer->IsOut() || TargetPlayer->HP <= 0.f) return false;
		if (OwnPlayerController && TargetPlayer->GetController() == OwnPlayerController) return false;

		return true;
	}

	if (AObjects* TargetObject = Cast<AObjects>(Target)) {
		if (TargetObject == this) return false;
		if (OwnPlayerController && TargetObject->OwnPlayerController == OwnPlayerController) return false;
		if (!TargetObject->ObjectsData || !TargetObject->ObjectsData->bUseHP) return false;

		if (TargetObject->HP <= 0.f) return false;

		return true;
	}

	return false;
}


void AObjects_Turret::OnAttackBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (!HasAuthority())return;
	if (!IsValidTurretTarget(OtherActor)) return;

	TryAttackTarget();

}

void AObjects_Turret::TryAttackTarget() {

	if (!HasAuthority() || !AttackRangeBox)return;

	TArray<AActor*> OverlappingActors;
	AttackRangeBox->GetOverlappingActors(OverlappingActors);


	AActor* BestTarget = nullptr;
	float MinDistanceSq = TNumericLimits<float>::Max();
	FVector TurretLocation = GetActorLocation();

	// 시작점 높이를 절반만큼 올려 중심이 아닌 꼭대기에서 공격하는것으로 변경
	if (PhysicsCollider) {
		TurretLocation.Z += PhysicsCollider->Bounds.BoxExtent.Z;
	}

	//범위 내의 공격 대상(적)을 탐색
	for (AActor* Actor : OverlappingActors) {
		if (!IsValidTurretTarget(Actor)) continue;

		FVector StartLoc = TurretLocation;
		FVector EndLoc = Actor->GetActorLocation();
		float DistanceSq = FVector::DistSquared(StartLoc, EndLoc);

		if (DistanceSq >= MinDistanceSq) continue;

		//대상자 중 가장 가까운 대상을 타겟팅
		FHitResult HitResult;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this);

		if (OwnPlayerController && OwnPlayerController->GetPawn()) {
			TraceParams.AddIgnoredActor(OwnPlayerController->GetPawn());
		}

		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, TraceParams);
		bool bCanSeeTarget = false;

		if (!bHit) bCanSeeTarget = true;
		else {
			AActor* HitActor = HitResult.GetActor();
			if (HitActor == Actor) bCanSeeTarget = true;
		}

		if (!bCanSeeTarget) continue;

		MinDistanceSq = DistanceSq;
		BestTarget = Actor;
	}

	CurrentTarget = BestTarget;
}

void AObjects_Turret::OnRep_TurretHeadYaw()
{
	ApplyTurretHeadYaw(TurretHeadYaw);
}

