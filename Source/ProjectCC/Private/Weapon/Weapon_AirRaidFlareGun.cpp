// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_AirRaidFlareGun.h"
#include "Object/Objects_AirRaidFlareGun.h"
#include "MapConstructor.h"
#include "Player_Character.h"
#include "Components/CapsuleComponent.h"

bool AWeapon_AirRaidFlareGun::InteractionWeaponFunction(EFunctionInterActionReason Reason)
{
	if (Reason == EFunctionInterActionReason::Attack) {
		if (!HasAuthority()) return false;
		if (!EquippedPlayer || !WeaponData || !CheckUseCounting()) return false;
		if (!WeaponData->Bullet) return false;
		if (bPendingExplosion) return false;

		UWorld* World = GetWorld();
		if (!World) return false;

		bPendingExplosion = true;

		FVector SpawnLocation = EquippedPlayer->GetActorLocation() + FVector(0.f, 0.f, FlareSpawnZOffset);
		FRotator SpawnRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = EquippedPlayer;
		SpawnParams.Instigator = EquippedPlayer;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AObjects* SpawnedObject = World->SpawnActor<AObjects>(WeaponData->Bullet, SpawnLocation, SpawnRotation, SpawnParams);

		if (!SpawnedObject) {
			bPendingExplosion = false;
			return false;
		}

		AObjects_AirRaidFlareGun* FlareBullet = Cast<AObjects_AirRaidFlareGun>(SpawnedObject);

		if (!FlareBullet) {
			SpawnedObject->Destroy();
			bPendingExplosion = false;
			return false;
		}

		if (FlareBullet->GetObjectPhysicsCollider()) {
			FlareBullet->GetObjectPhysicsCollider()->IgnoreActorWhenMoving(this, true);
			EquippedPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(FlareBullet, true);
		}

		FVector CapturedForward = EquippedPlayer->GetActorForwardVector();
		CapturedForward.Z = 0.f;
		CapturedForward = CapturedForward.GetSafeNormal();

		FlareBullet->InitAirRaid(EquippedPlayer, CapturedForward);

		FVector UpTargetPoint = SpawnLocation + FVector(0.f, 0.f, 1000.f);
		FVector UpVelocity = FVector(0.f, 0.f, FlareBullet->Move_Speed > 0.f ? FlareBullet->Move_Speed : 600.f);

		FlareBullet->ShootOrThrowWithLaunchData(EquippedPlayer, SpawnLocation, UpTargetPoint, UpVelocity, false);
		FlareBullet->BeginAirRaidFlare();

		ApplyUseEffect();
		UseWeapon();

		bPendingExplosion = false;
		return false;
	}

	return true;
}

