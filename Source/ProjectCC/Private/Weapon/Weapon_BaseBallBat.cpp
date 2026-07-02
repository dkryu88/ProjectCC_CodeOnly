// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_BaseBallBat.h"
#include "Player_Character.h"
#include "Objects.h"
#include "ObjectsDataAsset.h"
#include "MapConstructor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

bool AWeapon_BaseBallBat::UseEffect_Implementation(APlayer_Character* Player)
{
	if (!Player) return false;
	if (!Player->NowMap) return false;
	if (!WeaponData) return false;

	TArray<AActor*> OverlapActors;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel4));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel5));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel6));

	FVector Forward = Player->GetActorForwardVector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();

	FVector StartLocation = Player->GetActorLocation() + (Forward * (WeaponData->Stats.AttackRange * 0.5f * Player->NowMap->BlockSize));
	float AdjustRadius = (WeaponData->Stats.AttackRange - WeaponData->Stats.AttackRange * 0.4f) * Player->NowMap->BlockSize;

	UKismetSystemLibrary::SphereOverlapActors(this, StartLocation, AdjustRadius, ObjectTypes, nullptr, { Player }, OverlapActors);
	bool bHitAnything = false;

	for (AActor* Actor : OverlapActors) {
		if (!Actor) continue;
		if (Actor->ActorHasTag(TEXT("NonDestroy"))) continue;

		FVector Dir = Actor->GetActorLocation() - Player->GetActorLocation();
		Dir.Z = 0.f;
		Dir.Normalize();
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Actor->GetRootComponent())) {
			ECollisionEnabled::Type ColType = Prim->GetCollisionEnabled();
			if (ColType == ECollisionEnabled::QueryOnly || ColType == ECollisionEnabled::NoCollision) continue;

			EObjectTypeQuery ObjType = UEngineTypes::ConvertToObjectType(Prim->GetCollisionObjectType());
			float Force = KnockBackForce_OtherNotPlayer;
			if (ObjType == UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3)) {
				Force = KnockBackForce_Coin;
			}

			//플레이어는 ApplyDamageInternal 내부에 넉백을 이용
			if (APlayer_Character* HitPlayer = Cast<APlayer_Character>(Actor)) {
				continue;
			}

			//무기와 물체는 무게가 있는 경우 밀리는 힘 감소
			if (AWeapon* HitWeapon = Cast<AWeapon>(Actor)) {
				Force -= HitWeapon->GetWeaponWeight() * 100.f;
			}
			else if (AObjects* HitObjects = Cast <AObjects>(Actor)) {
				if (!HitObjects->ObjectsData->bCanMove) {
					continue;
				}
				Force -= HitObjects->ObjectsData->Weight * 100.f;
			}

			FVector Impulse = Dir * Force + FVector(0, 0, Force * 0.5f);
			Prim->SetSimulatePhysics(true);
			Prim->AddImpulse(Impulse, NAME_None, true);
			bHitAnything = true;
		}
	}

	//플레이어/물체 뿐만 아니라 다른 대상이 하나라도 맞으면 히트 사운드 재생
	if (bHitAnything && WeaponData && WeaponData->HitEffect.Sound) {
		if (Player->EffectManagerComp){
			FGameEffectData SoundOnlyEffect = WeaponData->HitEffect;

			// Niagara는 재생하지 않도록 제거
			SoundOnlyEffect.NiagaraEffect = nullptr;

			FGameEffectContext Context;
			Context.SourceActor = Player;
			Context.SourceComponent = Player->GetMesh();
			Context.WorldLocation = StartLocation;
			Context.WorldRotation = Player->GetActorRotation();

			Player->EffectManagerComp->PlayGameEffect_Multicast(SoundOnlyEffect, Context);
		}
	}

	return bHitAnything;
}

void AWeapon_BaseBallBat::HitEffect_Implementation(APlayer_Character* Player, AActor* Target)
{
	if (!Player) return;
	if (!Player->NowMap) return;
	if (!WeaponData) return;

	TArray<AActor*> OverlapActors;
	
	FVector Forward = Player->GetActorForwardVector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();

	FVector StartLocation = Player->GetActorLocation() + (Forward * (WeaponData->Stats.AttackRange * 0.5f * Player->NowMap->BlockSize));
	float AdjustRadius = (WeaponData->Stats.AttackRange - WeaponData->Stats.AttackRange * 0.4f) * Player->NowMap->BlockSize;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel4));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel5));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel6));

	UKismetSystemLibrary::SphereOverlapActors(this, StartLocation, AdjustRadius, ObjectTypes, nullptr, { Player }, OverlapActors);

	for (AActor* Actor : OverlapActors) {
		if (!Actor) continue;
		if (Actor->ActorHasTag(TEXT("NonDestroy"))) continue;

		FVector Dir = Actor->GetActorLocation() - Player->GetActorLocation();
		Dir.Z = 0.f;
		Dir.Normalize();
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Actor->GetRootComponent())) {
			ECollisionEnabled::Type ColType = Prim->GetCollisionEnabled();
			if (ColType == ECollisionEnabled::QueryOnly || ColType == ECollisionEnabled::NoCollision) continue;

			EObjectTypeQuery ObjType = UEngineTypes::ConvertToObjectType(Prim->GetCollisionObjectType());
			float Force = KnockBackForce_OtherNotPlayer;
			if (ObjType == UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3)) {
				Force = KnockBackForce_Coin;
			}

			//플레이어는 ApplyDamageInternal 내부에 넉백을 이용
			if (APlayer_Character* HitPlayer = Cast<APlayer_Character>(Actor)) {
				continue;
			}

			//무기와 물체는 무게가 있는 경우 밀리는 힘 감소
			if (AWeapon* HitWeapon = Cast<AWeapon>(Actor)) {
				Force -= HitWeapon->GetWeaponWeight() * 100.f;
			}
			else if (AObjects* HitObjects = Cast <AObjects>(Actor)) {
				if (!HitObjects->ObjectsData->bCanMove) {
					continue;
				}
				Force -= HitObjects->ObjectsData->Weight * 100.f;
			}

			FVector Impulse = Dir * Force + FVector(0, 0, Force * 0.5f);
			Prim->SetSimulatePhysics(true);
			Prim->AddImpulse(Impulse, NAME_None, true);
		}
	}
}
