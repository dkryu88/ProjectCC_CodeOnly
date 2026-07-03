// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotify_PlayDodgeSound.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Player_Character.h"
#include "Sound/PlayerSoundDataAsset.h"

void UAnimNotify_PlayDodgeSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetWorld()) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	APlayer_Character* Player = Cast<APlayer_Character>(OwnerActor);
	if (!Player || !Player->PlayerSoundData) return;

	UPlayerSoundDataAsset* PlayerSoundData = Player->PlayerSoundData;
	if (USoundBase* FinalSound = PlayerSoundData->DodgeSound) {
		UGameplayStatics::PlaySoundAtLocation(MeshComp->GetWorld(), FinalSound, Player->GetActorLocation());
	}

}
