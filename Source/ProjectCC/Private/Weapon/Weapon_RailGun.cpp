// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_RailGun.h"
#include "Player_Character.h"
#include "Net/UnrealNetwork.h"
#include "MapConstructor.h"
#include "ObjectsDataAsset.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
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
			LastChargeTime = -1.f;
			MaxChargeStartTime = -1.f;

			ResetRailGunMoveSpeed();

			bRailGunCharging = false;
			bWaitingRepress = true;
			bMaxChargeFired = true;
			
			ChargeGauge = 0.f;
			PendingDamage = 0.f;
			PendingRadius = 0.f;

			RailGunAnimState = ERailGunAnimState::None;

			float CompleteAnimDuration = 0.25f;

			if (RailGunCompleteAnimation)
			{
				CompleteAnimDuration = RailGunCompleteAnimation->GetPlayLength();
				PlayRailGunDynamicAnimation(RailGunCompleteAnimation, 1);
			}
			else
			{
				StopRailGunAnimation();
			}

			UseWeapon();

			if (!CheckUseCounting()) bWaitingRepress = true;

			InstantHideRailGunPreview(CompleteAnimDuration);
			ForceNetUpdate();

			return;
		}
	}

	ApplyRailGunMoveSpeed();
	RefreshRailGunAnimationSlot();

	if (ChargeGauge < MinGauge) return;

	//레일건 공격이 시작되면 공격 애니메이션 재생 (공격 끝까지 무한반복)
	if (RailGunAnimState != ERailGunAnimState::Firing) {
		RailGunAnimState = ERailGunAnimState::Firing;
		PlayRailGunDynamicAnimation(RailGunAttackAnimation, 999999);
	}

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

	StopRailGunAnimation();
	ResetRailGunMoveSpeed();

	float SavedGauge = ChargeGauge;
	bool bWasMax = bMaxChargeFired;

	ChargeGauge = 0.f;
	bMaxChargeFired = false;
	bWaitingRepress = false;
	bRailGunCharging = false;
	bNoShowingRailGunPreview = false;

	GetWorldTimerManager().ClearTimer(InstantHideRailGunPreviewTimerHandle);

	if (bWasMax || SavedGauge >= MinGauge) {
		UseWeapon();
	}

	PendingDamage = 0.f;
	PendingRadius = 0.f;

}

void AWeapon_RailGun::InstantHideRailGunPreview(float Duration)
{
	if (!HasAuthority()) return;
	bNoShowingRailGunPreview = true;

	if (EquippedPlayer && EquippedPlayer->IsLocallyControlled()) EquippedPlayer->HideAimPreview();
	ForceNetUpdate();

	GetWorldTimerManager().ClearTimer(InstantHideRailGunPreviewTimerHandle);
	GetWorldTimerManager().SetTimer(InstantHideRailGunPreviewTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
		bNoShowingRailGunPreview = false;
		ForceNetUpdate();
	}), FMath::Max(0.05f, Duration), false);
}

void AWeapon_RailGun::PlayRailGunDynamicAnimation(UAnimSequence* Sequence, int32 LoopCount)
{
	if (!HasAuthority()) return;
	if (!EquippedPlayer) return;
	if (!Sequence) return;

	FName RailGunSlotName = GetAnimationSlotName();

	if (!CurrentAnimSlot.IsNone() && CurrentAnimSlot != RailGunSlotName) {
		EquippedPlayer->Multicast_StopSlotAnimation(CurrentAnimSlot, 0.05);
	}

	CurrentAnimSlot = RailGunSlotName;

	EquippedPlayer->Multicast_StopSlotAnimation(RailGunSlotName, 0.01f);
	EquippedPlayer->Multicast_PlayAnimationDynamic(Sequence, RailGunSlotName, 0.1f, 0.1f, 1.f, LoopCount, 0);
}

