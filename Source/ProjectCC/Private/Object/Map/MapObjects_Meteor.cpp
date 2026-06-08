// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Map/MapObjects_Meteor.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "Areas/Area_FireArea.h"
#include "Engine/OverlapResult.h"
#include "Components/PrimitiveComponent.h"

AMapObjects_Meteor::AMapObjects_Meteor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bReplicates = true;       
	SetReplicateMovement(true); 
}

void AMapObjects_Meteor::ApplyAdditionalSetting()
{
	Super::ApplyAdditionalSetting();

	if (!HasAuthority()) return; // 서버에서만 처리
	if (!PhysicsCollider) return;
	if (!NowMap) return;

	PhysicsCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 물리+쿼리 충돌 활성화
	PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Ignore);            // 모든 채널 무시
	PhysicsCollider->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // 지형(WorldStatic)에만 충돌

	PhysicsCollider->SetSimulatePhysics(true); // 물리 시뮬레이션 활성화
	PhysicsCollider->SetEnableGravity(true);   // 중력 활성화 — 자유낙하
	PhysicsCollider->SetNotifyRigidBodyCollision(true);
	PhysicsCollider->WakeAllRigidBodies();     // 물리 바디 즉시 활성화

	PhysicsCollider->OnComponentHit.AddDynamic(this, &AMapObjects_Meteor::OnMeteorHit); // 지형 충돌 시 OnMeteorHit 바인딩
}

void AMapObjects_Meteor::OnMeteorHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return; // 서버에서만 처리
	if (bHasLanded) return;
	bHasLanded = true;
	PhysicsCollider->SetSimulatePhysics(false);	// 물리 정지 -> 바운스 차단
	PhysicsCollider->OnComponentHit.RemoveDynamic(this, &AMapObjects_Meteor::OnMeteorHit);
	OnLanded(Hit);               // 착지 처리로 위임
}

void AMapObjects_Meteor::OnLanded(const FHitResult& Hit)
{
	if (!NowMap) {
		Destroy(); // 맵 없으면 즉시 파괴
		return;
	}

	float BS = NowMap->BlockSize;         // 블록 크기 (월드 단위)
	FVector LandCenter = Hit.ImpactPoint; // 충돌 지점을 폭발 중심으로 사용

	FCollisionObjectQueryParams PawnQuery;
	PawnQuery.AddObjectTypesToQuery(ECC_Pawn); // Pawn 채널만 쿼리 

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(
		Overlaps, LandCenter, FQuat::Identity,
		PawnQuery,
		FCollisionShape::MakeBox(FVector(BS * 0.5f * 3, BS * 0.5f * 3, BS)) // 1칸 범위 박스
	);

	for (const FOverlapResult& Result : Overlaps) {
		APlayer_Character* Target = Cast<APlayer_Character>(Result.GetActor());
		if (!Target) continue;         // 플레이어가 아니면 스킵
		if (Target->IsOut()) continue; // 탈락한 플레이어는 스킵

		Target->ApplyDamageInternal(DamageAmount, nullptr, this, false, false); // 폭발 데미지 적용
	}

	if (Area_FireArea) {
		SpawnArea(Area_FireArea, GetAreaCenterLocation(), this, nullptr, nullptr, 1, 1); // 착지 위치에 1칸 화염 지역 생성
	}

	Destroy(); // 착지 후 자신 파괴
}
