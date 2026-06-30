// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotify_FootStep.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Player_Character.h"
#include "Sound/FootStepDataAsset.h"


void UAnimNotify_FootStep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetWorld()) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	APlayer_Character* Player = Cast<APlayer_Character>(OwnerActor);
	if (!Player || !Player->GetFootStepData()) return;

	FVector Start = MeshComp->GetSocketLocation(FootSocketName);
	FVector End = Start - FVector(0.f, 0.f, TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerActor);
	Params.bReturnPhysicalMaterial = true;

	bool bHit = MeshComp->GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	if (bHit && HitResult.PhysMaterial.IsValid()) {
		EPhysicalSurface StrikingSurface = HitResult.PhysMaterial->SurfaceType;

		UFootStepDataAsset* FootStepData = Player->GetFootStepData();
		if (TObjectPtr<USoundBase>* FoundSoundPtr = FootStepData->SurfaceSoundMap.Find(StrikingSurface)) {
			if (USoundBase* FinalSound = FoundSoundPtr->Get()) {
				UGameplayStatics::PlaySoundAtLocation(
					MeshComp->GetWorld(),
					FinalSound,
					HitResult.ImpactPoint
				);
			}
		}
	}
}
