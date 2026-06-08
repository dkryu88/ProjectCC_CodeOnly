// Fill out your copyright notice in the Description page of Project Settings.


#include "Areas/Area_RottenCheeseSlowArea.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"
#include "PlayerConditionDataAsset.h"

AArea_RottenCheeseSlowArea::AArea_RottenCheeseSlowArea()
{
	//최초 Area 정보 세팅
	SetAreaData(2.1f, 0.1f);
}

void AArea_RottenCheeseSlowArea::ApplyInAreaEffect_Implementation(AActor* OtherActor)
{
	ApplySlowToPlayer(OtherActor);
}

void AArea_RottenCheeseSlowArea::ApplyStayAreaEffect_Implementation(AActor* OtherActor)
{
	ApplySlowToPlayer(OtherActor);
}

void AArea_RottenCheeseSlowArea::ApplySlowToPlayer(AActor* OtherActor)
{
	if (!HasAuthority()) return;
	if (!OtherActor) return;
	if (!SlowConditionData) return;

	APlayer_Character* Player = Cast<APlayer_Character>(OtherActor);
	if (!Player) return;

	UPlayerConditionComponent* ConditionComp = Player->FindComponentByClass<UPlayerConditionComponent>();
	if (!ConditionComp) return;

	ConditionComp->ApplyCondition(SlowConditionData, OwnPlayer, 0.f);

	UE_LOG(LogTemp, Warning, TEXT("Apply Slow"));
}
