// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_SnowBomb.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "PlayerTransformationComponent.h"
#include "Engine/OverlapResult.h"


AObjects_SnowBomb::AObjects_SnowBomb(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AObjects_SnowBomb::BeginPlay()
{
	Super::BeginPlay();

	map = NowMap;

	if (!map && OwnPlayer) {
		map = OwnPlayer->NowMap;
	}
}

void AObjects_SnowBomb::Func_BecomeNormalType_Implementation(const FHitResult& Hit)
{
	if (!HasAuthority()) return;
	if (bLanded) return;
	bLanded = true;
	GetWorldTimerManager().SetTimer(ExplodeTimerHandle, this, &AObjects_SnowBomb::SnowExplode, 2.f, false);
}

void AObjects_SnowBomb::Func_Destroy_Implementation()
{
	if (bExploded) return;
	SnowExplode();
}

void AObjects_SnowBomb::SnowExplode()
{
	if (bExploded) return;
	if (!map) {
		map = NowMap;
		if (!map && OwnPlayer) {
			map = OwnPlayer->NowMap;
		}
	}
	if (!map) return;

	bExploded = true;

	FVector ExplosionOrigin = GetActorLocation();
	float HalfSize = map->BlockSize * 0.5f;

	//폭발 범위 생성
	TArray<FOverlapResult> OverlapResults;
	FVector BoxExtent = FVector(HalfSize * 3, HalfSize * 3, (HalfSize * 3) - 5.f);

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
			AObjects* TargetObjects = Cast<AObjects>(Result.GetActor());
			if (Victim) {
				Victim->ApplyDamageInternal(DamageAmount, OwnPlayer, this, true, true, false);
				UPlayerTransformationComponent* TransComp = Victim->FindComponentByClass<UPlayerTransformationComponent>();
				if (TransComp) {
					TransComp->StartTransformation(SnowManTransformationData, OwnPlayer, -1.f);
				}
			}
		}
	}

	Destroy();

}
