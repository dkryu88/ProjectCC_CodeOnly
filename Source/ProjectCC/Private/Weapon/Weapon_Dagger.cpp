// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_Dagger.h"
#include "Player_Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

float AWeapon_Dagger::OnPreHit(APlayer_Character* Target, bool& bSkipRotation)
{
	if (!Target) return 1.f;

	FVector PlayerForward = EquippedPlayer->GetActorForwardVector();
	FVector TargetForward = Target->GetActorForwardVector();

	float DotResult = FVector::DotProduct(PlayerForward, TargetForward);

	//Target의 뒤를 공격했을 경우 데미지 2배, 회전 비적용
	if (DotResult > BackAttackThreshold) {
		bSkipRotation = true;
		return BackAttackMuliplier;
	}

	//뒤가 아닌곳을 공격 시 일반 데미지, 회전 적용
	return Super::OnPreHit(Target, bSkipRotation);
}
