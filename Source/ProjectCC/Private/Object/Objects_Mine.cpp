// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_Mine.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "MapConstructor.h"
#include "Match_PlayerController.h"
#include "Player_Character.h"
#include "Engine/OverlapResult.h"

AObjects_Mine::AObjects_Mine(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UBoxComponent>(TEXT("PhysicsCollider"))){
	SizeMagnification = SizeMagnification * 1.25;
}

void AObjects_Mine::ApplyAdditionalSetting()
{
	if (PhysicsCollider) {
		PhysicsCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel4);

		PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
		PhysicsCollider->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		PhysicsCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		PhysicsCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap);
		PhysicsCollider->OnComponentBeginOverlap.AddDynamic(this, &AObjects_Mine::OnOverlapBegin);
		PhysicsCollider->SetGenerateOverlapEvents(true);
	}

	if (InterActionCollider) {
		InterActionCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InterActionCollider->SetGenerateOverlapEvents(false);
	}
}

void AObjects_Mine::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool vFromSweep, const FHitResult& SweepResult) {
	if (!HasAuthority() || bIsExploded) return;

	APlayer_Character* HittedPlayer = Cast<APlayer_Character>(OtherActor);
	AMatch_PlayerController* HittedPlayerController;
	if (HittedPlayer) {
		HittedPlayerController = Cast<AMatch_PlayerController>(HittedPlayer->GetController());
	}

	if (HittedPlayer && HittedPlayer != OwnPlayer && HittedPlayerController != OwnPlayerController) {
		bIsExploded = true;

		FVector ExplosionOrigin = GetActorLocation();
		float HalfSize = NowMap->BlockSize * 0.5f;

		if (NowMap) {
			int32 X = 0;
			int32 Y = 0;
			int32 Z = 0;
			NowMap->WorldToGridTopBlock(ExplosionOrigin, X, Y, Z);
			ExplosionOrigin += FVector(0.f, 0.f, HalfSize);
		}
		float HalfRange = NowMap->BlockSize * 1.5f;
		FVector BoxExtent = FVector(HalfRange, HalfRange, HalfSize - 5.f);

		TArray<FOverlapResult> OverlapResults;
		FCollisionShape BoxShape = FCollisionShape::MakeBox(BoxExtent);
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		bool bHasOverlap = GetWorld()->OverlapMultiByChannel(OverlapResults, ExplosionOrigin, FQuat::Identity, ECC_Pawn, BoxShape, Params);

		// 디버그 드로우도 블록 중앙에 맞춰서 그려줍니다.
		DrawDebugBox(GetWorld(), ExplosionOrigin, BoxExtent, FQuat::Identity, FColor::Red, false, 3.f, 0, 3.f);
		DrawDebugSphere(GetWorld(), ExplosionOrigin, 20.f, 12, FColor::Green, false, 3.0f);

		if (bHasOverlap) {
			for (auto& Result : OverlapResults) {
				APlayer_Character* Victim = Cast<APlayer_Character>(Result.GetActor());
				if (Victim) {
					APlayer_Character* Attacker = IsValid(OwnPlayer) ? OwnPlayer : nullptr;
					Victim->ApplyDamageInternal(DamageAmount, Attacker, this, true, true, false);
				}
			}
		}

		Destroy();
	}
}