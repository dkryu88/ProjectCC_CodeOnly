// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects.h"
#include "Net/UnrealNetwork.h"
#include "ObjectsDataAsset.h"
#include "Weapon.h"
#include "Area.h"
#include "Match_PlayerController.h"
#include "Player_Character.h"
#include "PlayerTransformationComponent.h"
#include "PlayMode_Match.h"
#include "MapConstructor.h"
#include "Objects_HPWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AObjects::AObjects(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	//물체 물리 Collider 부착 (상속 받은 객체가 재설정)
	PhysicsCollider = CreateDefaultSubobject<UShapeComponent, UBoxComponent>(TEXT("PhysicsCollider"));
	SetRootComponent(PhysicsCollider);
	//물리 동기화
	PhysicsCollider->SetIsReplicated(true);

	//물체 HP Widget
	HPWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPWidget"));
	HPWidgetComp->SetupAttachment(RootComponent);
	HPWidgetComp->SetRelativeLocation(FVector::ZeroVector);
	HPWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);

	HPWidgetComp->SetDrawAtDesiredSize(false);
	HPWidgetComp->SetDrawSize(FVector2D(120.f, 20.f));
	HPWidgetComp->SetPivot(FVector2D(0.5f, 0.5f));

	HPWidgetComp->SetVisibility(true);
	HPWidgetComp->SetHiddenInGame(false);

	//물체 매쉬 부착(실제 물리작용 x)
	MeshPivot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshPivot"));
	MeshPivot->SetupAttachment(PhysicsCollider);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetupAttachment(MeshPivot);
	//물체 상호작용/장착 Collider 부착
	InterActionCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("InterActionCollider"));
	InterActionCollider->InitBoxExtent(FVector(50.f, 50.f, 50.f));
	InterActionCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InterActionCollider->SetCollisionObjectType(ECC_GameTraceChannel1);
	InterActionCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	//ECC_GameTraceChannel1 <- 콜리전 프리셋 1번 (Interaction)
	InterActionCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	InterActionCollider->SetGenerateOverlapEvents(true);
	InterActionCollider->SetupAttachment(PhysicsCollider);

}

/*실제 자식 class PhysicsCollider 생성 예시
AChildObject::AChildObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USphereComponent>(TEXT("PhysicsCollider")))
{
}
*/

void AObjects::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AObjects, Type);
	DOREPLIFETIME(AObjects, HP);
	DOREPLIFETIME(AObjects, LifeTime);
	DOREPLIFETIME(AObjects, bIsEquipped);
	DOREPLIFETIME(AObjects, bNowActivated);
	DOREPLIFETIME(AObjects, FunctionInterval);
	DOREPLIFETIME(AObjects, OwnPlayer);
	DOREPLIFETIME(AObjects, AttackRange);
	DOREPLIFETIME(AObjects, StartLocation);
	DOREPLIFETIME(AObjects, TargetLocation);
	DOREPLIFETIME(AObjects, MoveDirection);
	DOREPLIFETIME(AObjects, bHaveThrowDamage);
	DOREPLIFETIME(AObjects, ThrowDamage);
}

// Called when the game starts or when spawned
void AObjects::BeginPlay()
{
	Super::BeginPlay();

	SetPhysicsCollider();

	if (HasAuthority()) {
		//물체 스탯 초기화
		SetObjectsStat();
		//물체 상태 적용
		if (PhysicsCollider) {
			PhysicsCollider->OnComponentHit.AddDynamic(this, &AObjects::OnPhysicsColliderHit);
		}
		//생성 기능 발동
		Func_Spawn();
	}

	ApplyCurrentState();

	//물체가 HP 위젯을 사용하는 경우 HP 위젯 초기화 및 HPWidgetComp에 위젯 부착
	if (ObjectsData && ObjectsData->bUseHPWidget && HPWidgetComp && Objects_HPWidget) {
		HPWidgetComp->SetWidgetClass(Objects_HPWidget);
		HPWidgetComp->InitWidget();
		UObjects_HPWidget* HPWidget = Cast<UObjects_HPWidget>(HPWidgetComp->GetUserWidgetObject());
		if (HPWidget) {
			HPWidget->InitWidget(this);
		}
	}

	UWorld* World = GetWorld();
	if (!World) return;
	APlayMode_Match* GM = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
	if (!GM) return;
	NowMap = GM->GetCurrentMap();
	if (!NowMap) return;
}

