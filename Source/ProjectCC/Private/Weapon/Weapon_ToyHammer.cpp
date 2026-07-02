// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_ToyHammer.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"

AWeapon_ToyHammer::AWeapon_ToyHammer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UStaticMeshComponent>((TEXT("WeaponPhysicsCollider"))))
{
}

void AWeapon_ToyHammer::HitEffect_Implementation(APlayer_Character* Player, AActor* Target)
{
	if (!HasAuthority() || !Player || !Target) return;
	if (!GetWorld()) return;

	APlayer_Character* CurrentTarget = Cast<APlayer_Character>(Target);
	if (!CurrentTarget) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastHitTime > ComboResetTime) {
		ComboStack = 0;
	}
	LastHitTime = CurrentTime;

	if (LastHitPlayer == CurrentTarget) {
		ComboStack++;

		if (ComboStack >= 3) {
			ComboStack = 0;

			if (Player && StunData && CurrentTarget && !CurrentTarget->IsOut() && CurrentTarget->ConditionComp) {
				CurrentTarget->ConditionComp->ApplyCondition(StunData, Player, StunDuration);
			}
		}
	}
	else {
		ComboStack = 1;
		LastHitPlayer = CurrentTarget;
	}
}
