// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlayFootStepSound.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API UAnimNotify_PlayFootStepSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	UPROPERTY(EditAnywhere, Category="FootStep")
	FName FootSocketName = TEXT("SK_PlayerFoot_Left");

	UPROPERTY(EditAnywhere,Category="FootStep")
	float TraceDistance = 50.f;
};