void AObjects::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);
	if (MeshPivot) {
		MeshPivot->SetRelativeLocation(MeshLocation);
		MeshPivot->SetRelativeRotation(MeshRotation);
	}
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(MeshScale);
	}
	//Physics Collider의 종류에 따라 결정
	if (PhysicsCollider) {
		if (UBoxComponent* Box = Cast<UBoxComponent>(PhysicsCollider)) {
			SetSizeofBoxColliderwithMesh(Box);
		}
		else if (USphereComponent* Sphere = Cast<USphereComponent>(PhysicsCollider)) {
			SetSizeofSphereColliderwithMesh(Sphere);
		}
	}

}

//LifeTime이 0이되어 제거될 때 Func_Destory 발동
void AObjects::LifeSpanExpired()
{
	Func_Destroy();

	Super::LifeSpanExpired();
}

// Called every frame
void AObjects::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHPWidgetWorldTransform();

	if (!HasAuthority() || !ObjectsData) return;
	if (!bIsEquipped && ObjectsData->bUseLifeTime) {
		CurrentTick += DeltaTime;
		if (CurrentTick > TickInterval) {
			CurrentTick = 0.f;
			LifeTime -= 1.f;
			LifeTime = FMath::Max(LifeTime, 0.f);
			if (LifeTime <= 0.f) {
				Func_ZeroLife();
				Destroy();
				return;
			}
		}
	}
	if (!bIsEquipped && bHavePassiveFunction) {
		CurrentFTick += DeltaTime;
		if (bHavePassiveFunction == true) {
			if (CurrentFTick > FunctionInterval) {
				Func_Persist(DeltaTime);
				CurrentFTick = 0.f;
			}
		}
	}

	if (bTickMoveActive) {
		FVector PreviousLocation = GetActorLocation();
		TickMoveElapsedTime += DeltaTime;

		FVector NextLocation = UpdateTickMoveLocation(TickMoveElapsedTime);
		FVector DeltaLocation = NextLocation - PreviousLocation;

		FHitResult SweepHitResult;

		AddActorWorldOffset(DeltaLocation, true, &SweepHitResult, ETeleportType::None);

		//실제 이동 거리
		FVector ActualDeltaLocation = GetActorLocation() - PreviousLocation;
		MoveDistance += FVector(ActualDeltaLocation.X, ActualDeltaLocation.Y, 0.f).Size();

		if (SweepHitResult.bBlockingHit) {
			AActor* HitActor = SweepHitResult.GetActor();

			if (ShouldIgnoreOwnerCollisionActor(HitActor)) return;

			HandleObjectsHit(SweepHitResult);
			return;
		}
	}

	if (bTickMoveActive && Type == EObjectsType::Projectile && AttackRange > 0.f && MoveDistance >= AttackRange) {
		bTickMoveActive = false;
		Func_Destroy();
		Destroy();
		return;
	}
}

void AObjects::OnRep_Type() {
	ApplyCurrentState();
}

void AObjects::OnRep_HP() {
	RefreshHPWidget();
}

void AObjects::RefreshHPWidget()
{
	if (!HPWidgetComp || !ObjectsData || !ObjectsData->bUseHPWidget) return;

	if (UObjects_HPWidget* HPWidget = Cast<UObjects_HPWidget>(HPWidgetComp->GetUserWidgetObject()))
	{
		HPWidget->InitWidget(this);
		HPWidget->RefreshWidget();
	}
}

void AObjects::UpdateHPWidgetWorldTransform()
{
	if (!HPWidgetComp) return;

	FVector WidgetLocation = GetActorLocation() + FVector(0.f, 0.f, 120.f);

	if (PhysicsCollider) {
		FBoxSphereBounds Bounds = PhysicsCollider->Bounds;
		WidgetLocation.X = Bounds.Origin.X;
		WidgetLocation.Y = Bounds.Origin.Y;
		WidgetLocation.Z = Bounds.Origin.Z + Bounds.BoxExtent.Z + HPWidgetOffsetZ;
	}

	HPWidgetComp->SetWorldLocation(WidgetLocation);
	HPWidgetComp->SetWorldRotation(FRotator::ZeroRotator);
}

//물체의 물리 Collider 기본 설정
void AObjects::SetPhysicsCollider()
{
	if (!PhysicsCollider || bPhysicsColliderInitialized) return;

	bPhysicsColliderInitialized = true;

	PhysicsCollider->SetUsingAbsoluteLocation(false);
	PhysicsCollider->SetUsingAbsoluteRotation(false);
	PhysicsCollider->SetMobility(EComponentMobility::Movable);
	if (MeshPivot) {
		MeshPivot->SetupAttachment(PhysicsCollider);
	}
	if (InterActionCollider) InterActionCollider->SetupAttachment(PhysicsCollider);

}

