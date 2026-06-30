// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_FootStep.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API UAnimNotify_FootStep : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	UPROPERTY(EditAnywhere, Category="FootStep")
	FName FootSocketName = TEXT("Foot_L");

	UPROPERTY(EditAnywhere, Category="FootStep")
	float TraceDistance = 50.f;
};
