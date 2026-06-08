// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_Magnetic.h"
#include "Player_Character.h"
#include "Coin.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/PrimitiveComponent.h"

void UPlayerConditionEffect_Magnetic::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer) {
	MagneticSweep(Player);
}

void UPlayerConditionEffect_Magnetic::PersistEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, float DeltaTime)
{
	if (!Player || !Player->HasAuthority() || Player->IsOut()) {
		EndEffect(Player, ConditionComp, ConditionData);
		ConditionData.Duration = -1.f;
		return;
	}

	ScanTimer += DeltaTime;
	if (ScanTimer >= 2.f) {
		ScanTimer = 0.f;
		MagneticSweep(Player);
	}
	float CurrentTime = Player->GetWorld()->GetTimeSeconds();
	FVector SweeperLocation = Player->GetActorLocation() + FVector(0.f, 0.f, 50.f);

	for (auto IT = CapturedCoins.CreateIterator(); IT; ++IT) {
		ACoin* Coin = IT->Get();
		if (!IsValid(Coin) || Coin->IsActorBeingDestroyed()) {
			CoinCaptureTimes.Remove(Coin);
			IT.RemoveCurrent();
			continue;
		}

		UPrimitiveComponent* CoinPhysics = Coin->GetObjectPhysicsCollider();
		if (!CoinPhysics) continue;

		CoinPhysics->WakeAllRigidBodies();

		//공중 대기 시간
		float* CapturedAt = CoinCaptureTimes.Find(Coin);
		if (CapturedAt && (CurrentTime - *CapturedAt) < 0.35f) continue;

		//실제 흡수 물리 계산
		FVector CoinLocation = Coin->GetActorLocation();
		FVector Direction = (SweeperLocation - CoinLocation);
		float Distance = Direction.Size();

		//거리에 따른 가변 속도 : 가까워질수록 가속
		float Alpha = FMath::Clamp(1.f - (Distance / MagneticRadius), 0.f, 1.f);
		float Speed = FMath::Lerp(400.f, 3000.f, FMath::Square(Alpha));

		FVector Velocity = Direction.GetSafeNormal() * Speed;

		//포물선 보정
		if (Distance > 200.f) {
			Velocity += FVector(0.f, 0.f, 300.f * (Distance / MagneticRadius));
		}
		CoinPhysics->SetPhysicsLinearVelocity(Velocity);
	}
}

void UPlayerConditionEffect_Magnetic::EndEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData)
{
}

void UPlayerConditionEffect_Magnetic::EndFunction(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, bool bUseEndEffect)
{
	for (auto& CoinPtr : CapturedCoins) {
		if (ACoin* Coin = CoinPtr.Get()) {
			UPrimitiveComponent* PhysicsComp = CoinPtr->GetObjectPhysicsCollider();
			if (PhysicsComp) {
				PhysicsComp->SetCollisionResponseToAllChannels(ECR_Block);
				PhysicsComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
				PhysicsComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
				PhysicsComp->SetEnableGravity(true);
				PhysicsComp->WakeAllRigidBodies();
				Coin->bIsFlying = true;
			}
		}
	}

	CapturedCoins.Empty();
	CoinCaptureTimes.Empty();

	Super::EndFunction(Player, ConditionComp, ConditionData, bUseEndEffect);
}

void UPlayerConditionEffect_Magnetic::MagneticSweep(APlayer_Character* Player) {
	if (!Player) return;

	//원형 범위(MagneticRadius)내의 Overlap된 모든 코인(ECC_GameTraceChannel3)을 받아옴
	TArray<AActor*> OverlapActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3));

	UKismetSystemLibrary::SphereOverlapActors(Player->GetWorld(), Player->GetActorLocation(), MagneticRadius, ObjectTypes, ACoin::StaticClass(), { Player }, OverlapActors);

	float CurrentTime = Player->GetWorld()->GetTimeSeconds();
	for (AActor* Actor : OverlapActors) {
		ACoin* Coin = Cast<ACoin>(Actor);
		if (Coin && !CapturedCoins.Contains(Coin) && !Coin->bIsTaken && !Coin->bIsCaptured /*최초 Capture한 대상에게만 적용*/) {
			Coin->bIsFlying = false;
			Coin->bFirstBounce = false;
			CapturedCoins.Add(Coin);
			CoinCaptureTimes.Add(Coin, CurrentTime);

			UPrimitiveComponent* PhysicsComp = Coin->GetObjectPhysicsCollider();
			if (PhysicsComp) {
				//코인의 움직임/회전 저항값
				PhysicsComp->SetLinearDamping(Coin->LandingLinearDamping);
				PhysicsComp->SetAngularDamping(Coin->LandingAngularDamping);
				//코인 물리 설정
				PhysicsComp->WakeAllRigidBodies();
				PhysicsComp->SetCollisionResponseToAllChannels(ECR_Ignore);
				PhysicsComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
				
				PhysicsComp->AddImpulse(FVector(0.f, 0.f, 600.f), NAME_None, true);
			}
		}
	}
}

