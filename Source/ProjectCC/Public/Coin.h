// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Coin.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class AMapConstructor;
class APlayer_Character;
class UEffectManagerComponent;

UCLASS()
class PROJECTCC_API ACoin : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACoin();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UFUNCTION()
	void OnCoinHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;
	//플레이어 탐지 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USphereComponent* Detect;
	//이펙트 담당 컴포넌트
	UPROPERTY()
	UEffectManagerComponent* EffectManagerComp;
	//현재 매치에서 사용 중인 맵
	UPROPERTY()
	TObjectPtr<AMapConstructor> NowMap = nullptr;
	//코인 가치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coin")
	int32 CoinValue = 1;
	//코인 회전 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coin")
	float SpinSpeed = 250.f;
	//코인 재획득 간격(손실 코인)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coin")
	float CoinGetInterval = 0.5f;
	//코인 재획득 쿨타임
	UPROPERTY()
	float CoinReCollectCoolTime = -1.f;
	//최근 코인 주인(손실 코인)
	UPROPERTY()
	TObjectPtr<APlayer_Character> LastOwnerPlayer = nullptr;
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	//로컬에 생성된 코인을 서버 코인 발사에 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Mulitcast_StartCoinLaunch(const FVector_NetQuantize& StartLocation, const FVector_NetQuantize& TargetLocation);
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	//코인의 포물선 이동 설정
	void LaunchToTargetLocation(FVector& StartLocation, FVector& TargetLocation);
	void CheckLandingBeforeHit();
	void TrackBounceDistance();
	//코인끼리의 충돌 제어
	void DisableCoinCollisonWithCoin();
	void EnableCoinCollisionWithCoin();
	//코인끼리 충돌 제어 타이머
	FTimerHandle EnableCoinCollisionTimerHandle;
	//서버/로컬 공용 실행 함수
	void StartCoinLaunchInternal(const FVector& StartLocation, const FVector& TargetLocation);
	//포물선 초기 속도 계산
	FVector CalculateLaunchVelocity(const FVector& StartLocation, const FVector& TargetLocation);
	//코인이 공중에 있는 지를 확인
	bool bIsFlying = false;
	//코인이 한번 튕겼는지 확인
	bool bFirstBounce = false;
	//코인 최초 착지 위치
	FVector LandingStartLocation = FVector::ZeroVector;
	//착지 시 이동 추적 거리
	float LandingTraceDistance = 10.f;
	//코인을 포물선 이동 시간
	float MinFlightTime = 0.35;
	float MaxFlightTime = 0.9f;
	//수평 거리를 비행 시간으로 바꿀때 사용하는 배율 (값이 클수록 비행시간이 늘어남)
	float XYDistanceToFlightTimeScale = 2.0f;
	//발사 시 Z축 추가 속도 
	float LaunchZAdditionalVelocity = 0.f;
	//발사 시 선형 속도/회전 Damp값
	float LaunchLinearDamping = 0.f;
	float LaunchAngularDamping = 0.f;
	//착지 시 선형 속도/회전 Damp값
	float LandingLinearDamping = 0.5f;
	float LandingAngularDamping = 0.9f;
	//착지 시 남아있는 속도 배율
	float LandingXYVelocityScale = 0.1f;
	float LandingZVelocityScale = 0.5f;
	//정지 시 선형 속도/회전 Damp값
	float StopLinearDamping = 150.f;
	float StopAngularDamping = 120.f;
	float BounceTravelLimitMultiplier = 0.5f;
	//코인 발사/착지 보정
	FTimerHandle LandingBrakeTimerHandle;
	//코인 착지 위치
	FVector LandTargetLocation = FVector::ZeroVector;
	//물체 물리 Collider 반환 (물리 계산시 사용)
	UPrimitiveComponent* GetObjectPhysicsCollider();
	//코인 손실 플레이어 설정/초기화
	void SetLastOwner(APlayer_Character* Player);
	void ClearLastOwner();
	FTimerHandle LastOwnerClearTimerHandle;
	//코인 획득 유무(중복 획득 방지)
	bool bIsTaken = false;
	//자석에 의해 끌려가는 중인지 확인(최초 캡처한 자석 효과에만 적용, 중복 방지)
	bool bIsCaptured = false;
};
