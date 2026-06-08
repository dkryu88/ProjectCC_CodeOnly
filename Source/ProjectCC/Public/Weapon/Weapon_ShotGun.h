// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_ShotGun.generated.h"

/**
 * 
 */
class APlayer_Character;

UCLASS()
class PROJECTCC_API AWeapon_ShotGun : public AWeapon
{
	GENERATED_BODY()

public:
	// BP에서 할당할 샷건 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ShotGun")
	USkeletalMeshComponent* SkeletalMesh;

	// 한 번 발사 시 나가는 탄알 수 (BP에서 조정 가능)
	UPROPERTY(EditDefaultsOnly, Category = "ShotGun")
	int32 PelletCount = 10;

	// AttackInternal에서 Cast 후 호출 — 탄알별 LineTrace 수행 후 피격 여부 반환
	bool FirePellets();

	virtual bool InteractionWeaponFunction(EFunctionInterActionReason Reason) override;
};
