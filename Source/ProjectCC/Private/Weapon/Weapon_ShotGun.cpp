// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_ShotGun.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "Objects.h"
#include "ObjectsDataAsset.h"
#include "WeaponDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"


bool AWeapon_ShotGun::InteractionWeaponFunction (EFunctionInterActionReason Reason)
{
	if (Reason == EFunctionInterActionReason::Attack) {
		if (FirePellets()) return false;
	}
	return true;
}

bool AWeapon_ShotGun::FirePellets()
{
	if (!EquippedPlayer && !WeaponData) return false;

	bool bAnyHit = false;

	FVector Forward = EquippedPlayer->GetActorForwardVector();
	//디버그 용
	TArray<AActor*>ActorsToIgnore;
	ActorsToIgnore.Add(this);
	ActorsToIgnore.Add(EquippedPlayer);

	//Hit한 Actor들 중 Pawn과 Object만 획득
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel4));

	for (int32 i = 0; i < PelletCount; i++) {
		float SpreadAngle = FMath::RandRange(-WeaponData->Stats.AttackDegree * 0.5f, WeaponData->Stats.AttackDegree * 0.5f);
		FVector PelletDir = Forward.RotateAngleAxis(SpreadAngle, FVector::UpVector).GetSafeNormal();
		FVector PelletEnd = EquippedPlayer->GetActorLocation() + PelletDir * (WeaponData->Stats.AttackRange * EquippedPlayer->NowMap->BlockSize);

		TArray<FHitResult> PelletHits;
		TSet<AActor*> PelletHitActors;

		UKismetSystemLibrary::SphereTraceMultiForObjects(
			EquippedPlayer,
			EquippedPlayer->GetActorLocation(),
			PelletEnd,
			WeaponData->Stats.AttackRadius,
			ObjectTypes,
			false,
			ActorsToIgnore,
			EDrawDebugTrace::ForDuration,
			PelletHits,
			true
		);

		// 히트 결과를 순회하며 데미지 적용
		for (const FHitResult& Hit : PelletHits)
		{
			AActor* HitActor = Hit.GetActor();
			// 유효하지 않거나 이미 이 탄알에 맞은 액터는 스킵
			if (!HitActor || PelletHitActors.Contains(HitActor)) continue;
			// 중복 데미지 방지를 위해 등록
			PelletHitActors.Add(HitActor);

			if (APlayer_Character* HittedPlayer = Cast<APlayer_Character>(HitActor))
			{
				// 자기 자신이거나 이미 탈락한 플레이어는 스킵
				if (HittedPlayer == EquippedPlayer || HittedPlayer->IsOut()) continue;
				// 플레이어에게 탄알 데미지 적용
				UGameplayStatics::ApplyDamage(HittedPlayer, FMath::CeilToFloat(WeaponData->Stats.Attack / PelletCount), EquippedPlayer->GetController(), EquippedPlayer, nullptr);
				ApplyHitEffect(HittedPlayer);
				bAnyHit = true;
			}
			else if (AObjects* HittedObj = Cast<AObjects>(HitActor))
			{
				// 본인이 들고 있는 오브젝트는 스킵
				if (HittedObj == EquippedPlayer->NowObjects) continue;
				// HP가 없는 오브젝트는 스킵
				if (!HittedObj->ObjectsData || !HittedObj->ObjectsData->bUseHP) continue;
				// 오브젝트에 탄알 데미지 적용 (전체 데미지/펠릿 수 (소숫점 올림으로 적용))
				UGameplayStatics::ApplyDamage(HittedObj, FMath::CeilToFloat(WeaponData->Stats.Attack / PelletCount), EquippedPlayer->GetController(), EquippedPlayer, nullptr);
				ApplyHitEffect(HittedObj);
				bAnyHit = true;
			}
		}
	}
	return bAnyHit;
}

