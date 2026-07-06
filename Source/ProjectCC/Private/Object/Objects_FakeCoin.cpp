// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_FakeCoin.h"
#include "Components/SphereComponent.h"
#include "Player_Character.h"
#include "Engine/OverlapResult.h"
#include "Components/BoxComponent.h"
#include "Match_PlayerController.h"
#include "Player_State.h"
#include "MapConstructor.h"

AObjects_FakeCoin::AObjects_FakeCoin(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<USphereComponent>(TEXT("PhysicsCollider")))
{
	PrimaryActorTick.bCanEverTick = true;

	USphereComponent* SphereCollider = Cast<USphereComponent>(PhysicsCollider);
	SizeMagnification = FVector(1.5f, 1.5f, 1.5f);

	if (SphereCollider) {
		SphereCollider->SetSphereRadius(ColliderRadius);
		SphereCollider->BodyInstance.bLockXRotation = true;
		SphereCollider->BodyInstance.bLockYRotation = true;
		SphereCollider->BodyInstance.bLockZRotation = true;
	}

	if (Mesh) {
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

}

void AObjects_FakeCoin::ApplyAdditionalSetting()
{
	if (USphereComponent* SphereCollider = Cast<USphereComponent>(PhysicsCollider))
	{
		SphereCollider->SetSimulatePhysics(false);
		SphereCollider->SetEnableGravity(false);

		SphereCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SphereCollider->SetCollisionObjectType(ECC_GameTraceChannel4);

		SphereCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
		SphereCollider->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		SphereCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		SphereCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap);

		SphereCollider->SetGenerateOverlapEvents(true);

		// 중복 바인딩 방지
		SphereCollider->OnComponentBeginOverlap.RemoveDynamic(this, &AObjects_FakeCoin::OnOverlapBegin);
		SphereCollider->OnComponentBeginOverlap.AddDynamic(this, &AObjects_FakeCoin::OnOverlapBegin);

		if (OwnPlayer){
			SphereCollider->IgnoreActorWhenMoving(OwnPlayer, true);
		}

		SetSizeofSphereColliderwithMesh(SphereCollider);
		SphereCollider->RecreatePhysicsState();
	}
	if (InterActionCollider) {
		InterActionCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InterActionCollider->SetGenerateOverlapEvents(false);
		InterActionCollider->SetHiddenInGame(true);
	}
}

void AObjects_FakeCoin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Mesh) {
		Mesh->AddLocalRotation(FRotator(0.f, SpinSpeed * DeltaTime, 0.f));
	}
}

void AObjects_FakeCoin::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool vFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bIsExploded) return;
	if (!NowMap) return;

	APlayer_Character* HittedPlayer = Cast<APlayer_Character>(OtherActor);
	if (!HittedPlayer) return;
	AMatch_PlayerController* HittedPlayerController = Cast<AMatch_PlayerController>(HittedPlayer->GetController());
	APlayer_State* HittedPlayerState = Cast<APlayer_State>(HittedPlayer->GetThePlayerState());

	if ((HittedPlayerState && HittedPlayerState->GetPortraitId() != OwnerPortraitId) && (HittedPlayerController && HittedPlayerController != OwnPlayerController)) {
		bIsExploded = true;

		FVector ExplosionOrigin = GetActorLocation();
		float HalfSize = NowMap->BlockSize * 0.5f;

		int32 X = 0;
		int32 Y = 0;
		int32 Z = 0;

		NowMap->WorldToGridTopBlock(ExplosionOrigin, X, Y, Z);
		ExplosionOrigin = GetActorLocation() + FVector(0.f, 0.f, 50.f);

		TArray<FOverlapResult> OverlapResults;
		FVector BoxExtent = FVector(HalfSize, HalfSize, HalfSize - 5.f);

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
					Victim->ApplyDamageInternal(DamageAmount, Attacker, this, true, true, true, false);
				}
			}
		}

		//이펙트 파라미터 설정 및 재생
		FGameEffectContext EffectContext;
		EffectContext.SourceActor = this;
		EffectContext.SourceComponent = Mesh;
		EffectContext.WorldLocation = ExplosionOrigin;
		EffectContext.WorldRotation = FRotator::ZeroRotator;
		EffectContext.HitPoint = ExplosionOrigin;
		EffectContext.HitNormal = FVector::UpVector;

		PlayObjectsEffect(EEffectType::Destroy, EffectContext);

		DelayForDestroyEffect();
	}
}
