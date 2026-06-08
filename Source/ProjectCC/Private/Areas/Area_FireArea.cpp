// Fill out your copyright notice in the Description page of Project Settings.


#include "Areas/Area_FireArea.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "PlayerConditionDataAsset.h"

AArea_FireArea::AArea_FireArea()
{
	//최초 Area 정보 세팅
	SetAreaData(3.1f, 1.f);
}

void AArea_FireArea::ApplyInAreaEffect_Implementation(AActor* OtherActor) {
	ApplyBurnToPlayer(OtherActor);
}

void AArea_FireArea::ApplyStayAreaEffect_Implementation(AActor* OtherActor) {
	ApplyBurnToPlayer(OtherActor);
}

void AArea_FireArea::ApplyBurnToPlayer(AActor* OtherActor)
{
	if (!HasAuthority()) return;
	if (!OtherActor) return;
	if (!BurnConditionData) return;

	APlayer_Character* Player = Cast<APlayer_Character>(OtherActor);
	if (!Player) return;

	UPlayerConditionComponent* ConditionComp = Player->FindComponentByClass<UPlayerConditionComponent>();
	if (!ConditionComp) return;

	ConditionComp->ApplyCondition(BurnConditionData, OwnPlayer, 0.f);
	
	UE_LOG(LogTemp, Warning, TEXT("Apply Burn"));
}

