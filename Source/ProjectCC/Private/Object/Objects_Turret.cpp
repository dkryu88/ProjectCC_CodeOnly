// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_Turret.h"
#include "Components/BoxComponent.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "Kismet/GameplayStatics.h"
#include "Match_PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "PlayMode_Match.h"
//디버그드로우
#include "DrawDebugHelpers.h"

AObjects_Turret::AObjects_Turret(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

	Type = EObjectsType::Install;

	AttackRangeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackRangeBox"));
	AttackRangeBox->SetupAttachment(RootComponent);

	AttackRangeBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AttackRangeBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackRangeBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	AttackRangeBox->SetBoxExtent(FVector(250.f, 250.f, 200.f));
}

void AObjects_Turret::ApplyAdditionalSetting() {
	Super::ApplyAdditionalSetting();


	if (HasAuthority() && !NowMap) {
		if (APlayMode_Match* GM = GetWorld()->GetAuthGameMode<APlayMode_Match>()) {
			NowMap = GM->GetCurrentMap();
		}
	}

	if (HasAuthority() && NowMap && Type == EObjectsType::Install) {
		float BoxExtent = (NowMap->BlockSize * 5.f) / 2.f;
		AttackRangeBox->SetBoxExtent(FVector(BoxExtent, BoxExtent, NowMap->BlockSize * 2.f));

		// 바인딩 되지 않았다면, 바인딩
		if (!AttackRangeBox->OnComponentBeginOverlap.IsAlreadyBound(this, &AObjects_Turret::OnAttackBoxBeginOverlap)) {
			AttackRangeBox->OnComponentBeginOverlap.AddDynamic(this, &AObjects_Turret::OnAttackBoxBeginOverlap);
		}

		if (PhysicsCollider) {
			PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel4);
		}

		AttackRangeBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		TryAttackTarget();
	}

}

void AObjects_Turret::Func_Persist_Implementation(float DeltaTime)
{
	TryAttackTarget();
}

void AObjects_Turret::OnAttackBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (!HasAuthority())return;

	APlayer_Character* EnemyPlayer = Cast<APlayer_Character>(OtherActor);

	if (!EnemyPlayer->IsOut() && EnemyPlayer->HP > 0.0f) {
		if (OwnPlayerController && EnemyPlayer->GetController() == OwnPlayerController) {
			return;
		}
		TryAttackTarget();
	}

}

void AObjects_Turret::TryAttackTarget() {

	if (!HasAuthority())return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	// 쿨타임 계산
	if (CurrentTime - LastPassiveFunctionTime < FunctionInterval) return;

	TArray<AActor*> OverlappingActors;
	AttackRangeBox->GetOverlappingActors(OverlappingActors, APlayer_Character::StaticClass());

	AActor* FinalTarget = nullptr;
	float MinDistanceSq = TNumericLimits<float>::Max();
	FVector TurretLocation = GetActorLocation();

	// 시작점 높이를 절반만큼 올려 중심이 아닌 꼭대기에서 공격하는것으로 변경
	if (UBoxComponent* Box = Cast<UBoxComponent>(PhysicsCollider)) {
		TurretLocation.Z += Box->GetScaledBoxExtent().Z;
	}

	//범위 내에 적이 있는지 확인하는 플레그->타이머 조건으로 사용
	bool bHasVaildEnemyInRange = false;

	//범위 내의 공격 대상(적)을 탐색
	for (AActor* Actor : OverlappingActors) {
		APlayer_Character* EnemyPlayer = Cast<APlayer_Character>(Actor);
		if (!EnemyPlayer || EnemyPlayer->IsOut() || EnemyPlayer->HP <= 0.0f)continue;
		// 컨트롤러로 소유자 확인, 부활 이후에도 소유자 유지
		if (OwnPlayerController && EnemyPlayer->GetController() == OwnPlayerController) continue;

		bHasVaildEnemyInRange = true;

		FVector StartLoc = TurretLocation;
		FVector EndLoc = EnemyPlayer->GetActorLocation();
		float DistanceSq = FVector::DistSquared(StartLoc, EndLoc);

		//대상자 중 가장 가까운 대상을 타겟팅
		if (DistanceSq < MinDistanceSq) {
			FHitResult HitResult;
			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(this);
			if (OwnPlayerController && OwnPlayerController->GetPawn()) {
				TraceParams.AddIgnoredActor(OwnPlayerController->GetPawn());
			}

			bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, TraceParams);

			DrawDebugLine(GetWorld(), StartLoc, EndLoc, bHit ? FColor::Green : FColor::Red, false, 1.0f, 0, 2.0f);

			if (bHit) {
				AActor* HitActor = HitResult.GetActor();

				if (APlayer_Character* HitPlayer = Cast<APlayer_Character>(HitActor)) {
					if (OwnPlayerController && HitPlayer->GetController() != OwnPlayerController) {
						MinDistanceSq = DistanceSq;
						FinalTarget = HitPlayer;
					}
				}
				else if (AObjects* HitObject = Cast<AObjects>(HitActor)) {
					if (OwnPlayerController && HitObject->OwnPlayerController != OwnPlayerController) {
						MinDistanceSq = DistanceSq;
						FinalTarget = HitObject;

					}
				}
			}
			else {
				MinDistanceSq = DistanceSq;
				FinalTarget = EnemyPlayer;
			}
		}
	}
	if (FinalTarget) {
		// 실질적인 공격 함수
		if (APlayer_Character* TargetPlayer = Cast<APlayer_Character>(FinalTarget)) {
			TargetPlayer->ApplyDamageInternal(Damage, OwnPlayer, this, true, false, false);
		}
		else if (AObjects* TargetObjects = Cast<AObjects>(FinalTarget)) {
			TargetObjects->ApplyDamageInternal(Damage, OwnPlayer, this, false, false);
		}
		LastPassiveFunctionTime = CurrentTime;
	}
}

