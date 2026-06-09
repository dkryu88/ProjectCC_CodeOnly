// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_Turret.h"
#include "Object/Objects_Turret.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Match_PlayerController.h"
#include "DrawDebugHelpers.h"

bool AItem_Turret::UseEffect_Implementation(APlayer_Character* Player) {
	// 유효성 검사 및 서버 권한 확인
	if (!Player || !TurretClass || !Player->NowMap) return false;
	if (!Player->HasAuthority()) return false;

	AMapConstructor* Map = Player->NowMap;
	FVector PlayerLocation = Player->GetActorLocation();
	FVector ForwardDir = Player->GetActorForwardVector().GetSafeNormal2D();

	if (ForwardDir.IsNearlyZero()) return false;

	float CapsuleHalfHeight = 0.f;
	if (Player->GetCapsuleComponent()) CapsuleHalfHeight = Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector PlayerBottomLocation = Player->GetActorLocation();
	PlayerBottomLocation.Z -= CapsuleHalfHeight;

	float BoxHalfSize = Map->BlockSize * 0.35f;
	FVector FinalTargetLocation = FVector::ZeroVector;
	bool bFoundInstallLocation = false;

	// 플레이어의 1 ~ 2칸 앞(BlockSize 만큼 이동한) 타겟 위치 계산 (이동 중 설치 시도 실패 우려)
	for (int32 i = 1; i <= 2; i++) {
		FVector TargetBlock = PlayerBottomLocation + ForwardDir * Map->BlockSize * i;
		FVector TargetLocation = Map->GetTopBlockLocationFromWorld(TargetBlock);

		// 유효하지 않은 맵 위치면 설치 실패
		if (TargetLocation == FVector(-1.f, -1.f, -1.f)) continue;

		// 플레이어와 설치 위치의 Z축(높이) 차이 검사
		
		if (FMath::Abs(TargetLocation.Z - PlayerBottomLocation.Z) > BoxHalfSize) continue;

		// 생성하려는 위치에 무엇인가 있는지 확인
		FVector CheckCenter = TargetLocation + FVector(0.f, 0.f, BoxHalfSize);
		FCollisionShape CheckBox = FCollisionShape::MakeBox(FVector(BoxHalfSize, BoxHalfSize, BoxHalfSize));
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Player);
		if (Player->NowSupport) {
			Params.AddIgnoredActor(Player->NowSupport);
		}

		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectParams.AddObjectTypesToQuery(ECC_Visibility);

		DrawDebugBox(GetWorld(), CheckCenter, CheckBox.GetExtent(), FColor::Green, false, 1.0f, 0, 2.0f);
		bool bBlocked = GetWorld()->OverlapAnyTestByObjectType(CheckCenter, FQuat::Identity, ObjectParams, CheckBox, Params);
		if (bBlocked) continue;

		FinalTargetLocation = TargetLocation;
		bFoundInstallLocation = true;
		break;
	}
	
	if (!bFoundInstallLocation) return false;

	//장비나 오브젝트를 살짝 밀어냄
	TArray<FOverlapResult> NearObjects;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel5);	//Equipment (Item, Weapon)
	ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);	//Onject

	FVector PushCenter = FinalTargetLocation + FVector(0.f, 0.f, -20.f);
	FCollisionShape PushSphere = FCollisionShape::MakeSphere(48.f);
	FCollisionQueryParams PushParams;
	PushParams.AddIgnoredActor(Player);

	if (GetWorld()->OverlapMultiByObjectType(NearObjects, PushCenter, FQuat::Identity, ObjectQueryParams, PushSphere, PushParams)) {
		for (auto& Res : NearObjects) {
			AActor* Other = Res.GetActor();
			if (Other && Other != Player) {
				UPrimitiveComponent* RootComp = Cast < UPrimitiveComponent>(Other->GetRootComponent());
				if (RootComp && RootComp->IsSimulatingPhysics()) {
					FVector PushDir = Other->GetActorLocation() - FinalTargetLocation;
					PushDir.Normalize();
					PushDir.Z = 0.5f;

					FVector NewLocation = FinalTargetLocation + (PushDir * 60.f) + FVector(0.f, 0.f, 10.f);
					Other->SetActorLocation(NewLocation);

					RootComp->AddImpulse(PushDir * 300.f, NAME_None, true);
				}
			}
		}
	}

	// 스폰할 위치와 회전값을 담는 Transform 구조체를 만듬
	FTransform SpawnTransform(FQuat::Identity, FinalTargetLocation);

	AObjects_Turret* NewTurret = GetWorld()->SpawnActorDeferred<AObjects_Turret>(
		TurretClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (NewTurret) {
		// 설치한 플레이어를 소유자로 지정하여 아군 판정이나 점수 계산에 활용
		NewTurret->OwnPlayer = Player;
		NewTurret->OwnPlayerController = Cast<AMatch_PlayerController>(Player->GetController());
		UGameplayStatics::FinishSpawningActor(NewTurret, SpawnTransform);

		return true;
	}

	return false;

}
