// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/Title_PlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"

// Sets default values
ATitle_PlayerCharacter::ATitle_PlayerCharacter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	PreviewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Preview Character"));
	PreviewMesh->SetupAttachment(RootScene);

	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetGenerateOverlapEvents(false);
	//네비게이션에 영향 없음
	PreviewMesh->SetCanEverAffectNavigation(false);

	SetActorEnableCollision(false);
}

void ATitle_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultYaw = GetActorRotation().Yaw;
}

void ATitle_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bReturnToFront) return;

	FRotator CurrentRotator = GetActorRotation();
	FRotator TargetRotator(0.f, DefaultYaw, 0.f);

	FRotator NewRotator = FMath::RInterpTo(CurrentRotator, TargetRotator, DeltaTime, ReturnRotateSpeed);
	SetActorRotation(NewRotator);

	float RemainYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(NewRotator.Yaw, DefaultYaw));
	if (RemainYaw < 0.5f) {
		SetActorRotation(TargetRotator);
		bReturnToFront = false;
	}
}

//캐릭터 Mesh를 YawChange(마우스 드래그) 만큼 회전
void ATitle_PlayerCharacter::AddPreviewYaw(float YawChange) {
	bReturnToFront = false;

	FRotator Rotator = GetActorRotation();
	Rotator.Yaw += YawChange * -DragRotateSpeed;
	SetActorRotation(Rotator);
}

void ATitle_PlayerCharacter::TurnToFront()
{
	bReturnToFront = true;
}

void ATitle_PlayerCharacter::StopTurnToFront()
{
	bReturnToFront = false;
}
