// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_LocketLauncherBullet.h"
#include "Kismet/GameplayStatics.h"
#include "MapConstructor.h"
#include "Player_Character.h"
#include "Match_PlayerController.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"

AObjects_LocketLauncherBullet::AObjects_LocketLauncherBullet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (PhysicsCollider) {
		PhysicsCollider->BodyInstance.bUseCCD = true;
	}
}

void AObjects_LocketLauncherBullet::Func_Destroy_Implementation()
{
	Super::Func_Destroy_Implementation();

	if (!NowMap) return;

	FVector ExplosionCenter = GetActorLocation();

	float BoxHalfSize = (NowMap->BlockSize * 5.f) / 2.f;
	FVector BoxExtent = FVector(BoxHalfSize, BoxHalfSize, BoxHalfSize);

	if (HasAuthority()) {
		TArray<FOverlapResult> OverlapResults;
		FCollisionShape BoxShape = FCollisionShape::MakeBox(BoxExtent);

		DrawDebugBox(GetWorld(), ExplosionCenter, BoxExtent, FColor::Magenta, false, 2.f, 0, 3.f);

		FCollisionObjectQueryParams ObjParams;
		ObjParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);

		if (GetWorld()->OverlapMultiByObjectType(OverlapResults, ExplosionCenter, FQuat::Identity, ObjParams, BoxShape)) {
			TSet<AActor*> DamagedActors;

			for (auto& Result : OverlapResults) {
				APlayer_Character* Victim = Cast<APlayer_Character>(Result.GetActor());
				AObjects* TargetObjects = Cast<AObjects>(Result.GetActor());
				if (Victim && !DamagedActors.Contains(Victim)) {
					DamagedActors.Add(Victim);
					Victim->ApplyDamageInternal(DamageAmount, OwnPlayer, this, true, true, false);
				}
				if (TargetObjects && !DamagedActors.Contains(TargetObjects)) {
					DamagedActors.Add(Victim);
					TargetObjects->ApplyDamageInternal(DamageAmount, OwnPlayer, this, true, false);
				}
			}
		}
	}
}


//<아직 미적용>
void AObjects_LocketLauncherBullet::Multicast_PlayExplosionEffects_Implementation(FVector ExplosionCenter, float Radius)
{
	if (ExplosionCameraShake) {

		float InnerRadius = 500.f;
		float OuterRadius = 1200.f;

		UGameplayStatics::PlayWorldCameraShake(
			GetWorld(),
			ExplosionCameraShake,
			ExplosionCenter,
			InnerRadius,		// Inner (풀파워로 흔들리는 구간)
			OuterRadius,		// Outer (점점 약해지는 최대 구간)
			1.f,				// 흔들림 배율 (기본 1.0)
			false			    // bOrientShakeTowardsEpicenter (폭발 방향에 맞춰 흔들리게 할지 여부)
		);
	}
}




