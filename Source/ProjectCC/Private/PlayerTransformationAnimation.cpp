// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerTransformationAnimation.h"
#include "Player_Character.h"
#include "PlayerTransformationComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UPlayerTransformationAnimation::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (!OwnPlayer) {
		OwnPlayer = Cast<APlayer_Character>(TryGetPawnOwner());
	}
}

void UPlayerTransformationAnimation::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnPlayer) {
		OwnPlayer = Cast<APlayer_Character>(TryGetPawnOwner());
	}

	if (!OwnPlayer) return;

	FVector Velocity = OwnPlayer->GetVelocity();
	Velocity.Z = 0.f;

	GroundSpeed = Velocity.Size();
	bIsMoving = GroundSpeed > 5.f;

	if (UCharacterMovementComponent* MoveComp = OwnPlayer->GetCharacterMovement()) {
		bIsInAir = MoveComp->IsFalling();
	}

	bIsDodging = OwnPlayer->bIsDodging;
	bIsAiming = OwnPlayer->bIsAiming;

	if (TransformationComp) {
		bIsTransformed = TransformationComp->IsTransformed();
	}

	Direction = CalculateDirection2D(OwnPlayer->GetVelocity(), OwnPlayer->GetActorRotation());
}

float UPlayerTransformationAnimation::CalculateDirection2D(const FVector& Velocity, const FRotator& Rotation)
{
	FVector HorizontalVelocity = Velocity;
	HorizontalVelocity.Z = 0.f;

	if (HorizontalVelocity.IsNearlyZero()) return 0.f;

	FVector MoveDir = HorizontalVelocity.GetSafeNormal();
	FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	float ForwardValue = FVector::DotProduct(Forward, MoveDir);
	float RightValue = FVector::DotProduct(Right, MoveDir);

	return FMath::RadiansToDegrees(FMath::Atan2(RightValue, ForwardValue));

}

