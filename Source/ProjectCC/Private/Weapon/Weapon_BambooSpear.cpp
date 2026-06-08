// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_BambooSpear.h"
#include "Player_Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

float AWeapon_BambooSpear::OnPreHit(APlayer_Character* Target, bool& bSkipRotation)
{
	if (!Target) return 1.f;

	APlayer_State* PlayerState = Cast<APlayer_State>(EquippedPlayer->GetPlayerState());
	APlayer_State* TargetState = Cast<APlayer_State>(Target->GetPlayerState());

	if (!PlayerState || !TargetState) return 1.f;

	int32 PlayerCoin = PlayerState->GetPlayerCoin();
	int32 TargetCoin = TargetState->GetPlayerCoin();
	float Magnification = 1.f;

	//Target의 코인과 장착자의 코인이 특정 개수 이상이고, 장착자의 코인이 특정 개수 이하일때 공격 배수 적용
	if (TargetCoin - PlayerCoin >= 30 && PlayerCoin <= 10) {
		Magnification = AttackMagnification;
	}

	//뒤가 아닌곳을 공격 시 일반 데미지, 회전 적용
	return Magnification;
}
