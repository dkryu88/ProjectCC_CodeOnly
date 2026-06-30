// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Player_FunctionInterActionReason.h"	//[Ãß°¡]
#include "AnimNotify_ActionSound.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Action Sound Notify"))
class PROJECTCC_API UAnimNotify_ActionSound : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	UPROPERTY(EditAnywhere, Category = "ActionSound")
	EFunctionInterActionReason ActionReason = EFunctionInterActionReason::Jump;
};
