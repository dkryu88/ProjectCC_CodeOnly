// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_Mine.h"
#include "Object/Objects_Mine.h"
#include "Player_Character.h"
#include "Match_PlayerController.h"
#include "MapConstructor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"

bool AItem_Mine::UseEffect_Implementation(APlayer_Character* Player)
{
	if (!Player || !Mine || !Player->NowMap) return false;
	if (!HasAuthority()) return false;

	AMapConstructor* Map = Player->NowMap;
	FVector PlayerLocation = Player->GetActorLocation();

	FVector TopBlock = Map->GetTopBlockLocationFromWorld(PlayerLocation);
	if (TopBlock == FVector(-1.f, -1.f, -1.f)) return false;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Player);
	
	//대상 블록에 물체(설치)가 있는지 확인
	bool bIsOtherExist = false;
	FCollisionShape CheckShape = FCollisionShape::MakeSphere(50.f);
	TArray<FOverlapResult>MineOverlap;
	FCollisionObjectQueryParams MineParams;

	MineParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);
	if (GetWorld()->OverlapMultiByObjectType(MineOverlap, TopBlock, FQuat::Identity, MineParams, CheckShape, Params)) {
		for (auto& Res : MineOverlap) {
			if (AObjects* OverlapObjects = Cast<AObjects>(Res.GetActor())) {
				if (OverlapObjects->Type == EObjectsType::Install) {
					bIsOtherExist = true;
					break;
				}
			}
		}
	}

	if (!bIsOtherExist) {
		TArray<FOverlapResult> NearObjects;
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel5);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);

		FCollisionShape PushSphere = FCollisionShape::MakeSphere(40.f);
		if (GetWorld()->OverlapMultiByObjectType(NearObjects, TopBlock, FQuat::Identity, ObjectQueryParams, PushSphere, Params)) {
			for (auto& Res : NearObjects) {
				if (Res.GetActor()) {
					UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(Res.GetActor()->GetRootComponent());
					if (RootComp && RootComp->IsSimulatingPhysics()) {
						FVector PushDir = Res.GetActor()->GetActorLocation() - TopBlock;
						PushDir = PushDir.GetSafeNormal();
						PushDir.Z = 0.5f;
						RootComp->AddImpulse(PushDir * ImpulsePower, NAME_None, true);
					}
				}
				
			}
		}
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AObjects_Mine* NewTrap = GetWorld()->SpawnActor<AObjects_Mine>(Mine, TopBlock, FRotator::ZeroRotator, SpawnParams);
		if (NewTrap) {
			NewTrap->SetActorLocation(TopBlock);
			NewTrap->OwnPlayer = Player;
			NewTrap->OwnPlayerController = Cast<AMatch_PlayerController>(Player->GetController());
			return true;
		}
	}

	return false;
}