//각 상태 별로 물리 Collider 설정
void AObjects::ApplyCurrentState() {
	SetPhysicsCollider();

	if (Type == EObjectsType::Support) {
		if (bIsEquipped) {
			if (!OwnPlayer) return;
			ApplySupportState();
			bRuntimeStateResolved = true;
		}
		else {
			ApplyNormalState();
			bRuntimeStateResolved = true;
		}
	}
	else if (Type == EObjectsType::Equip) {
		if (bIsEquipped) {
			if (!OwnPlayer) return;
			ApplyEquipState();
			bRuntimeStateResolved = true;
		}
		else {
			ApplyNormalState();
			bRuntimeStateResolved = true;
		}
	}
	else if (Type == EObjectsType::Install) {
		ApplyInstallState();
		bRuntimeStateResolved = true;
	}
	else if (Type == EObjectsType::Throwable || Type == EObjectsType::Projectile) {
		if (HasAuthority()) {
			ApplyShootorThrowState();
			bRuntimeStateResolved = true;
		}
	}
	else {
		ApplyNormalState();
		bRuntimeStateResolved = true;
	}
}

//물체의 기본 스탯을 설정
void AObjects::SetObjectsStat() {
	if (!ObjectsData)
	{
		return;
	}
	Type = ObjectsData->Type;
	DamageType = ObjectsData->DamageType;
	HP = ObjectsData->bUseHP ? ObjectsData->DefaultHP : -10.f;
	LifeTime = ObjectsData->bUseLifeTime ? ObjectsData->DefaultLifeTime : -10.f;
	FunctionInterval = ObjectsData->FunctionInterval;
	bHavePassiveFunction = ObjectsData->bHavePassiveFunction;
	Move_Speed = ObjectsData->Move_Speed;
}

//물체 Collider(Box) 크기를 계산
void AObjects::SetSizeofBoxColliderwithMesh(UBoxComponent* Collider)
{
	if (!Mesh || !Mesh->GetStaticMesh() || !Collider) return;
	FBoxSphereBounds MeshBounds = Mesh->GetStaticMesh()->GetBounds();
	FVector MeshSize = MeshPivot->GetRelativeScale3D().GetAbs();
	//Mesh의 각 크기값을 획득
	FVector BaseSize = MeshBounds.BoxExtent;
	BaseSize.X *= MeshSize.X;
	BaseSize.Y *= MeshSize.Y;
	BaseSize.Z *= MeshSize.Z;
	//최종 Collider 크기 계산
	FVector ColliderSize;
	ColliderSize.X = BaseSize.X * SizeMagnification.X + ColliderOffset.X;
	ColliderSize.Y = BaseSize.Y * SizeMagnification.Y + ColliderOffset.Y;
	ColliderSize.Z = BaseSize.Z * SizeMagnification.Z + ColliderOffset.Z;
	//최솟값 설정(음수 방지)
	ColliderSize.X = FMath::Max(ColliderSize.X, 0.01f);
	ColliderSize.Y = FMath::Max(ColliderSize.Y, 0.01f);
	ColliderSize.Z = FMath::Max(ColliderSize.Z, 0.01f);
	//최종 Collider 크기 적용
	Collider->SetBoxExtent(ColliderSize);
}
//물체 Collider(Sphere) 크기를 계산
void AObjects::SetSizeofSphereColliderwithMesh(USphereComponent* Collider) {
	if (!Mesh || !Mesh->GetStaticMesh() || !Collider) return;
	FBoxSphereBounds MeshBounds = Mesh->GetStaticMesh()->GetBounds();
	FVector MeshSize = MeshPivot->GetRelativeScale3D().GetAbs();
	//완전 원형이 아닐 경우 거리가 가장 먼 곳을 반지름으로 설정
	float MaxSize = MeshSize.GetMax();
	float Radius = MeshBounds.SphereRadius * MaxSize;
	//추가적인 Radius 크기 설정 (배율/offset)
	Radius = Radius * SizeMagnification.X + ColliderOffset.X;
	//반지름 최솟값 설정
	Radius = FMath::Max(Radius, 0.001f);
	//최종 Collider 크기 설정
	Collider->SetSphereRadius(Radius);
}
bool AObjects::PickedByPlayer(APlayer_Character* player) {
	if (!HasAuthority()) return false;
	if (!player) return false;
	//플레이어가 무기를 주웠다면 플레이어에서 무기 획득 함수 실행
	return player->PickObjects(this);
}
//물체 데미지 적용 전 처리 (플레이어에 의해 데미지를 받으면 플레이어 포인터 획득, 아니면 플레이어 포인터 NULL) <override 상태>
float AObjects::TakeDamage(float damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) {
	if (!HasAuthority()) return 0.0f;
	APlayer_Character* AttackPlayer = nullptr;
	if (EventInstigator) {
		AttackPlayer = Cast<APlayer_Character>(EventInstigator->GetPawn());
	}
	return ApplyDamageInternal(damage, AttackPlayer, DamageCauser, true, false);
}

