// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_SuperHammer.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "PlayerConditionDataAsset.h"
#include "TimerManager.h"
#include "InputActionValue.h"

AWeapon_SuperHammer::AWeapon_SuperHammer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UStaticMeshComponent>((TEXT("WeaponPhysicsCollider"))))
{
}

void AWeapon_SuperHammer::EquipEffect_Implementation(APlayer_Character* Player) {
	OwnPlayer = Player;
	if (OwnPlayer) {
		OwnPlayer->bInteractionLock = true;
		OwnPlayer->bEquipLock = true;
		OwnPlayer->bIsDodgeLocked = true;

		if (OwnPlayer->ConditionComp && DamageImmunityDataAsset) {
			OwnPlayer->ConditionComp->ApplyCondition(DamageImmunityDataAsset, OwnPlayer, Duration);
		}

		//자동공격 타이머
		GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AWeapon_SuperHammer::AutoAttack, 0.1f, true, 0.1f);
		//무기 지속시간 타이머
		GetWorldTimerManager().SetTimer(ExpireTimerHandle, this, &AWeapon_SuperHammer::FinishHammerDuration, Duration + 0.2f, false);
	}
}

void AWeapon_SuperHammer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UStaticMeshComponent* StaticCollider = Cast<UStaticMeshComponent>(WeaponCollider);
	if (!StaticCollider) return;

	if (SuperHammerCollisionMesh) {
		StaticCollider->SetStaticMesh(SuperHammerCollisionMesh);
	}

	//매쉬 콜라이더가 실제로 보이지 않도록 함
	StaticCollider->SetHiddenInGame(true);

	StaticCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticCollider->SetCollisionObjectType(ECC_GameTraceChannel5);
	StaticCollider->SetCollisionResponseToAllChannels(ECR_Block);
	StaticCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	StaticCollider->SetNotifyRigidBodyCollision(true);
	StaticCollider->SetAllUseCCD(true);
}

void AWeapon_SuperHammer::Destroyed()
{
	if (OwnPlayer) {
		OwnPlayer->bInteractionLock = false;
		OwnPlayer->bEquipLock = false;
		OwnPlayer->bIsDodgeLocked = false;
	}

	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(ExpireTimerHandle);
	Super::Destroyed();
}

bool AWeapon_SuperHammer::InteractionWeaponFunction(EFunctionInterActionReason Reason)
{
	if (Reason == EFunctionInterActionReason::Attack) {
		AttackCount -= 1;
		return true;
	}
	return true;
}

void AWeapon_SuperHammer::AutoAttack()
{
	if (OwnPlayer && !OwnPlayer->IsOut()) {
		this->NowUseCount += 1;
		OwnPlayer->Attack(FInputActionValue());
		if (AttackCount <= 0) {
			FinishHammerDuration();
		}
	}
}

void AWeapon_SuperHammer::FinishHammerDuration()
{
	if (OwnPlayer) {
		OwnPlayer->bInteractionLock = false;
		OwnPlayer->bIsDodgeLocked = false;
	}

	this->NowUseCount = 0;
	UseWeapon();
}


