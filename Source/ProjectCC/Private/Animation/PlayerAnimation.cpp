// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/PlayerAnimation.h"
#include "GameFramework/Pawn.h"
#include "Player_Character.h"

void UPlayerAnimation::NativeUpdateAnimation(float DeltaSeconds) {
	Super::NativeUpdateAnimation(DeltaSeconds);

	APlayer_Character* PlayerCharacter = Cast<APlayer_Character>(TryGetPawnOwner());

	if (!PlayerCharacter) {
		CurrentGripSequence = nullptr;
		bHasGrip = false;
		GripAlpha = 0.f;
		return;
	}

	CurrentGripSequence = PlayerCharacter->GetCurrentGripSequence();
	bHasGrip = CurrentGripSequence != nullptr;
	GripAlpha = bHasGrip ? 1.f : 0.f;
}
