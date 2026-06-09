// Fill out your copyright notice in the Description page of Project Settings.


#include "Coin.h"
#include "Player_Character.h"
#include "GameFramework/Character.h"
#include "PlayMode_Match.h"
#include "MapConstructor.h"
#include "EffectManagerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ACoin::ACoin()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	NetUpdateFrequency = 60.f;
	MinNetUpdateFrequency = 30.f;
	
	//플레이어를 탐지하는 Collider 부착
	Detect = CreateDefaultSubobject<USphereComponent>(TEXT("Detect Range"));
	Detect->InitSphereRadius(30.f);
	SetRootComponent(Detect);
	//플레이어 탐지 Collider 설정
	Detect->SetCollisionObjectType(ECC_GameTraceChannel3);
	Detect->SetCollisionResponseToAllChannels(ECR_Block);
	//카메라 충돌 방지
	Detect->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Detect->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Detect->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);

	//블록 감지 Hit 이벤트 델리게이트 바인딩
	Detect->OnComponentHit.AddDynamic(this, &ACoin::OnCoinHit);

	//코인 매쉬 컴포넌트 부착
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetupAttachment(Detect);

	//이펙트 담당 컴포넌트 부착
	EffectManagerComp = CreateDefaultSubobject<UEffectManagerComponent>(TEXT("EffectManager"));
}

// Called when the game starts or when spawned
void ACoin::BeginPlay()
{
	Super::BeginPlay();

	if (Detect) {
		Detect->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Detect->SetSimulatePhysics(true);
		Detect->SetGenerateOverlapEvents(true);
		Detect->SetNotifyRigidBodyCollision(true);
		//코인 공기저항 설정
		Detect->SetLinearDamping(0.5f);
		//회전 저항 (굴러다니는 현상 방지)
		Detect->SetAngularDamping(2.0f);
		//질량 조절(가벼우면 너무 날아다니므로 무게감을 줍니다)
		// 숫자가 클수록 물리 엔진이 더 무거운 물체로 취급
		Detect->SetMassScale(NAME_None, 5.f);
		// 수면 모드 설정 (움직임이 작아지면 물리 연산을 빨리 멈추게 함)
		// 코인이 바닥에서 미세하게 떨리거나 계속 미끄러지는 걸 방지
		Detect->BodyInstance.SleepFamily = ESleepFamily::Sensitive;
	}
	
	//코인 회전 초기값 (Z축 기준으로만 회전시키고 나머지는 회전 X)
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	//플레이어 감지 Overlap이벤트 델리게이트 바인딩
	Detect->OnComponentBeginOverlap.AddDynamic(this, &ACoin::OnTriggerBeginOverlap);
	UWorld* World = GetWorld();
	if (!World) return;
	APlayMode_Match* GM = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
	if (!GM) return;
	NowMap = GM->GetCurrentMap();
	if (!NowMap) return;

}


// Called every frame
void ACoin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority()) {
		if (bIsFlying) {
			CheckLandingBeforeHit();
		}
		else if (bFirstBounce) {
			TrackBounceDistance();
		}
	}
	//코인 회전 설정 (Z축 기준으로만 회전시키고 나머지는 회전 X)
	float Coin_Yaw = GetActorRotation().Yaw;
	SetActorRotation(FRotator(0.f, Coin_Yaw, 0.f));
	Mesh->AddLocalRotation(FRotator(0.f, SpinSpeed * DeltaTime, 0.f));
}

void ACoin::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(LastOwnerClearTimerHandle);
		World->GetTimerManager().ClearTimer(EnableCoinCollisionTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ACoin::OnCoinHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bIsFlying || !HasAuthority()) return;
	if (!HitComponent) return;
	//자기 자신은 제외
	if (!OtherActor || OtherActor == this) return;

	UPrimitiveComponent* CoinComp = GetObjectPhysicsCollider();
	if (!CoinComp) return;

	FVector CurrentVelocity = CoinComp->GetPhysicsLinearVelocity();

	if (CurrentVelocity.Z > 0.f) return;
	if (Hit.ImpactNormal.Z >= 0.7) return;


	CoinComp->SetLinearDamping(LandingLinearDamping);
	CoinComp->SetAngularDamping(LandingAngularDamping);

	CurrentVelocity.X *= LandingXYVelocityScale;
	CurrentVelocity.Y *= LandingXYVelocityScale;

	CoinComp->SetPhysicsLinearVelocity(CurrentVelocity);
	CoinComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

