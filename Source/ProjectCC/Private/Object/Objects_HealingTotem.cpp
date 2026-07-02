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
#include "NiagaraComponent.h"
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

void AObjects_HealingTotem::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority()) Multicast_SetLifeTimeEffectActive(!bHealPaused && !bIsDestroyed);
}

void AObjects_HealingTotem::Func_Persist_Implementation(float DeltaTime) {
	if (!HasAuthority() || HP <= 0) return;

	// 정지 중이면 힐 처리 중단
	if (bHealPaused) return;

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
	FVector BoxExtent = FVector(HalfSize, HalfSize, HalfSize);
	FVector Center = GetActorLocation();

	// 디버그드로우
	DrawDebugBox(GetWorld(), Center, BoxExtent, FColor::Cyan, false, 0.5f, 0, 2.0f);

	TArray<AActor*> OverrlapActors;
	HealRangeBox->SetBoxExtent(BoxExtent);
	HealRangeBox->GetOverlappingActors(OverrlapActors, APlayer_Character::StaticClass());

	bool bDidHeal = false;
	for (AActor* Actor : OverrlapActors) {
		APlayer_Character* HealTarget = Cast<APlayer_Character>(Actor);
		if (!HealTarget) continue;
		if (HealTarget->IsOut()) continue;

		float MaxHP = HealTarget->BaseStats.Max_HP;
		if (HealTarget->HP >= MaxHP) continue;

		float PrevHP = HealTarget->HP;

		HealTarget->HPChange(HealAmount);

		if (HealTarget->HP > PrevHP) {
			bDidHeal = true;
			PlayHealEffectOnPlayer(HealTarget);

			DrawDebugBox(GetWorld(), HealTarget->GetActorLocation(), FVector(40.f, 40.f, 10.f), FColor::Green, false, 0.5f, 0, 5.0f);
		}
	}

	if (bDidHeal) {
		ApplyDamageInternal(1.f, nullptr, this, false, true);
		RefreshHPWidget();

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

void AObjects_HealingTotem::ApplyInteractionState(APlayer_Character* InterActionPlayer)
{
	if (!HasAuthority()) return;
	if (!InterActionPlayer) return;
	if (bIsDestroyed) return;

	if (!bHealPaused) {
		bHealPaused = true;
		bResumeCoolDown = true;
		bNowActivated = true;

		Multicast_SetLifeTimeEffectActive(false);

		GetWorldTimerManager().ClearTimer(HealResumeCoolTimerHandle);
		GetWorldTimerManager().SetTimer(HealResumeCoolTimerHandle, this, &AObjects_HealingTotem::FinishHealResumeCoolDown, HealResumeCoolTime, false);
		return;
	}

	if (bResumeCoolDown) return;

	bHealPaused = false;
	bNowActivated = false;

	Multicast_SetLifeTimeEffectActive(true);
}

void AObjects_HealingTotem::UnifiedDestructionPath() {
	if (!HasAuthority() || bIsDestroyed) return;

	bIsDestroyed = true;
	bHealPaused = true;
	bResumeCoolDown = false;

	GetWorldTimerManager().ClearTimer(HealResumeCoolTimerHandle);

	Multicast_SetLifeTimeEffectActive(false);
}

void AObjects_HealingTotem::FinishHealResumeCoolDown()
{
	if (!HasAuthority()) return;
	if (bIsDestroyed) return;

	bResumeCoolDown = false;
}

void AObjects_HealingTotem::PlayHealEffectOnPlayer(APlayer_Character* HealTarget)
{
	if (!HasAuthority()) return;
	if (!HealTarget) return;
	if (!HealTarget->EffectManagerComp) return;
	if (!ObjectsData) return;

	FGameEffectData* EffectData = ObjectsData->CustomEffects.Find(HealEffectName);
	if (!EffectData) return;

	FGameEffectData UseEffectData = *EffectData;

	//플레이어에게 붙어서 재생하고 싶으면 true
	UseEffectData.SpawnLocationType = EGameEffectSpawnLocationType::ActorLocation;
	UseEffectData.SpawnRotationType = EGameEffectSpawnRotationType::ActorRotation;

	FGameEffectContext Context;
	Context.SourceActor = HealTarget;
	Context.SourceComponent = HealTarget->GetRootComponent();
	Context.WorldLocation = HealTarget->GetActorLocation();
	Context.WorldRotation = HealTarget->GetActorRotation();

	FGameEffectRuntimeParams Params;
	Params.AddFloatParam(TEXT("User.LifeTime"), 1.f);

	HealTarget->EffectManagerComp->PlayGameEffect_Multicast(UseEffectData, Context, Params);
}

void AObjects_HealingTotem::UpdateLifeTimeEffectActive_Local()
{
	bool bShouldActive = !bHealPaused && !bIsDestroyed;

	if (!LifeTimeEffectComp) return;
	if (bShouldActive) {
		LifeTimeEffectComp->SetHiddenInGame(false);
		LifeTimeEffectComp->SetVisibility(true, true);

		if (!LifeTimeEffectComp->IsActive()) LifeTimeEffectComp->Activate(true);
	}
	else {
		LifeTimeEffectComp->Deactivate();
		LifeTimeEffectComp->SetVisibility(false, true);
		LifeTimeEffectComp->SetHiddenInGame(true);
	}
}

void AObjects_HealingTotem::Multicast_PlayHealEffect_Implementation() {
	RefreshHPWidget();
}


void AObjects_HealingTotem::Multicast_SetLifeTimeEffectActive_Implementation(bool bActive)
{
	if (!LifeTimeEffectComp) return;
	if (bActive) {
		LifeTimeEffectComp->SetHiddenInGame(false);
		LifeTimeEffectComp->SetVisibility(true, true);
		LifeTimeEffectComp->Activate(true);
	}
	else {
		LifeTimeEffectComp->Deactivate();
		LifeTimeEffectComp->SetVisibility(false, true);
		LifeTimeEffectComp->SetHiddenInGame(true);
	}
}