//물체 데미지 적용 처리
float AObjects::ApplyDamageInternal(float Damage, APlayer_Character* AttackPlayer, AActor* DamageCauser, bool bApplyKnockBack, bool bForceDamage)
{
	if (!HasAuthority()) return 0.f;
	if (Damage < 0) return 0.f;
	if (!ObjectsData) return 0.f;

	FVector AttackDir = FVector::ZeroVector;
	float KnockBackStrength = 0.f;
	//물체가 피격당한 대상의 위치와 현재 물체의 위치를 통해 넉백 방향 계산
	if (AttackPlayer) {
		AttackDir = GetActorLocation() - AttackPlayer->GetActorLocation();
		KnockBackStrength = AttackPlayer->AStat.KnockBackStrength;
	}
	if (AObjects* Object = Cast<AObjects>(DamageCauser)) {
		AttackDir = GetActorLocation() - Object->GetActorLocation();
		KnockBackStrength = Object->ObjectsData->KnockBackStrength;
	}
	AttackDir.Z = 0.f;
	AttackDir = AttackDir.GetSafeNormal();
	//물체 넉백 적용
	if (AttackDir != FVector::ZeroVector && bApplyKnockBack) ApplyKnockBack(AttackDir, KnockBackStrength, 0);

	float FinalDamage = FMath::Max(0.f, Damage);

	//물체의 데미지 타입에 따라 차등 적용
	switch (DamageType) {
	case EObjectDamageType::Fix:
		if (bForceDamage) HP = FMath::Clamp(HP - FinalDamage, 0.0f, ObjectsData->DefaultHP);
		else HP = FMath::Clamp(HP - 1.f, 0.0f, ObjectsData->DefaultHP);
		break;
	case EObjectDamageType::Multiply:
		if (bForceDamage) HP = FMath::Clamp(HP - FinalDamage, 0.0f, ObjectsData->DefaultHP);
		else HP = FMath::Clamp(HP - (FinalDamage * ObjectsData->DamageManification), 0.0f, ObjectsData->DefaultHP);
		break;
	case EObjectDamageType::Full:
		HP = FMath::Clamp(HP - FinalDamage, 0.0f, ObjectsData->DefaultHP);
		break;
	}

	RefreshHPWidget();

	//체력이 0이 되면 파괴 (파괴 효과 발동)
	if (HP <= 0.0f) {
		Func_Destroy();
		Destroy();
	}
	//체력이 남으면 피격 효과 발동
	else {
		Func_AttackedByPlayer(AttackPlayer);
	}

	return FinalDamage;
}

//물체 넉백 처리
void AObjects::ApplyKnockBack(FVector& AttackDir, float Strength, float UpStrength) {
	if (!HasAuthority()) return;
	if (!ObjectsData->bCanMove) return;
	if (!PhysicsCollider) return;

	FVector LaunchPow = AttackDir * FMath::Max(0.f, (Strength / 2 - (ObjectsData->Weight * 100)));
	LaunchPow.Z = UpStrength;

	if (LaunchPow.IsNearlyZero()) {
		return;
	}
	PhysicsCollider->AddImpulse(LaunchPow, NAME_None, true);
}


//플레이어가 획득한 물체 정보 획득
void AObjects::GetObjectInfo(APlayer_Character* EPlayer)
{
	if (!HasAuthority()) return;
	if (!EPlayer) return;
	EPlayer->Weight += ObjectsData ? ObjectsData->Weight : 0;
}

//장착 상태 변경 시 서버에서 자동 호출
void AObjects::OnRep_IsEquipped() {
	ApplyCurrentState();
}

//물체 장착
void AObjects::Equip(APlayer_Character* Player) {
	if (!HasAuthority()) return;
	//장착 플레이어 지정
	if (!Player) return;
	//물체를 장착된 상태로 변경, 무기 매쉬의 물리 설정 변경
	bIsEquipped = true;
	OwnPlayer = Player;
	OwnPlayerController = Cast<AMatch_PlayerController>(Player->GetController());
	ApplyEquipState();
	if (!OwnPlayer) {
		UE_LOG(LogTemp, Error, TEXT("No Detected OwnPlayer"));
		return;
	}
	OwnPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
	//물체 장착 효과 발동
	Func_Equip(Player);
}

void AObjects::EquipSupport(APlayer_Character* Player)
{
	if (!HasAuthority() || !Player) return;

	bIsEquipped = true;
	OwnPlayer = Player;
	OwnPlayerController = Cast<AMatch_PlayerController>(Player->GetController());
	ApplySupportState();

	if (!OwnPlayer) return;
	OwnPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);

	Func_Equip(Player);
}

