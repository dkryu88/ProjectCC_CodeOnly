// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_RottenCheese.h"
#include "MapConstructor.h"
#include "PlayMode_Match.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "Areas/Area_RottenCheeseSlowArea.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"

AObjects_RottenCheese::AObjects_RottenCheese(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

void AObjects_RottenCheese::Func_Destroy_Implementation()
{
	Super::Func_Destroy_Implementation();

	if (!HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World || !RottenCheeseSlowArea) return;

	SpawnArea(RottenCheeseSlowArea, GetAreaCenterLocation(), this, OwnPlayer, nullptr, 0, 0);
}

void AObjects_RottenCheese::Func_HitPlayer_Implementation(APlayer_Character* Player)
{
	Super::Func_HitPlayer_Implementation(Player);

	if (!HasAuthority()) return;
	if (!Player) return;
	if (!SlowConditionData) return;

	UPlayerConditionComponent* ConditionComp = Player->FindComponentByClass<UPlayerConditionComponent>();
	if (!ConditionComp) return;

	ConditionComp->ApplyCondition(SlowConditionData, OwnPlayer, 0.f);

	UE_LOG(LogTemp, Warning, TEXT("Apply Slow"));
}


