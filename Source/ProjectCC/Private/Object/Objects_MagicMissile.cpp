// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_MagicMissile.h"
#include "Player_Character.h"
#include "PlayerTransformationComponent.h"
#include "PlayerTransformationDataAsset.h"
#include "PlayerConditionComponent.h"

AObjects_MagicMissile::AObjects_MagicMissile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bReplicates = true;
}

//날아가는 도중 계속해서 회전하도록 함
void AObjects_MagicMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (MeshPivot) {
		MeshPivot->AddLocalRotation(FRotator(0.f, 0.f, 360.f * DeltaTime));
	}
}

void AObjects_MagicMissile::Func_HitPlayer_Implementation(APlayer_Character* HitPlayer) {
	Super::Func_HitPlayer_Implementation(HitPlayer);

	if (!HasAuthority()) return;
	if (!TransformationData) return;

	if (HitPlayer && !HitPlayer->IsOut()) {
		UPlayerTransformationComponent* TransComp = HitPlayer->FindComponentByClass<UPlayerTransformationComponent>();
		if (TransComp) {
			TransComp->StartTransformation(TransformationData, OwnPlayer, -1.f);
		}	
	}

}