void AWeapon_RailGun::StopRailGunAnimation()
{
	if (!HasAuthority()) return;
	if (!EquippedPlayer) return;

	if (!CurrentAnimSlot.IsNone()) {
		EquippedPlayer->Multicast_StopSlotAnimation(CurrentAnimSlot, 0.15f);
	}

	EquippedPlayer->Multicast_StopSlotAnimation(FName(TEXT("UpperBody")), 0.15f);
	EquippedPlayer->Multicast_StopSlotAnimation(FName(TEXT("DefaultSlot")), 0.15f);

	CurrentAnimSlot = NAME_None;
	RailGunAnimState = ERailGunAnimState::None;
}

void AWeapon_RailGun::OnRep_RailGunCharging()
{
	if(bRailGunCharging){
		bLocalNoShowingRailGunPreview = true;

		if (bRailGunCharging && EquippedPlayer && EquippedPlayer->IsLocallyControlled()) {
			EquippedPlayer->HideAimPreview();
		}
	}
	else {
		bLocalNoShowingRailGunPreview = false;
	}
}

void AWeapon_RailGun::OnRep_NoShowingRailGunPreview()
{
	if (bNoShowingRailGunPreview && EquippedPlayer && EquippedPlayer->IsLocallyControlled()) {
		EquippedPlayer->HideAimPreview();
	}
}

FName AWeapon_RailGun::GetAnimationSlotName()
{
	if (!EquippedPlayer) return FName(TEXT("UpperBody"));

	FVector Velocity2D = EquippedPlayer->GetVelocity();
	Velocity2D.Z = 0.f;

	bool bStandingStill = Velocity2D.SizeSquared() < FMath::Square(10.f);

	if (bStandingStill) return FName(TEXT("DefaultSlot"));

	return FName(TEXT("UpperBody"));
}

void AWeapon_RailGun::RefreshRailGunAnimationSlot()
{
	if (!HasAuthority()) return;
	if (!EquippedPlayer) return;
	if (RailGunAnimState == ERailGunAnimState::None) return;

	FName DesiredSlot = GetAnimationSlotName();

	if (CurrentAnimSlot == DesiredSlot) return;

	UAnimSequence* CurrentSequence = nullptr;

	switch (RailGunAnimState) {
	case ERailGunAnimState::Charging:
		CurrentSequence = RailGunChargeAnimation;
		break;
	case ERailGunAnimState::Firing:
		CurrentSequence = RailGunAttackAnimation;
		break;
	default:
		break;
	}

	if (CurrentSequence) PlayRailGunDynamicAnimation(CurrentSequence, 999999);
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

	bool bPlayFireEnd = bWasMax || SavedGauge >= 60.f;

	if (bPlayFireEnd) {
		RailGunAnimState = ERailGunAnimState::None;
		float CompleteAnimDuration = 0.25f;

		if (RailGunCompleteAnimation) {
			CompleteAnimDuration = RailGunCompleteAnimation->GetPlayLength();
			PlayRailGunDynamicAnimation(RailGunCompleteAnimation, 1);
		}

		InstantHideRailGunPreview(CompleteAnimDuration);
	}
	else {
		StopRailGunAnimation();
		InstantHideRailGunPreview(0.25f);
	}

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

void AWeapon_RailGun::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon_RailGun, ChargeGauge);
	DOREPLIFETIME(AWeapon_RailGun, bRailGunCharging);
	DOREPLIFETIME(AWeapon_RailGun, bNoShowingRailGunPreview);
}

