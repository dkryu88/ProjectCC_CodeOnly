// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_RailGun.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "ObjectsDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

void AWeapon_RailGun::ExecuteRailGunContinuousAttack()
{
	if (!HasAuthority() || !EquippedPlayer) return;

	if (bMaxChargeFired) {
		ResetRailGunMoveSpeed();
		return;
	}

	if (bWaitingRepress) return;

	if (!CheckUseCounting()) {
		bWaitingRepress = true;
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();

	float ChargeDeltaTime = 0.f;

	if (LastChargeTime < 0.f)
	{
		ChargeDeltaTime = GetWorld()->GetDeltaSeconds();
	}
	else
	{
		ChargeDeltaTime = CurrentTime - LastChargeTime;
	}

	LastChargeTime = CurrentTime;

	ChargeGauge = FMath::Min(
		ChargeGauge + ChargeAmountPerSecond * ChargeSpeedPerSecond * ChargeDeltaTime,
		100.f
	);
	//게이지 충전량에 따라 조준 속도 변경 (점점 느려짐)
	EquippedPlayer->Aim_TurnSpeed = FMath::Lerp(WeaponData->Aim_TurnSpeed + 5, 150, ChargeGauge / 100);

	if (ChargeGauge >= 100.f && MaxChargeStartTime < 0.f) {
		ChargeGauge = 100.f;
		MaxChargeStartTime = CurrentTime;
	}

	if (MaxChargeStartTime >= 0.f) {
		float MaxHoldTime = CurrentTime - MaxChargeStartTime;

		if (MaxHoldTime >= MaxChargeHoldDuration) {
			bMaxChargeFired = true;
			ResetRailGunMoveSpeed();

			return;
		}
	}

	ApplyRailGunMoveSpeed();

	if (ChargeGauge < MinGauge) return;

	float Damage = GetDamageByGauge(ChargeGauge, WeaponData->Stats.Attack);
	float Radius = GetRadiusByGauge(ChargeGauge);

	PendingDamage = Damage;
	PendingRadius = Radius;

	FireRailGunBeam(PendingDamage, PendingRadius, ChargeGauge / 100);
}

void AWeapon_RailGun::CancelRailGunAttack()
{
	LastChargeTime = -1.f;
	MaxChargeStartTime = -1.f;

	ResetRailGunMoveSpeed();

	float SavedGauge = ChargeGauge;
	bool bWasMax = bMaxChargeFired;

	ChargeGauge = 0.f;
	bMaxChargeFired = false;
	bWaitingRepress = true;
	bRailGunCharging = false;

	if (bWasMax || SavedGauge >= MinGauge) {
		UseWeapon();
	}

	PendingDamage = 0.f;
	PendingRadius = 0.f;

}

void AWeapon_RailGun::ReleaseRailGunAttack()
{
	LastChargeTime = -1.f;
	MaxChargeStartTime = -1.f;

	ResetRailGunMoveSpeed();

	bWaitingRepress = false;
	bRailGunCharging = false;

	float SavedGauge = ChargeGauge;
	ChargeGauge = 0.f;

	bool bWasMax = bMaxChargeFired;
	bMaxChargeFired = false;

	if (bWasMax) {
		UseWeapon();
		if (!CheckUseCounting()) bWaitingRepress = true;
		return;
	}

	if (SavedGauge < MinGauge) {
		PendingDamage = 0.f;
		PendingRadius = 0.f;
		return;
	}

	float Damage = GetDamageByGauge(SavedGauge, WeaponData->Stats.Attack);
	float Radius = GetRadiusByGauge(SavedGauge);

	PendingDamage = Damage;
	PendingRadius = Radius;

	UseWeapon();

	FireRailGunBeam(Damage, Radius, SavedGauge / 100);

	PendingDamage = 0.f;
	PendingRadius = 0.f;

	if (!CheckUseCounting()) bWaitingRepress = true;
}

bool AWeapon_RailGun::BeforeAttackWeaponFunction()
{
	if (!EquippedPlayer) return false;
	if (!HasAuthority()) return true;
	if (bRailGunCharging) return true;
	if (bWaitingRepress) return false;
	if (!CheckUseCounting()) {
		bWaitingRepress = true;
		return false;
	}

	bRailGunCharging = true;

	ResetRailGunMoveSpeed();

	bWaitingRepress = false;
	LastChargeTime = -1.f;
	MaxChargeStartTime = -1.f;
	bMaxChargeFired = false;
	ChargeGauge = 0.f;
	PendingDamage = 0.f;
	PendingRadius = 0.f;

	return true;
}

bool AWeapon_RailGun::InteractionWeaponFunction(EFunctionInterActionReason Reason)
{
	switch (Reason)
	{
	case EFunctionInterActionReason::Attack:
		if (HasAuthority()) {
			ExecuteRailGunContinuousAttack();
		}
		return false;
	case EFunctionInterActionReason::Dodge:
	case EFunctionInterActionReason::InterAction:
	case EFunctionInterActionReason::Hitted:
	case EFunctionInterActionReason::Drop:
		if (HasAuthority())
		{
			CancelRailGunAttack();
		}

		return true;
	case EFunctionInterActionReason::Jump:
		// 레일건은 점프로 취소하지 않음
		return true;
	default:
		return true;
	}
}

void AWeapon_RailGun::ReleaseAttackWeaponFunction()
{
	if (!HasAuthority() || !EquippedPlayer) return;

	//충전 중이 아닌, 단순히 다시 누르기 대기 상태에서 Release 입력을 받았다면 대기상태 해제
	if (!bRailGunCharging && bWaitingRepress) {
		bWaitingRepress = false;
		return;
	}

	ReleaseRailGunAttack();

	if (EquippedPlayer->bNowHoldingAttack)
	{
		EquippedPlayer->bNowHoldingAttack = false;
		EquippedPlayer->LastAttackTime = GetWorld()->GetTimeSeconds();
	}
}

void AWeapon_RailGun::AdditionalUnEquipWeaponFunction()
{
	if (!HasAuthority() || !EquippedPlayer) return;

	CancelRailGunAttack();

	if (EquippedPlayer->bNowHoldingAttack) {
		EquippedPlayer->bNowHoldingAttack = false;
		EquippedPlayer->LastAttackTime = GetWorld()->GetTimeSeconds();
	}
}

void AWeapon_RailGun::FireRailGunBeam(float Damage, float Radius, float GaugePercent)
{
	if (!HasAuthority() || !EquippedPlayer || !EquippedPlayer->NowMap) return;

	float AttackRealRange = WeaponData->Stats.AttackRange * EquippedPlayer->NowMap->BlockSize;
	float RailGunAttackRealRange = FMath::Lerp(AttackRealRange * 0.25f, AttackRealRange, GaugePercent);
	FVector TraceStart = EquippedPlayer->GetActorLocation();
	FVector TraceEnd = TraceStart + EquippedPlayer->GetActorForwardVector() * RailGunAttackRealRange;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(EquippedPlayer);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel4));

	TArray<FHitResult> TraceHits;

	UKismetSystemLibrary::SphereTraceMultiForObjects(
		this,
		TraceStart,
		TraceEnd,
		FMath::Max(1.f, Radius),
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		TraceHits,
		true
	);

	TSet<AActor*> HitActors;
	TArray<APlayer_Character*> TargetPlayers;
	TArray<AObjects*> TargetObjects;

	for (const FHitResult& Hit : TraceHits) {
		AActor* HitActor = Hit.GetActor();

		if (!HitActor) continue;
		if (HitActors.Contains(HitActor)) continue;

		HitActors.Add(HitActor);

		if (APlayer_Character* TargetPlayer = Cast<APlayer_Character>(HitActor)) {
			if (TargetPlayer == EquippedPlayer) continue;
			if (TargetPlayer->IsOut()) continue;

			TargetPlayers.AddUnique(TargetPlayer);
		}
		else if (AObjects* TargetObject = Cast<AObjects>(HitActor)) {
			if (!TargetObject->ObjectsData) continue;
			if (!TargetObject->ObjectsData->bUseHP) continue;

			TargetObjects.AddUnique(TargetObject);
		}
	}

	if (TargetPlayers.Num() == 0 && TargetObjects.Num() == 0) {
		PendingDamage = 0.f;
		return;
	}

	//플레이어 AttackInternal과 동일
	auto ApplyToPlayer = [&](APlayer_Character* TargetPlayer) {
		if (!TargetPlayer) return;
		TargetPlayer->ApplyDamageInternal(Damage, EquippedPlayer, nullptr, WeaponData->bApplyKnockBack, WeaponData->bApplyRotation, false);
		ApplyHitEffect(TargetPlayer);
	};

	auto ApplyToObjects = [&](AObjects* TargetObject) {
		if (!TargetObject) return;
		TargetObject->ApplyDamageInternal(Damage, EquippedPlayer, nullptr, true, false);
		ApplyHitEffect(TargetObject);
	};

	switch (WeaponData->Stats.AttackTargetType)
	{
		case EAttackTargetType::SingleTarget:
		{
			AActor* ClosestActor = nullptr;
			float ClosestDistSq = TNumericLimits<float>::Max();

			for (APlayer_Character* Player : TargetPlayers)
			{
				const float DistSq = FVector::DistSquared(
					EquippedPlayer->GetActorLocation(),
					Player->GetActorLocation()
				);

				if (DistSq < ClosestDistSq)
				{
					ClosestDistSq = DistSq;
					ClosestActor = Player;
				}
			}

			for (AObjects* Obj : TargetObjects)
			{
				const float DistSq = FVector::DistSquared(
					EquippedPlayer->GetActorLocation(),
					Obj->GetActorLocation()
				);

				if (DistSq < ClosestDistSq)
				{
					ClosestDistSq = DistSq;
					ClosestActor = Obj;
				}
			}

			if (APlayer_Character* Player = Cast<APlayer_Character>(ClosestActor))
			{
				ApplyToPlayer(Player);
			}
			else if (AObjects* Obj = Cast<AObjects>(ClosestActor))
			{
				ApplyToObjects(Obj);
			}

			break;
		}

		case EAttackTargetType::MultiTarget:
		{
			for (APlayer_Character* Player : TargetPlayers)
			{
				ApplyToPlayer(Player);
			}

			for (AObjects* Obj : TargetObjects)
			{
				ApplyToObjects(Obj);
			}

			break;
		}
	}

}