//장착 상태 적용
void AObjects::ApplyEquipState()
{
	if (InterActionCollider)
	{
		InterActionCollider->SetGenerateOverlapEvents(false);
		InterActionCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (PhysicsCollider) {
		if (PhysicsCollider) {
			PhysicsCollider->SetSimulatePhysics(false);
			PhysicsCollider->SetEnableGravity(false);
			PhysicsCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel5);
			PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
			PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer, true);
		}
		if (MeshPivot) {
			MeshPivot->SetRelativeLocation(MeshLocation);
			MeshPivot->SetRelativeRotation(MeshRotation);
		}

		if (OwnPlayer && OwnPlayer->GetMesh()) {
			AttachToComponent(OwnPlayer->ObjectsSlot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}

	}
	ApplyAdditionalSetting();
	bRuntimeStateResolved = true;
}

//물체 해제
void AObjects::UnEquip(APlayer_Character* Player) {
	if (!HasAuthority()) return;
	if (PhysicsCollider && OwnPlayer) {
		PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer, false);
		OwnPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ApplyNormalState();
	Func_UnEquip(Player);
	OwnPlayer = nullptr;
	OwnPlayerController = nullptr;
	bIsEquipped = false;
}

//물체 던지기 상태 설정
void AObjects::BeginObjectThrow(APlayer_Character* owner, float damage) {
	if (!HasAuthority()) return;
	OwnPlayer = owner;
	PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel6);
	PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer, true);
	OwnPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
	PhysicsCollider->SetNotifyRigidBodyCollision(true);
	PhysicsCollider->SetAllUseCCD(true);
	OwnPlayerController = Cast<AMatch_PlayerController>(owner->GetController());
	ThrowDamage = damage;
	bHaveThrowDamage = true;
}
void AObjects::EndObjectThrow() {
	if (!HasAuthority()) return;
	if (Type == EObjectsType::Equip) {
		if (PhysicsCollider && OwnPlayer) {
			PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer, false);
			OwnPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
		}
		ApplyNormalState();
	}
	OwnPlayer = nullptr;
	OwnPlayerController = nullptr;
	ThrowDamage = 0.f;
	bHaveThrowDamage = false;
}


//설치 상태 적용
void AObjects::ApplyInstallState() {
	if (!HasAuthority()) {
		return;
	}
	if (PhysicsCollider) {
		FHitResult Hit;
		//Line Trace 범위 설정
		FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
		FVector End = GetActorLocation() - FVector(0.f, 0.f, 500.f);
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

		//실제 물체의 바닥의 위치를 획득 후 위치 보정 (원점을 중심으로 설치하기 때문에 Z크기의 반만큼 올려줘야 함)
		if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, Params)) {
			float ObjectBottom = 0.f;
			if (UBoxComponent* Box = Cast<UBoxComponent>(PhysicsCollider)) {
				ObjectBottom = Box->GetScaledBoxExtent().Z;
			}
			else if (USphereComponent* Sphere = Cast<USphereComponent>(PhysicsCollider)) {
				ObjectBottom = Sphere->GetScaledSphereRadius();
			}
			else {
				ObjectBottom = PhysicsCollider->Bounds.BoxExtent.Z;
			}
			FVector InstallLocation = Hit.Location;
			InstallLocation.Z += ObjectBottom;
			SetActorLocation(InstallLocation);
		}
		//설치 후 이동 방지(물리 적용 x)
		PhysicsCollider->SetSimulatePhysics(false);
		PhysicsCollider->SetEnableGravity(true);
		PhysicsCollider->SetPhysicsLinearVelocity(FVector::ZeroVector);
		PhysicsCollider->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		PhysicsCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Block);
	}
	ApplyAdditionalSetting();
	bRuntimeStateResolved = true;
}

//물체 상호작용 상태
void AObjects::ApplyInteractionState(APlayer_Character* InterActionPlayer) {
	if (!HasAuthority() || !InterActionPlayer) {
		return;
	}
	UWorld* World = GetWorld();
	float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastInteractionTime < ObjectsData->InterActionInterval) return;
	if (bNowActivated) {
		bNowActivated = false;
		return;
	}
	else {
		bNowActivated = true;
		Func_Interaction(InterActionPlayer);
	}

}


