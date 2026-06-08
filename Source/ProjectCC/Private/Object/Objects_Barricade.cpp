// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_Barricade.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

void AObjects_Barricade::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;
	UWorld* World = GetWorld();
	if (!World) return;

	FVector StartLoc = GetActorLocation();

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	ECollisionChannel TraceChannel = ECC_WorldStatic;

	FHitResult HitResult;
	bool bHitX = false;
	bool bHitY = false;

	if (World->LineTraceSingleByChannel(HitResult, StartLoc, StartLoc + FVector(TraceDistance, 0.f, 0.f), TraceChannel, TraceParams)) bHitX = true;
	if (World->LineTraceSingleByChannel(HitResult, StartLoc, StartLoc + FVector(-TraceDistance, 0.f, 0.f), TraceChannel, TraceParams)) bHitX = true;
	if (World->LineTraceSingleByChannel(HitResult, StartLoc, StartLoc + FVector(0.f, TraceDistance, 0.f), TraceChannel, TraceParams)) bHitY = true;
	if (World->LineTraceSingleByChannel(HitResult, StartLoc, StartLoc + FVector(0.f, -TraceDistance, 0.f), TraceChannel, TraceParams)) bHitY = true;

	float TargetYaw = 0.f;

	if (bHitY && !bHitX)TargetYaw = 0.f;
	else if (bHitX && !bHitY)TargetYaw = 0.f;
	else TargetYaw = FMath::RandBool() ? 0.f : 90.f;

	SetActorRotation(FRotator(0.f, TargetYaw, 0.f));
}

void AObjects_Barricade::ApplyAdditionalSetting()
{
	if (!PhysicsCollider || !HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	PhysicsCollider->SetSimulatePhysics(true);
	PhysicsCollider->SetEnableGravity(true);
	
	//넘어지는건 허용, 쉽게 회전하지 않도록 Damping 증가
	PhysicsCollider->SetAngularDamping(12.f);
	//밀렸을 때 쉽게 밀리지 않도록 Damping 증가
	PhysicsCollider->SetLinearDamping(1.5f);
	//질량 증가
	PhysicsCollider->SetMassOverrideInKg(NAME_None, 150.f, true);
	//무게 중심을 아래로 이동
	PhysicsCollider->SetCenterOfMass(FVector(0.f, 0.f, -25.f), NAME_None);
	//회전 관성 증가
	PhysicsCollider->BodyInstance.InertiaTensorScale = FVector(8.f, 8.f, 1.f);
	PhysicsCollider->BodyInstance.UpdateMassProperties();
}
