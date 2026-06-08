// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerTransformationAnimation.generated.h"

/**
 * 
 */
class APlayer_Character;
class UPlayerTransformationComponent;

UCLASS()
class PROJECTCC_API UPlayerTransformationAnimation : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	void SetOwnerPlayer(APlayer_Character* Player) { OwnPlayer = Player; }
	void SetTransformationComponent(UPlayerTransformationComponent* Comp) { TransformationComp = Comp; }

	static float CalculateDirection2D(const FVector& Velocity, const FRotator& Rotation);

protected:
	UPROPERTY(BlueprintReadOnly, Category="Transformation Animation")
	TObjectPtr<APlayer_Character> OwnPlayer;

	UPROPERTY(BlueprintReadOnly, Category="Transformation Animation")
	TObjectPtr<UPlayerTransformationComponent> TransformationComp;

	UPROPERTY(BlueprintReadOnly, Category = "Transformation Animation")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Transformation Animation")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "Transformation Animation")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Transformation Animation")
	bool bIsDodging = false;

	UPROPERTY(BlueprintReadOnly, Category = "Transformation Animation")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Transformation Animation")
	bool bIsTransformed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Transformation Animation")
	float Direction = 0.f;
};
