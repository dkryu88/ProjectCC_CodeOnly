// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotify_ActionSound.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player_Character.h"
#include "Sound/FootStepDataAsset.h"

void UAnimNotify_ActionSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	/*if (!MeshComp || !MeshComp->GetWorld()) return;

	APlayer_Character* Player = Cast<APlayer_Character>(MeshComp->GetOwner());
	if (!Player || !Player->GetFootStepData()) return;

	UCharacterMovementComponent* MoveComp = Player->GetCharacterMovement();
	if (!MoveComp) return;

	EPhysicalSurface StrikingSurface = SurfaceType_Default;
	if (MoveComp->CurrentFloor.bBlockingHit && MoveComp->CurrentFloor.HitResult.PhysMaterial.IsValid()) {
		StrikingSurface = MoveComp->CurrentFloor.HitResult.PhysMaterial->SurfaceType;
	}

	UFootStepDataAsset* DataAsset = Player->GetFootStepData();
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<USoundBase>>* TargetMap = nullptr;

	switch (ActionReason) {
	case EFunctionInterActionReason::Jump:
		TargetMap = &DataAsset->JumpSoundMap;
		break;
	case EFunctionInterActionReason::Dodge:
		TargetMap = &DataAsset->DodgeSoundMap;
		break;
	default:
		return;
	}

	if (TargetMap) {
		if (TObjectPtr<USoundBase>* FoundSoundPtr = TargetMap->Find(StrikingSurface)) {
			if (USoundBase* FinalSound = FoundSoundPtr->Get()) {
				UGameplayStatics::PlaySoundAtLocation(
					MeshComp->GetWorld(),
					FinalSound,
					Player->GetActorLocation()
				);
			}
		}
	}*/
}