bool AWeapon_RailGun::BeforeAttackWeaponFunction()
{
	if (!EquippedPlayer) return false;
	if (bRailGunCharging) return true;
	if (bWaitingRepress) return false;
	if (!HasAuthority()) {
		//공격 버튼을 누르는 순간 차징중으로 판단 (Preview가 보이지 않음)
		bLocalNoShowingRailGunPreview = true;
		bRailGunCharging = true;
		EquippedPlayer->HideAimPreview();
		return true;
	}
	if (!CheckUseCounting()) {
		bWaitingRepress = true;
		return false;
	}

	bRailGunCharging = true;
	RailGunAnimState = ERailGunAnimState::Charging;

	//레일건 차징 애니메이션 재생(무한 루프)
	PlayRailGunDynamicAnimation(RailGunChargeAnimation, 999999);

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
		bMaxChargeFired = false;
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

bool AWeapon_RailGun::BuildAimPreviewData(APlayer_Character* Player, FAimPreviewVisualData& PreviewData)
{
	PreviewData.Reset();

	if (!Player) return false;
	if (!Player->NowMap) return false;
	if (!WeaponData) return false;
	if (!CheckUseCounting()) {
		PreviewData.Reset();
		return true;
	}

	if (bLocalNoShowingRailGunPreview || bNoShowingRailGunPreview || bWaitingRepress || bRailGunCharging || bMaxChargeFired) {
		PreviewData.Reset();
		return true;
	}

	float TheBlockSize = Player->NowMap->BlockSize;
	float AttackRealRange = (WeaponData->Stats.AttackRange) * TheBlockSize;
	float GaugePercent = FMath::Clamp(ChargeGauge / 100.f, 0.f, 1.f);
	float RailGunAttackRealRange = FMath::Lerp(AttackRealRange * 0.25f, AttackRealRange, GaugePercent);
	float Radius = ChargeGauge >= MinGauge ? GetRadiusByGauge(ChargeGauge) : MinBeamRadius;
	float MaxRange = WeaponData->Stats.AttackRange * TheBlockSize;
	float MinRange = FMath::Lerp(MaxRange * 0.25f, MaxRange, FMath::Clamp(MinGauge / 100.f, 0.f, 1.f));

	PreviewData.bVisible = true;

	PreviewData.bShowAttackSector = false;
	PreviewData.bShowAttackPath = true;
	PreviewData.bShowAttackSecondPath = true;
	PreviewData.bShowAttackRangeCircle = false;

	PreviewData.PreviewRange = MaxRange;
	PreviewData.SecondPreviewRange = MinRange;
	PreviewData.PathRadius = MaxBeamRadius;
	PreviewData.SecondPathRadius = MinBeamRadius;
	PreviewData.PreviewRadius = MaxBeamRadius;

	PreviewData.HalfAngleDegree = 0.f;
	PreviewData.AttackDirection = EWeaponAttackDirection::Horizontal;

	PreviewData.bOnlySameHeight = true;
	PreviewData.bBlockByWall = true;

	return PreviewData.CheckUsingAnyVisual();
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

	//벽에 막히는 지 검사-----------------------------
	FHitResult WallHit;
	FCollisionQueryParams Params;
	Params.bTraceComplex = false;
	Params.AddIgnoredActor(EquippedPlayer);
	if (EquippedPlayer->NowWeapon) {
		Params.AddIgnoredActor(EquippedPlayer->NowWeapon);
	}
	if (EquippedPlayer->NowObjects) {
		Params.AddIgnoredActor(EquippedPlayer->NowObjects);
	}
	if (EquippedPlayer->NowSupport) {
		Params.AddIgnoredActor(EquippedPlayer->NowSupport);
	}
	FCollisionObjectQueryParams WallObjectParams;
	WallObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

	bool bWallHit = GetWorld()->LineTraceSingleByObjectType(WallHit, TraceStart + FVector(0.f, 0.f, 80.f), TraceEnd + FVector(0.f, 0.f, 80.f), WallObjectParams, Params);
	if (bWallHit) {
		TraceEnd = WallHit.ImpactPoint;
	}

	//--------------------------------------------------

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

		//벽 뒤의 적은 무시
		if (EquippedPlayer && !EquippedPlayer->AttackLineOfSight(HitActor)) continue;

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
		TargetPlayer->ApplyDamageInternal(Damage, EquippedPlayer, nullptr, WeaponData->bApplyKnockBack, WeaponData->bApplyRotation, WeaponData->bUsingHitAction, false);
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