float AWeapon_RailGun::GetDamageByGauge(float Gauge, float MaxDamage)
{
	if (Gauge < 75.f) {
		return FMath::Lerp(MaxDamage * MinDamageRatio, MaxDamage, FMath::Clamp((Gauge - MinGauge) / (75.f - MinGauge), 0.f, 1.f));
	}

	return MaxDamage;
}

float AWeapon_RailGun::GetRadiusByGauge(float Gauge)
{
	float RadiusFactor = FMath::Clamp((Gauge - MinGauge) / (75.f - MinGauge), 0.f, 1.f);

	return FMath::Lerp(MinBeamRadius, MaxBeamRadius, RadiusFactor);
}

void AWeapon_RailGun::ApplyRailGunMoveSpeed()
{
	if (!EquippedPlayer || !EquippedPlayer->GetCharacterMovement()) return;

	float SpeedScale = FMath::GetMappedRangeValueClamped(FVector2D(MinGauge, 75.f), FVector2D(0.3f, 0.f), ChargeGauge);
	//기존에 있던 스피드 컨트롤러 제거
	EquippedPlayer->RemoveSpeedControllerByName("RailGunSpeed");
	//새로운 스피드 컨트롤러로 적용
	EquippedPlayer->AddSpeedController("RailGunSpeed", SpeedScale, 0.f);
}

void AWeapon_RailGun::ResetRailGunMoveSpeed() {
	if (!EquippedPlayer || !EquippedPlayer->GetCharacterMovement()) return;

	EquippedPlayer->RemoveSpeedControllerByName("RailGunSpeed");
	EquippedPlayer->Aim_TurnSpeed = WeaponData->Aim_TurnSpeed;
}