//시작 위치에서 대상 위치까지 포물선으로 날아가기 위한 속도를 계산하여 적용
void ACoin::StartCoinLaunchInternal(const FVector& StartLocation, const FVector& TargetLocation) {
	UWorld* World = GetWorld();
	if (!World) return;
	UPrimitiveComponent* CoinComp = GetObjectPhysicsCollider();
	if (!CoinComp) return;
	bFirstBounce = false;
	bIsFlying = true;
	LandingStartLocation = FVector::ZeroVector;
	SetActorLocation(StartLocation, false, nullptr, ETeleportType::TeleportPhysics);
	DisableCoinCollisonWithCoin();

	CoinComp->SetSimulatePhysics(true);
	CoinComp->SetEnableGravity(true);
	CoinComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CoinComp->WakeAllRigidBodies();

	CoinComp->SetLinearDamping(0.f);
	CoinComp->SetAngularDamping(0.f);
	CoinComp->SetAllUseCCD(true);

	
	FVector LaunchVelocity = CalculateLaunchVelocity(StartLocation, TargetLocation);
	CoinComp->SetPhysicsLinearVelocity(LaunchVelocity);
}

//코인이 날아갈 속도를 계산
FVector ACoin::CalculateLaunchVelocity(const FVector& StartLocaton, const FVector& TargetLocation) {
	UWorld* World = GetWorld();
	if (!World) return FVector::ZeroVector;
	//포물선 코인 발사 속도 계산 식
	float GravityZ = FMath::Abs(World->GetGravityZ());
	if (GravityZ <= KINDA_SMALL_NUMBER) return FVector::ZeroVector;

	FVector StartLocation = GetActorLocation();
	FVector EndLocation = TargetLocation;
	float MaxArcZ = FMath::Max(StartLocation.Z, EndLocation.Z) + 100.f;

	if (MaxArcZ <= StartLocation.Z || MaxArcZ <= EndLocation.Z) return FVector::ZeroVector;

	float UpHeight = MaxArcZ - StartLocation.Z;
	float TimeUp = FMath::Sqrt((2.f * UpHeight) / GravityZ);

	float DownHeight = MaxArcZ - EndLocation.Z;
	float TimeDown = FMath::Sqrt((2.f * DownHeight) / GravityZ);

	float TotalTime = TimeUp + TimeDown;
	if (TotalTime <= KINDA_SMALL_NUMBER) return FVector::ZeroVector;

	FVector LaunchVelocity = FVector::ZeroVector;

	LaunchVelocity.X = (EndLocation.X - StartLocation.X) / TotalTime;
	LaunchVelocity.Y = (EndLocation.Y - StartLocation.Y) / TotalTime;
	LaunchVelocity.Z = GravityZ * TimeUp;

	return LaunchVelocity;
}

//서버/로컬에 멀티캐스트로 코인 발사 요청
void ACoin::LaunchToTargetLocation(FVector& StartLocation, FVector& TargetLocation)
{
	if (!HasAuthority()) return;
	Mulitcast_StartCoinLaunch(StartLocation, TargetLocation);
	//초기 상태를 강제로 복제
	ForceNetUpdate();
}