//투척/발사/장착/서포트 타입 물체의 충돌/착지 처리
void AObjects::OnPhysicsColliderHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return;

	if (Type == EObjectsType::Throwable || Type == EObjectsType::Projectile) return;

	HandleObjectsHit(Hit);
}
//물체 발사/투척 상태
void AObjects::ApplyShootorThrowState()
{
	if (InterActionCollider) {
		InterActionCollider->SetGenerateOverlapEvents(false);
		InterActionCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (PhysicsCollider) {
		PhysicsCollider->SetPhysicsLinearVelocity(FVector::ZeroVector);
		PhysicsCollider->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		PhysicsCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel6);
		PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Block);
		//코인 무시
		PhysicsCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Ignore);
		//Equipment 무시
		PhysicsCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Ignore);
		//발사체/투사체 무시
		PhysicsCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel6, ECR_Ignore);
		PhysicsCollider->SetSimulatePhysics(false);
		PhysicsCollider->SetEnableGravity(false);
		PhysicsCollider->SetAllUseCCD(false);
		PhysicsCollider->SetNotifyRigidBodyCollision(false);
		if (OwnPlayer) {
			PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer, true);
		}
	}
	if (OwnPlayer) {
		HitDamage = OwnPlayer->AStat.Attack;
	}
	ApplyAdditionalSetting();
	bRuntimeStateResolved = true;
}
//발사/투척 시 무시하도록 설정한 물체의 Collsiion 충돌 복구 (발사/투척 타입)
void AObjects::EnableOwnerCollisionAgain()
{
	if (!HasAuthority()) return;
	if (!ObjectsData) return;

	if (!ObjectsData->bIgnoreOwnerCollision) {
		ApplyOwnerCollisionIgnore(false);
	}

	ForceNetUpdate();
}

FVector AObjects::UpdateTickMoveLocation(float Time)
{
	return TickMoveStartLocation + (TickMoveInitialVelocity * Time) + (0.5f * TickMoveGravity * Time * Time);
}

void AObjects::StartTickMovement(const FVector& initialVelocity, bool bUseGravity)
{
	bTickMoveActive = true;
	TickMoveElapsedTime = 0.f;
	MoveDistance = 0.f;

	bHavingHitPoint = false;
	HitPoint = FVector::ZeroVector;

	TickMoveStartLocation = GetActorLocation();
	TickMoveInitialVelocity = initialVelocity;

	TickMoveGravity = FVector::ZeroVector;

	if (bUseGravity) {
		if (UWorld* World = GetWorld()) {
			TickMoveGravity = FVector(0.f, 0.f, World->GetGravityZ());
		}
	}

	SetReplicateMovement(true);
}

void AObjects::ShootOrThrowWithLaunchData(APlayer_Character* UsePlayer, const FVector& TheStartLocation, const FVector& TheTargetLocation, const FVector& TheLaunchVelocity, bool bUseGravity)
{
	if (!HasAuthority() || !UsePlayer || !ObjectsData)
	{
		return;
	}

	OwnPlayer = UsePlayer;
	OwnPlayerController = Cast<AMatch_PlayerController>(UsePlayer->GetController());

	StartLocation = TheStartLocation;
	TargetLocation = TheTargetLocation;
	MoveDistance = 0.f;

	AttackRange = FVector::Dist2D(StartLocation, TargetLocation);

	SetActorLocation(StartLocation);

	MoveDirection = TargetLocation - StartLocation;
	MoveDirection.Z = 0.f;
	MoveDirection = MoveDirection.GetSafeNormal();

	ApplyShootorThrowState();
	ApplyOwnerCollisionIgnore(true);

	GetWorld()->GetTimerManager().ClearTimer(EnableThrowCollisionTimerhandle);

	if (!ObjectsData->bIgnoreOwnerCollision) {
		GetWorld()->GetTimerManager().SetTimer(EnableThrowCollisionTimerhandle, this, &AObjects::EnableOwnerCollisionAgain, 0.1f, false);
	}


	StartTickMovement(TheLaunchVelocity, bUseGravity);

	ForceNetUpdate();
}
void AObjects::ApplyOwnerCollisionIgnore(bool bIgnore)
{
	if (!PhysicsCollider || !OwnPlayer) return;

	bOwnerCollisionIgnored = bIgnore;

	PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer, bIgnore);

	if (OwnPlayer->GetCapsuleComponent()) {
		OwnPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(this, bIgnore);
	}

	if (OwnPlayer->NowWeapon && OwnPlayer->NowWeapon->PhysicsCollider) {
		PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer->NowWeapon, bIgnore);
		OwnPlayer->NowWeapon->PhysicsCollider->IgnoreActorWhenMoving(this, bIgnore);
	}

	if (OwnPlayer->NowObjects && OwnPlayer->NowObjects->PhysicsCollider) {
		PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer->NowObjects, bIgnore);
		OwnPlayer->NowObjects->PhysicsCollider->IgnoreActorWhenMoving(this, bIgnore);
	}

	if (OwnPlayer->NowSupport && OwnPlayer->NowSupport->PhysicsCollider) {
		PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer->NowSupport, bIgnore);
		OwnPlayer->NowSupport->PhysicsCollider->IgnoreActorWhenMoving(this, bIgnore);
	}
}

