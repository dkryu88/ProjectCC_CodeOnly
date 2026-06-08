// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_HealingTotem.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "PlayMode_Match.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "ObjectsDataAsset.h"
#include "Objects_HPWidget.h"
// 디버그 드로우
#include "DrawDebugHelpers.h"

AObjects_HealingTotem::AObjects_HealingTotem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	HealRangeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HealRangeBox"));
	HealRangeBox->SetupAttachment(RootComponent);
	HealRangeBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	HealRangeBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HealRangeBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HealRangeBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 넘어짐 방지
	PhysicsCollider->BodyInstance.bLockXRotation = true;
	PhysicsCollider->BodyInstance.bLockYRotation = true;
}

void AObjects_HealingTotem::Func_Persist_Implementation(float DeltaTime) {
	if (!HasAuthority() || HP <= 0) return;

	//NowMap이 없으면 게임모드에서 직접 가져옴
	if (!NowMap) {
		UWorld* World = GetWorld();
		if (!World) return;
		APlayMode_Match* GM = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
		if (!GM) return;
		NowMap = GM->GetCurrentMap();
		if (!NowMap) return;
	}

	float HalfSize = (NowMap->BlockSize * HealRange) * 0.5f;
	FVector BoxExtent = FVector(HalfSize, HalfSize, HalfSize * 3);
	FVector Center = GetActorLocation();

	// 디버그드로우
	DrawDebugBox(GetWorld(), Center, BoxExtent, FColor::Cyan, false, 0.5f, 0, 2.0f);

	TArray<AActor*> OverrlapActors;
	HealRangeBox->SetBoxExtent(BoxExtent);
	HealRangeBox->GetOverlappingActors(OverrlapActors, APlayer_Character::StaticClass());

	bool bDidHeal = false;
	for (AActor* Actor : OverrlapActors) {
		if (APlayer_Character* HealTarget = Cast<APlayer_Character>(Actor)) {
			if (HealTarget && !HealTarget->IsOut() && HealTarget->HP < 100.f) {
				HealTarget->HPChange(HealAmount);
				bDidHeal = true;

				// 디버그드로우
				DrawDebugBox(GetWorld(), HealTarget->GetActorLocation(), FVector(40.f, 40.f, 10.f), FColor::Green, false, 0.5f, 0, 5.0f);
			}
		}
	}

	if (bDidHeal) {
		ApplyDamageInternal(1.f, nullptr, this, false, true);

		RefreshHPWidget();

		// 디버그드로우
		DrawDebugBox(GetWorld(), Center, BoxExtent + FVector(2.f), FColor::Green, false, 0.2f, 0, 5.0f);

		Multicast_PlayHealEffect();
	}
}

void AObjects_HealingTotem::Func_Destroy_Implementation() {
	UnifiedDestructionPath();
}

void AObjects_HealingTotem::Func_ZeroLife_Implementation() {
	UnifiedDestructionPath();
}

void AObjects_HealingTotem::ApplyAdditionalSetting()
{
	if (!PhysicsCollider || !HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	PhysicsCollider->SetSimulatePhysics(true);
	PhysicsCollider->SetEnableGravity(true);

	//넘어지는건 허용, 쉽게 회전하지 않도록 Damping 증가
	PhysicsCollider->SetAngularDamping(20.f);
	//밀렸을 때 쉽게 밀리지 않도록 Damping 증가
	PhysicsCollider->SetLinearDamping(5.f);
	//질량 증가
	PhysicsCollider->SetMassOverrideInKg(NAME_None, 150.f, true);
	//무게 중심을 아래로 이동
	PhysicsCollider->SetCenterOfMass(FVector(0.f, 0.f, -25.f), NAME_None);
	//회전 관성 증가
	PhysicsCollider->BodyInstance.InertiaTensorScale = FVector(10.f, 10.f, 1.f);
	PhysicsCollider->BodyInstance.UpdateMassProperties();
}

void AObjects_HealingTotem::UnifiedDestructionPath() {
	if (!HasAuthority() || bIsDestroyed) return;

	/*이곳에 파괴 연출 추가*/
	
	// 파괴되는 경우가 hp 0, lifetime 0 으로 2가지가 있기 때문에 한 곳으로 묶음
	bIsDestroyed = true;
}

void AObjects_HealingTotem::Multicast_PlayHealEffect_Implementation() {
	RefreshHPWidget();
}