//코인이 바닥에 닿기 직전을 감지하여 속도 조절 
void ACoin::CheckLandingBeforeHit() {
	UWorld* World = GetWorld();
	UPrimitiveComponent* CoinComp = GetObjectPhysicsCollider();

	if (!World || !CoinComp) return;
	FVector CurrentVelocity = CoinComp->GetPhysicsLinearVelocity();

	if (CurrentVelocity.Z >= 0.f) return;
	FVector CurrentLocation = CoinComp->GetComponentLocation();

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CoinLandingTrace), false, this);
	QueryParams.AddIgnoredActor(this);

	//코인 아래를 검사하기 위한 범위 설정
	FVector TraceStart = CurrentLocation;
	FVector TraceEnd = CurrentLocation - FVector(0.f, 0.f, Detect->GetScaledSphereRadius() + LandingTraceDistance);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	bool bHitGround = World->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, ObjectQueryParams, QueryParams);

	if (!bHitGround) return;
	if (Hit.ImpactNormal.Z < 0.7f) return;

	//바닥 근처라면 새로운 속도 및 Damp 적용
	CoinComp->SetLinearDamping(LandingLinearDamping);
	CoinComp->SetAngularDamping(LandingAngularDamping);

	FVector NewVelocity = CurrentVelocity;
	NewVelocity.X *= LandingXYVelocityScale;
	NewVelocity.Y *= LandingXYVelocityScale;
	NewVelocity.Z *= LandingZVelocityScale;

	CoinComp->SetPhysicsLinearVelocity(NewVelocity);
	CoinComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	//코인의 현재 상태 변경
	bIsFlying = false;
	bFirstBounce = true;
	LandingStartLocation = CurrentLocation;
}
//코인의 튕김 거리 제한
void ACoin::TrackBounceDistance() {
	UPrimitiveComponent* CoinComp = GetObjectPhysicsCollider();
	if (!CoinComp) return;

	//블록 사이즈 기준 이동 최대 거리 설정
	float BlockSize = 100.f;
	if(NowMap) BlockSize = NowMap->BlockSize;
	float MaxBounceTravel = BlockSize * BounceTravelLimitMultiplier;

	//현재 이동 거리 계산
	FVector CurrentLocation = CoinComp->GetComponentLocation();
	float TravelDist2D = FVector::Dist2D(LandingStartLocation, CurrentLocation);

	//최대 거리 이상으로 이동 시 즉시 속도 제거, Damp를 크게 올림
	if (TravelDist2D >= MaxBounceTravel) {
		CoinComp->SetLinearDamping(StopLinearDamping);
		CoinComp->SetAngularDamping(StopAngularDamping);
		CoinComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CoinComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

		bFirstBounce = false;
		return;
	}

	//거의 멈췄으면 추적 종료
	FVector CurrentVelocity = CoinComp->GetPhysicsLinearVelocity();
	if (CurrentVelocity.SizeSquared2D() < 0.5f && FMath::Abs(CurrentVelocity.Z) < 0.5f) { 
		CoinComp->SetLinearDamping(StopLinearDamping);
		CoinComp->SetAngularDamping(StopAngularDamping);
		CoinComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CoinComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		bFirstBounce = false; 
		return;
	}
}

void ACoin::DisableCoinCollisonWithCoin()
{
	if (!Detect) return;
	Detect->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Ignore);
	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(EnableCoinCollisionTimerHandle);
	World->GetTimerManager().SetTimer(EnableCoinCollisionTimerHandle, this, &ACoin::EnableCoinCollisionWithCoin, 1.5f, false);
}


void ACoin::EnableCoinCollisionWithCoin()
{
	if (!Detect) return;
	Detect->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
}

//손실 플레이어 설정 (일정 시간 동안 손실한 플레이어는 획득 불가)
void ACoin::SetLastOwner(APlayer_Character* Player)
{
	LastOwnerPlayer = Player;
	UWorld* World = GetWorld();
	if (!World) return;
	World->GetTimerManager().ClearTimer(LastOwnerClearTimerHandle);
	World->GetTimerManager().SetTimer(LastOwnerClearTimerHandle, this, &ACoin::ClearLastOwner, CoinGetInterval, false);
}

void ACoin::ClearLastOwner() {
	LastOwnerPlayer = nullptr;
}

//플레이어의 코인 획득 기능
void ACoin::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (bIsTaken) return;

	//코인 획득 Interval 적용
	if (GetGameTimeSinceCreation() < CoinGetInterval) return;

	APlayer_Character* Player = Cast<APlayer_Character>(OtherActor);
	if (!Player) return;
	APlayer_State* PlayerState = Player->GetThePlayerState();
	if (PlayerState && Player != LastOwnerPlayer) {
		bIsTaken = true;
		PlayerState->AddCoin(CoinValue);
		if (APlayMode_Match* Match = GetWorld()->GetAuthGameMode<APlayMode_Match>()) {
			Match->UpdatePlayersRank();
		}
	}	
	Destroy();
}

//서버-로컬 멀티캐스트
void ACoin::Mulitcast_StartCoinLaunch_Implementation(const FVector_NetQuantize& StartLocation, const FVector_NetQuantize& TargetLocation) {
	StartCoinLaunchInternal(StartLocation, TargetLocation);
}


UPrimitiveComponent* ACoin::GetObjectPhysicsCollider()
{
	return Detect;
}