bool AObjects::ShouldIgnoreOwnerCollisionActor(AActor* OtherActor)
{
	if (!OtherActor || !OwnPlayer) return false;
	if (!IsOwnerActor(OtherActor)) return false;

	//현재 소유주 충돌 무시 상태인 경우에만 무시
	return bOwnerCollisionIgnored;
}

void AObjects::HandleObjectsHit(const FHitResult& Hit)
{
	if (!HasAuthority()) return;
	if (!ObjectsData) return;

	AActor* OtherActor = Hit.GetActor();

	if (Type == EObjectsType::Install || Type == EObjectsType::Normal) return;
	if (ShouldIgnoreOwnerCollisionActor(OtherActor)) return;

	bHavingHitPoint = true;
	HitPoint = Hit.ImpactPoint;

	if (OtherActor && OwnPlayer) {
		APlayer_Character* HitPlayer = Cast<APlayer_Character>(OtherActor);
		AObjects* HitObject = Cast<AObjects>(OtherActor);

		if (HitPlayer && !HitPlayer->IsOut()) {
			HitPlayer->ApplyDamageInternal(HitDamage, OwnPlayer, this, true, true, false);
			Func_HitPlayer(HitPlayer);
			LastHitPlayer = HitPlayer;
		}

		else if (HitObject && HitObject->ObjectsData && HitObject->ObjectsData->bUseHP) {
			HitObject->ApplyDamageInternal(HitDamage, OwnPlayer, this, true, false);
		}
	}

	//던져진 물체의 데미지 타입에 따라 반작용 데미지 차등 적용
	if (ObjectsData->bUseHP && bHaveThrowDamage) {
		switch (DamageType) {
		case EObjectDamageType::Fix:
			HP = FMath::Clamp(HP - 1.f, 0.0f, ObjectsData->DefaultHP);
			break;
		default:
			HP = FMath::Clamp(HP - 10.f, 0.0f, ObjectsData->DefaultHP);
			break;
		}
		Func_AttackedByPlayer(LastHitPlayer);
	}

	//월드 지형에 피격시 기능 적용 (물체 던지기)
	if (Type == EObjectsType::Equip && bHaveThrowDamage) {
		EndObjectThrow();
	}

	if ((ObjectsData->bUseHP && HP <= 0.0f)) {
		bTickMoveActive = false;
		Func_Destroy();
		Destroy();
		return;
	}

	if (Type == EObjectsType::Projectile || Type == EObjectsType::Throwable) {
		switch (ObjectsData->HitAction) {
		case EBulletObjectsHitAction::BecomeNormal:
			ChangeToNormalType(Hit);
			return;
		case EBulletObjectsHitAction::Destroy:
		default:
			bTickMoveActive = false;
			Func_Destroy();
			Destroy();
			return;
		}
	}
}

FVector AObjects::GetAreaCenterLocation()
{
	return bHavingHitPoint ? HitPoint : GetActorLocation();
}

