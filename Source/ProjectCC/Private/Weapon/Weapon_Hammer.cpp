// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_Hammer.h"
#include "Components/StaticMeshComponent.h"
#include "Player_Character.h"

AWeapon_Hammer::AWeapon_Hammer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UStaticMeshComponent>((TEXT("WeaponPhysicsCollider"))))
{
}

void AWeapon_Hammer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UStaticMeshComponent* StaticCollider = Cast<UStaticMeshComponent>(WeaponCollider);
	if (!StaticCollider) return;

	if (HammerCollisionMesh) {
		StaticCollider->SetStaticMesh(HammerCollisionMesh);
	}

	//매쉬 콜라이더가 실제로 보이지 않도록 함
	StaticCollider->SetHiddenInGame(true);
	StaticCollider->SetVisibility(false);

	StaticCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticCollider->SetCollisionObjectType(ECC_GameTraceChannel5);
	StaticCollider->SetCollisionResponseToAllChannels(ECR_Block);
	StaticCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	StaticCollider->SetNotifyRigidBodyCollision(true);
	StaticCollider->SetAllUseCCD(true);
}





