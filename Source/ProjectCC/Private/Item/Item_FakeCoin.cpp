// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_FakeCoin.h"
#include "Object/Objects_FakeCoin.h"
#include "Coin.h"
#include "Player_Character.h"
#include "Match_PlayerController.h"
#include "MapConstructor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"

bool AItem_FakeCoin::UseEffect_Implementation(APlayer_Character* Player) {
	if (!Player || !FakeCoin || !Player->NowMap) return false;
	if (!HasAuthority()) return false;

	AMapConstructor* Map = Player->NowMap;
	FVector PlayerLocation = Player->GetActorLocation();

	FVector TopBlock = Map->GetTopBlockLocationFromWorld(PlayerLocation);
	if (TopBlock == FVector(-1.f, -1.f, -1.f))return false;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Player);

	//블록에 코인 존재 유무 확인
	FCollisionShape CheckShape = FCollisionShape::MakeSphere(20.f);
	TArray<FOverlapResult> CoinOverlap;
	FCollisionObjectQueryParams CoinParams;
	CoinParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);	//Coin
	CoinParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);	//Object
	bool bIsCoinExist = false;
	if (GetWorld()->OverlapMultiByObjectType(CoinOverlap, TopBlock, FQuat::Identity, CoinParams, CheckShape, Params)) {
		for (auto& Res : CoinOverlap) {
			if (Res.GetActor() && (Res.GetActor()->IsA(AObjects_FakeCoin::StaticClass()) || Res.GetActor()->IsA(ACoin::StaticClass()))) {
				bIsCoinExist = true;
				break;
			}
		}
	}
	if (!bIsCoinExist) {	//코인이 존재하지 않는다면
		TArray<FOverlapResult> NearObjects;
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel5);	//Equipment (Item, Weapon)
		ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);	//Onject
		//장비나 오브젝트를 살짝 밀어냄
		FCollisionShape PushSphere = FCollisionShape::MakeSphere(40.f);
		if (GetWorld()->OverlapMultiByObjectType(NearObjects, TopBlock, FQuat::Identity, ObjectQueryParams, PushSphere, Params)) {
			for (auto& Res : NearObjects) {
				AActor* Other = Res.GetActor();
				if (Other && !Other->IsA(AObjects_FakeCoin::StaticClass())) {
					UPrimitiveComponent* RootComp = Cast < UPrimitiveComponent>(Other->GetRootComponent());
					if (RootComp && RootComp->IsSimulatingPhysics()) {
						FVector PushDir = Other->GetActorLocation() - TopBlock;
						PushDir.Normalize();
						PushDir.Z = 0.5f;
						RootComp->AddImpulse(PushDir * ImpulsePower, NAME_None, true);
					}
				}
			}
		}
		//실제 생성
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FVector SpawnLocation = TopBlock + FVector(0.f, 0.f, SpawnZOffset);	//소환 전 위치값 계산
		AObjects_FakeCoin* NewTrap = GetWorld()->SpawnActor<AObjects_FakeCoin>(FakeCoin, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
		if (NewTrap) {
			NewTrap->SetActorLocation(SpawnLocation);	//위치 보정
			NewTrap->OwnPlayer = Player;
			return true;
		}
	}

	return false;
}