//일반 상태 적용
void AObjects::ApplyNormalState() {
	bIsEquipped = false;
	if (InterActionCollider)
	{
		InterActionCollider->SetGenerateOverlapEvents(true);
		InterActionCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (PhysicsCollider) {
		PhysicsCollider->SetSimulatePhysics(true);
		PhysicsCollider->SetEnableGravity(true);
		PhysicsCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel4);
		PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Block);
		PhysicsCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		PhysicsCollider->WakeAllRigidBodies();
	}
	ApplyAdditionalSetting();
	bRuntimeStateResolved = true;
}
//지원 상태 적용
void AObjects::ApplySupportState()
{
	if (InterActionCollider) {
		InterActionCollider->SetGenerateOverlapEvents(false);
		InterActionCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (PhysicsCollider) {
		SetReplicateMovement(false);
		PhysicsCollider->SetSimulatePhysics(false);
		PhysicsCollider->SetEnableGravity(false);
		PhysicsCollider->PutRigidBodyToSleep();
		PhysicsCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
		if (OwnPlayer) {
			PhysicsCollider->IgnoreActorWhenMoving(OwnPlayer, true);
		}
	}

	if (OwnPlayer && OwnPlayer->SupportSlot && PhysicsCollider) {
		//플레이어의 SupportSlot에 부착
		AttachToComponent(OwnPlayer->SupportSlot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		SetActorRelativeLocation(FVector(XOffset, YOffset, ZOffset));
		SetActorRelativeRotation(FRotator::ZeroRotator);
	}

	ApplyAdditionalSetting();

	bRuntimeStateResolved = true;
}

void AObjects::SpawnArea(TSubclassOf<AArea> TheArea, const FVector& CenterLocation, AActor* AreaSpawner, APlayer_Character* AreaOwner, TArray<AArea*>* OutSpawnedArea, int32 offsetx, int32 offsety)
{
	UWorld* World = GetWorld();
	if (!World || !NowMap || !TheArea) return;

	int32 CenterX = 0;
	int32 CenterY = 0;
	int32 CenterZ = 0;
	FIntVector CandidateLocation = FIntVector(-1, -1, -1);
	AArea* RootArea = nullptr;

	if (!NowMap->FindNearestTopBlock(CenterLocation, CenterX, CenterY, CenterZ, 2)) return;

	for (int32 OffsetY = -offsety; OffsetY <= offsety; OffsetY++) {
		for (int32 OffsetX = -offsetx; OffsetX <= offsetx; OffsetX++) {
			CandidateLocation.X = CenterX + OffsetX;
			CandidateLocation.Y = CenterY + OffsetY;
			CandidateLocation.Z = CenterZ;

			AArea* Areas = World->SpawnActorDeferred<AArea>(TheArea, FTransform::Identity, AreaSpawner, AreaOwner, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Areas) continue;

			Areas->NowMap = NowMap;
			Areas->OwnPlayer = OwnPlayer;

			if (!Areas->InitializeArea(CandidateLocation.X, CandidateLocation.Y, CandidateLocation.Z)) {
				Areas->Destroy();
				continue;
			}

			if (AArea* ExistingArea = AArea::FindExistingAreaAtGrid(World, NowMap, Areas->GetGridLocation(), Areas)) {
				ExistingArea->DestroyOnlySameGridArea();
			}

			//Area 그룹의 Root 지정
			if (!RootArea) {
				RootArea = Areas;
				Areas->RootArea = Areas;
				Areas->bIsRoot = true;
			}
			else {
				Areas->RootArea = RootArea;
				Areas->bIsRoot = false;
			}

			FTransform FinalTransform = Areas->GetSpawnTransform();
			Areas->FinishSpawning(FinalTransform);
			Areas->ApplyAreaSizeFromGrid();
			RootArea->AllArea.Add(Areas);

		}
	}

}

UBoxComponent* AObjects::GetObjectInterActionCollider()
{
	return InterActionCollider;
}
UPrimitiveComponent* AObjects::GetObjectPhysicsCollider()
{
	return PhysicsCollider;
}

bool AObjects::IsOwnerActor(AActor* OtherActor)
{
	if (!OtherActor || !OwnPlayer) return false;
	return OtherActor == OwnPlayer || OtherActor == OwnPlayer->NowWeapon || OtherActor == OwnPlayer->NowObjects || OtherActor == OwnPlayer->NowSupport;
}

void AObjects::ChangeToNormalType(const FHitResult& Hit)
{
	if (!HasAuthority()) return;

	bTickMoveActive = false;
	TickMoveElapsedTime = 0.f;
	TickMoveInitialVelocity = FVector::ZeroVector;
	TickMoveGravity = FVector::ZeroVector;
	MoveDistance = 0.f;

	bHavingHitPoint = true;
	HitPoint = Hit.ImpactPoint;

	GetWorldTimerManager().ClearTimer(EnableThrowCollisionTimerhandle);

	Type = EObjectsType::Normal;

	// Equip 던지기 데미지 상태는 정리.
	// OwnPlayer는 Bomb 폭발 시 공격자 판정에 필요할 수 있으므로 유지.
	bHaveThrowDamage = false;
	ThrowDamage = 0.f;

	ApplyNormalState();

	Func_BecomeNormalType(Hit);

	ForceNetUpdate();
}

void AObjects::OnRep_OwnPlayer()
{
	ApplyCurrentState();
}

//각 물체의 고유 기능(자식 클래스가 상속받아 구현)
void AObjects::Func_Spawn_Implementation() {}
void AObjects::Func_Persist_Implementation(float DeltaTime) {}
void AObjects::Func_Destroy_Implementation() {}
void AObjects::Func_ZeroLife_Implementation() {}
void AObjects::Func_BecomeNormalType_Implementation(const FHitResult& Hit) {};
void AObjects::Func_HitPlayer_Implementation(APlayer_Character* Player) {}
void AObjects::Func_Equip_Implementation(APlayer_Character* Player) {}
void AObjects::Func_UnEquip_Implementation(APlayer_Character* Player) {}
void AObjects::Func_Interaction_Implementation(APlayer_Character* Player) {}
void AObjects::Func_Throw_Implementation(APlayer_Character* Player) {}
void AObjects::Func_AttackedByPlayer_Implementation(APlayer_Character* AttackPlayer) {}

//물체 물리 추가 설정 (상속 받는 클래스에서 지정)
void AObjects::ApplyAdditionalSetting() {}

