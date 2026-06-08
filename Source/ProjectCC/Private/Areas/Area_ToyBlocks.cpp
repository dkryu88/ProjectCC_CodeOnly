// Fill out your copyright notice in the Description page of Project Settings.


#include "Areas/Area_ToyBlocks.h"
#include "Player_Character.h"

AArea_ToyBlocks::AArea_ToyBlocks() {
	AreaDuration = 10.f;
	AreaEffectInterval = 1.f;
}

void AArea_ToyBlocks::ApplyInAreaEffect_Implementation(AActor* OtherActor) {
	if (!HasAuthority() || !OtherActor) return;
	if (APlayer_Character* Victim = Cast<APlayer_Character>(OtherActor)) {
		APlayer_Character* Attacker = OwnPlayer.Get();
		Victim->ApplyDamageInternal(DamageAmount, Attacker, this, false, false, false);
	}
}

void AArea_ToyBlocks::ApplyStayAreaEffect_Implementation(AActor* OtherActor) {
	if (!HasAuthority() || !OtherActor) return;
	if (APlayer_Character* Victim = Cast<APlayer_Character>(OtherActor)) {
		APlayer_Character* Attacker = OwnPlayer.Get();
		Victim->ApplyDamageInternal(DamageAmount, Attacker, this, false, false);
	}
}