// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "ObjectsDataAsset.h"
#include "ETC/ObjectsLaunchData.h"
#include "Player_Character.h"
#include "MapConstructor.h"
#include "WeaponDataAsset.h"
#include "WeaponStats.h"
#include "ETC/AttackPreviewGuide.h"
#include "Objects.h"
#include "Net/UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"

AWeapon::AWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	EquipmentType = EEquipmentType::Weapon;

	bReplicates = true;
	SetReplicateMovement(true);

	//무기 물리 collider 생성
	WeaponCollider = CreateDefaultSubobject<UPrimitiveComponent, UBoxComponent>(TEXT("WeaponPhysicsCollider"));
	WeaponCollider->SetNotifyRigidBodyCollision(true);
	WeaponCollider->SetAllUseCCD(true);
	WeaponCollider->OnComponentHit.AddDynamic(this, &AWeapon::OnWeaponHit);
	PhysicsCollider = WeaponCollider;
	SetRootComponent(WeaponCollider);
	//Mesh Pivot 설정 후 MeshPivot과 PickupCollider을 ItemCollider 하위로 부착
	MeshPivot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshPivot"));
	MeshPivot->SetupAttachment(WeaponCollider);
	PickupCollider->SetupAttachment(WeaponCollider);
	Mesh->SetupAttachment(MeshPivot);
}

void AWeapon::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);
	if (MeshPivot) {
		MeshPivot->SetRelativeLocation(MeshLocation);
		MeshPivot->SetRelativeRotation(MeshRotation);
	}
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(MeshScale);
	}
	if (WeaponCollider) {
		if (UBoxComponent* Box = Cast<UBoxComponent>(WeaponCollider)) {
			SetSizeofWeaponColliderwithMesh(Box);
		}
		else if (USphereComponent* Sphere = Cast<USphereComponent>(WeaponCollider)) {
			SetSizeofSphereColliderWithMesh(Sphere);
		}
		else if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(WeaponCollider)) {
			SetSizeofCapsuleColliderWithMesh(Capsule);
		}
	}

}

void AWeapon::BeginPlay() {
	Super::BeginPlay();
	//WeaponCollider의 물리 설정
	if (WeaponCollider) {
		SetPhysicsCollider(WeaponCollider);
	}
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, NowUseCount);
	DOREPLIFETIME(AWeapon, bHaveThrowDamage);
	DOREPLIFETIME(AWeapon, ThrowDamage);
}

void AWeapon::OnRep_NowUseCount()
{
	OnWeaponUseCountChanged.Broadcast();
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//필드에 일정 시간 이상 방치되면 제거
	if (!bIsEquipped && HasAuthority()) {
		CurrentTick += DeltaTime;
		if (CurrentTick > TickInterval) {
			CurrentTick = 0.f;
			LifeTime -= 1.f;
			LifeTime = FMath::Max(LifeTime, 0.f);
			if (LifeTime <= 0.f) {
				Destroy();
				return;
			}
		}
	}
}

bool AWeapon::PickedByPlayer(APlayer_Character* player) {
	if (!HasAuthority()) return false;
	if (!player) return false;
	//플레이어가 무기를 주웠다면 플레이어에서 무기 획득 함수 실행
	return player->PickWeapon(this);
}

//무기가 던져졌을 때 히트 판정
void AWeapon::OnWeaponHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return;
	if (!bHaveThrowDamage) return;
	if (!OtherActor || OtherActor == this) return;
	if (OtherActor == EquippedPlayer) return;

	APlayer_Character* HitPlayer = Cast<APlayer_Character>(OtherActor);
	AObjects* HitObject = Cast<AObjects>(OtherActor);

	if (HitPlayer) {
		HitPlayer->ApplyDamageInternal(ThrowDamage, EquippedPlayer, this, true, false);
	}
	else if (HitObject) {
		HitObject->ApplyDamageInternal(ThrowDamage, EquippedPlayer, this, true, false);
	}

	Destroy();
}

float AWeapon::OnPreHit(APlayer_Character* TargetPlayer, bool& bSkipRotation)
{
	return 1.f;
}

//해당 무기의 스탯을 획득
FWeaponStats* AWeapon::GetWeaponStats() {
	return WeaponData ? &WeaponData->Stats : nullptr;
}

//플레이어의 정보를 무기 정보로 변경 (Stat과 무관)
void AWeapon::GetWeaponInfo(APlayer_Character* EPlayer)
{
	if (!HasAuthority()) return;
	if (!EPlayer) return;
	EPlayer->Weight += WeaponData ? WeaponData->Weight : 0;
	EPlayer->Aim_TurnSpeed += WeaponData ? WeaponData->Aim_TurnSpeed : 0;
}

//무기 장착
void AWeapon::Equip(APlayer_Character* Player) {
	if (!HasAuthority()) return;
	//장착 플레이어 지정
	if (!Player) return;
	//무기를 장착된 상태로 변경, 무기 매쉬의 물리 설정 변경
	SetEquipState(Player);
	EquippedPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
	if (WeaponData) {
		//최초 생성된 무기의 사용 횟수 초기화
		if (NowUseCount == -2) NowUseCount = WeaponData->MaxUseCount;
		//무기 장착 효과 발동
		if (NowUseCount != 0) {
			EquipEffect(Player);
		}
	}
}

//무기 해제
void AWeapon::UnEquip(APlayer_Character* Player) {
	if (!HasAuthority()) return;
	AdditionalUnEquipWeaponFunction();
	//무기 해제 효과 발동
	if (NowUseCount != 0) {
		UnEquipEffect(EquippedPlayer);
	}
	if (WeaponCollider && EquippedPlayer) {
		WeaponCollider->IgnoreActorWhenMoving(EquippedPlayer, false);
		EquippedPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
	}
	SetWorldState();
	//장착 해제 시 LifeTime이 10초 미만이면 10초로 연장
	if (LifeTime < 10.f) {
		LifeTime = 10.f;
	}
	bIsEquipped = false;
}


//무기 던지기 시작 상태 적용
void AWeapon::BeginWeaponThrow(APlayer_Character* Player, float damage)
{
	if (!HasAuthority()) return;
	PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel6);
	if (!EquippedPlayer) {
		EquippedPlayer = Player;
	}
	ThrowDamage = damage;
	bHaveThrowDamage = true;

}
//무기 던지기 상태 비활성화
void AWeapon::EndWeaponThrow() {
	if (!HasAuthority()) return;
	PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel5);
	EquippedPlayer = nullptr;
	ThrowDamage = 0.f;
	bHaveThrowDamage = false;
}

//무기 적중 효과 발동 처리
void AWeapon::ApplyHitEffect(AActor* Target) {
	if (!HasAuthority()) return;
	HitEffect(EquippedPlayer, Target);
}

//무기 사용 효과 발동 처리
bool AWeapon::ApplyUseEffect() {
	return UseEffect(EquippedPlayer);
}

//무기 사용횟수 검사
bool AWeapon::CheckUseCounting() {
	if (!WeaponData) return false;
	if (NowUseCount <= 0) return false;
	return true;
}

//무기 사용
void AWeapon::UseWeapon() {
	if (!HasAuthority()) return;
	if (!WeaponData) return;
	//NowUseCount의 최소값을 -1로 지정 (-2는 초기화 전 최초 생성값)
	if (NowUseCount > -1) {
		NowUseCount -= 1;
		OnWeaponUseCountChanged.Broadcast();
		ForceNetUpdate();
	}
	//사용 횟수 차감
	if (NowUseCount == 0) {
		AllUseEffect(EquippedPlayer);
	}
	//원거리 무기를 제외한 무기는 UseCount가 0이 되면 즉시 파괴 (파괴 전 후처리 진행 후 파괴)
	if (NowUseCount <= 0 && (WeaponData->Stats.AttackType != EAttackType::Shoot && WeaponData->Stats.AttackType != EAttackType::Shoot_HS)) {
		if (EquippedPlayer) {
			EquippedPlayer->NowWeapon = NULL;
			EquippedPlayer->AStat = EquippedPlayer->GetWeaponStat();
			EquippedPlayer->Weight = EquippedPlayer->BaseStats.Default_Weight;
			EquippedPlayer->move_Speed = EquippedPlayer->BaseStats.Default_Speed;
			EquippedPlayer->Aim_TurnSpeed = 0;
			EquippedPlayer->NowWeapon = nullptr;

			EquippedPlayer->UpdateMoveSpeed();
			EquippedPlayer->OnWeaponChanged.Broadcast();
		}
		Destroy();
	}
}

//무기 발사
void AWeapon::ShootorThrow(APlayer_Character* Player, FVector TargetPoint) {
	if (!HasAuthority()) return;
	if (!Player || !WeaponData || !CheckUseCounting()) return;
	//투척/발사 물체가 있는 경우에만 함수 기능 적용 (Melee와 HitScan은 없음)
	if (!WeaponData->Bullet) return;
	
	FObjectLaunchData LaunchData;

	if (!BuildBulletLaunchData(Player, TargetPoint, LaunchData)) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Player;
	SpawnParams.Instigator = Player;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	//무기가 ShootType이면 Bullet 방향을 플레이어 발사 방향에 맞추기 (ThrowType이면 그냥 랜덤 방향으로 맞춤)
	FVector SpawnDir = LaunchData.LaunchVelocity;
	SpawnDir.Z = 0;

	if (SpawnDir.IsNearlyZero()) {
		SpawnDir = LaunchData.TargetLocation - LaunchData.StartLocation;
		SpawnDir.Z = 0.f;
	}
	if (SpawnDir.IsNearlyZero()) {
		SpawnDir = Player->GetActorForwardVector();
		SpawnDir.Z = 0.f;
	}

	SpawnDir = SpawnDir.GetSafeNormal();
	FRotator SpawnRotation = FRotator::ZeroRotator;
	if (WeaponData->Stats.AttackType == EAttackType::Shoot) {
		SpawnRotation = SpawnDir.Rotation();
		SpawnRotation += FRotator(0.f, 90.f, 0.f);
	}
	else if (WeaponData->Stats.AttackType == EAttackType::Throw) {
		SpawnRotation = FRotator(FMath::RandRange(-180.f, 180.f), FMath::RandRange(-180.f, 180.f), FMath::RandRange(-180.f, 180.f));
	}

	AObjects* Bullet = World->SpawnActor<AObjects>(WeaponData->Bullet, LaunchData.StartLocation, SpawnRotation, SpawnParams);
	if (!Bullet) return;

	
	//스폰된 물체와 스폰 무기 간의 충돌 무시
	if (Bullet->GetObjectPhysicsCollider()) {
		Bullet->GetObjectPhysicsCollider()->IgnoreActorWhenMoving(this, true);
	}
	//스폰된 물체와 소유 플레이어의 충돌 무시
	if (Player->GetCapsuleComponent()) {
		Player->GetCapsuleComponent()->IgnoreActorWhenMoving(Bullet, true);
	}

	Bullet->OwnPlayer = EquippedPlayer;
	//스폰된 물체 정보 초기화 및 상태 적용
	Bullet->ShootOrThrowWithLaunchData(Player, LaunchData.StartLocation, LaunchData.TargetLocation, LaunchData.LaunchVelocity, LaunchData.bUseGravity);
	//스폰된 물체의 투척 효과 발동
	Bullet->Func_Throw(Player);

	UseWeapon();
	ApplyUseEffect();
}

//무기 이름 반환 (다른 cpp 파일에서 이름 사용시)
FName AWeapon::WeaponName() {
	return this->WeaponData->Stats.AttackName;
}

//무기 물리 Collider 반환 (다른 cpp 파일에서 사용시)
UPrimitiveComponent* AWeapon::GetweaponCollider()
{
	return WeaponCollider;
}
//Weapon Collider 크기를 계산
void AWeapon::SetSizeofWeaponColliderwithMesh(UBoxComponent* Box)
{
	if (!Mesh || !Mesh->GetStaticMesh() || !Box) return;
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
	Box->SetBoxExtent(ColliderSize);
}

void AWeapon::SetSizeofSphereColliderWithMesh(USphereComponent* Sphere)
{
	if (!Mesh || !Mesh->GetStaticMesh() || !Sphere) return;

	FBoxSphereBounds MeshBounds = Mesh->GetStaticMesh()->GetBounds();
	FVector MeshSize = MeshPivot->GetRelativeScale3D().GetAbs();

	float MaxSize = MeshSize.GetMax();
	float Radius = MeshBounds.SphereRadius * MaxSize;

	Radius = Radius * SizeMagnification.X + ColliderOffset.X;
	Radius = FMath::Max(Radius, 0.01f);

	Sphere->SetSphereRadius(Radius);
}

void AWeapon::SetSizeofCapsuleColliderWithMesh(UCapsuleComponent* Capsule)
{
	if (!Mesh || !Mesh->GetStaticMesh() || !Capsule) return;

	FBoxSphereBounds MeshBounds = Mesh->GetStaticMesh()->GetBounds();
	FVector MeshSize = MeshPivot->GetRelativeScale3D().GetAbs();

	FVector BaseSize = MeshBounds.BoxExtent;
	BaseSize.X *= MeshSize.X;
	BaseSize.Y *= MeshSize.Y;
	BaseSize.Z *= MeshSize.Z;

	float Radius = FMath::Max(BaseSize.X, BaseSize.Y);
	float HalfHeight = BaseSize.Z;

	Radius = Radius * SizeMagnification.X + ColliderOffset.X;
	HalfHeight = HalfHeight * SizeMagnification.Z + ColliderOffset.Z;

	Radius = FMath::Max(Radius, 0.01f);
	HalfHeight = FMath::Max(HalfHeight, Radius);

	Capsule->SetCapsuleRadius(Radius);
	Capsule->SetCapsuleHalfHeight(HalfHeight);
}

void AWeapon::ApplyEquipState()
{
	Super::ApplyEquipState();

	if (WeaponCollider) {
		if (WeaponCollider) {
			WeaponCollider->SetSimulatePhysics(false);
			WeaponCollider->SetEnableGravity(false);
			WeaponCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			WeaponCollider->IgnoreActorWhenMoving(EquippedPlayer, true);
			WeaponCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
		if (MeshPivot) {
			MeshPivot->SetRelativeLocation(MeshLocation);
			MeshPivot->SetRelativeRotation(MeshRotation);
		}
		if (!EquippedPlayer) {
			UE_LOG(LogTemp, Error, TEXT("No Detected EquippedPlayer"));
			return;
		}
		AttachToComponent(EquippedPlayer->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("SK_PlayerHand"));

		if (!GripSocketName.IsNone() && Mesh && Mesh->DoesSocketExist(GripSocketName)) {
			FVector HandSocketWorld = EquippedPlayer->GetMesh()->GetSocketLocation(TEXT("SK_PlayerHand"));
			FVector GripSocketWorld = Mesh->GetSocketLocation(GripSocketName);
			FVector Offset = HandSocketWorld - GripSocketWorld;
			AddActorWorldOffset(Offset);
		}
	}
}

void AWeapon::ApplyWorldState()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Super::ApplyWorldState();
	if (WeaponCollider) {
		WeaponCollider->SetSimulatePhysics(true);
		WeaponCollider->SetEnableGravity(true);
		WeaponCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		WeaponCollider->WakeAllRigidBodies();
	}
}

FVector AWeapon::GetBulletSpawnLocation(APlayer_Character* Player)
{
	if (!Player)
	{
		return FVector::ZeroVector;
	}

	return Player->GetActorLocation() + Player->GetActorForwardVector() * 50.f + Player->GetActorRightVector() * 40.f;
}

bool AWeapon::BuildBulletLaunchData(APlayer_Character* Player, const FVector& TheTargetLocation, FObjectLaunchData& OutLaunchData)
{
	if (!Player || !WeaponData || !WeaponData->Bullet) return false;
	if (!Player->NowMap)return false;

	AObjects* Bullet = WeaponData->Bullet->GetDefaultObject<AObjects>();
	
	if (!Bullet || !Bullet->ObjectsData) return false;
	
	UObjectsDataAsset* BulletData = Bullet->ObjectsData;

	float AttackRange = Player->AStat.AttackRange * Player->NowMap->BlockSize;
	
	FVector StartLocation = GetBulletSpawnLocation(Player);
	FVector StandardLocation = Player->GetActorLocation();
	StandardLocation.Z = 0.f;
	FVector TargetLocation = ClampTargetByRange2D(StandardLocation, TheTargetLocation, AttackRange);

	OutLaunchData.StartLocation = StartLocation;
	OutLaunchData.TargetLocation = TargetLocation;
	OutLaunchData.AttackRange = AttackRange;

	if (BulletData->Type == EObjectsType::Throwable)
	{
		OutLaunchData.bUseGravity = true;
		OutLaunchData.LaunchVelocity = CalculateThrowLaunchVelocity(StartLocation, TargetLocation, 100.f);	
	}
	else if (BulletData->Type == EObjectsType::Projectile)
	{
		FVector Dir = TargetLocation - StartLocation;
		Dir.Z = 0.f;
		Dir = Dir.GetSafeNormal();

		if (Dir.IsNearlyZero())
		{
			return false;
		}

		OutLaunchData.bUseGravity = false;
		OutLaunchData.LaunchVelocity = Dir * BulletData->Move_Speed;
	}
	else
	{
		return false;
	}

	return !OutLaunchData.LaunchVelocity.IsNearlyZero();
}

FVector AWeapon::ClampTargetByRange2D(const FVector& TheStartLocation, const FVector& TheTargetLocation, float Range)
{
	FVector Result = TheTargetLocation;

	FVector Start2D = TheStartLocation;
	FVector Target2D = TheTargetLocation;

	Start2D.Z = 0.f;
	Target2D.Z = 0.f;

	FVector Dir2D = Target2D - Start2D;

	if (!Dir2D.IsNearlyZero() && Dir2D.Size() > Range)
	{
		Dir2D = Dir2D.GetSafeNormal() * Range;

		Result.X = TheStartLocation.X + Dir2D.X;
		Result.Y = TheStartLocation.Y + Dir2D.Y;
	}

	return Result;
}

FVector AWeapon::CalculateThrowLaunchVelocity(const FVector& TheStartLocation, const FVector& TheTargetLocation, float ArcHeight)
{
	const float GravityZ = FMath::Abs(GetWorld()->GetGravityZ());

	if (GravityZ <= KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	float MaxArcZ = FMath::Max(TheStartLocation.Z, TheTargetLocation.Z) + ArcHeight;

	if (MaxArcZ <= TheStartLocation.Z || MaxArcZ <= TheTargetLocation.Z)
	{
		return FVector::ZeroVector;
	}

	float UpHeight = MaxArcZ - TheStartLocation.Z;
	float TimeUp = FMath::Sqrt((2.f * UpHeight) / GravityZ);
	float DownHeight = MaxArcZ - TheTargetLocation.Z;
	float TimeDown = FMath::Sqrt((2.f * DownHeight) / GravityZ);
	float TotalTime = TimeUp + TimeDown;

	if (TotalTime <= KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	FVector LaunchVelocity = FVector::ZeroVector;
	LaunchVelocity.X = (TheTargetLocation.X - TheStartLocation.X) / TotalTime;
	LaunchVelocity.Y = (TheTargetLocation.Y - TheStartLocation.Y) / TotalTime;
	LaunchVelocity.Z = GravityZ * TimeUp;

	return LaunchVelocity;
}

FVector AWeapon::EvaluateLaunchLocation(FObjectLaunchData& LaunchData, float Time)
{
	FVector Gravity = FVector::ZeroVector;
	if (LaunchData.bUseGravity){
		if (UWorld* World = GetWorld()){
			Gravity = FVector(0.f, 0.f, World->GetGravityZ());
		}
	}

	return LaunchData.StartLocation + LaunchData.LaunchVelocity * Time + 0.5f * Gravity * Time * Time;	
}

float AWeapon::GetBulletMeshRadius(float TheRadius)
{
	if (!WeaponData || !WeaponData->Bullet) return TheRadius;

	AObjects* TheBullet = WeaponData->Bullet->GetDefaultObject<AObjects>();
	if (!TheBullet) return TheRadius;

	UStaticMeshComponent* MeshComp = TheBullet->FindComponentByClass<UStaticMeshComponent>();
	if (!MeshComp || !MeshComp->GetStaticMesh()) return TheRadius;

	FBoxSphereBounds Bounds = MeshComp->GetStaticMesh()->GetBounds();
	FVector Scale = MeshComp->GetRelativeScale3D().GetAbs();

	float x = Bounds.BoxExtent.X * Scale.X;
	float y = Bounds.BoxExtent.Y * Scale.Y;

	float Radius = FMath::Max(x, y);

	return FMath::Max(Radius, TheRadius);

}

bool AWeapon::BuildAimPreviewData(APlayer_Character* Player, FAimPreviewVisualData& PreviewData)
{
	PreviewData.Reset();

	if (!Player) return false;
	if (!WeaponData || !WeaponData->bUseAimPreview) return false;
	if (!Player->NowMap) return false;

	FWeaponStats& Stats = WeaponData->Stats;
	float BlockSize = Player->NowMap->BlockSize;

	float RangeBlock = WeaponData->bOverrideAimPreviewRange ? WeaponData->AimPreviewRangeOverride : Stats.AttackRange;
	float Degree = WeaponData->bOverrideAimPreviewDegree ? WeaponData->AimPreviewDegreeOverride : Stats.AttackDegree;

	PreviewData.bVisible = true;
	PreviewData.PreviewRange = RangeBlock * BlockSize;
	PreviewData.PreviewRadius = Stats.AttackRadius;
	PreviewData.HalfAngleDegree = Degree * 0.5f;
	PreviewData.PathRadius = WeaponData->ShootPathPreviewRadius;
	PreviewData.AttackDirection = WeaponData->AttackDirection;

	PreviewData.FillOpacity = WeaponData->AimFillOpacity;
	PreviewData.EdgeOpacity = WeaponData->AimEdgeOpacity;
	PreviewData.EdgeWidth = WeaponData->AimEdgeWidth;
	PreviewData.EdgeSoftness = WeaponData->AimEdgeSoftness;
	PreviewData.PatternLength = WeaponData->AimPatternLength;
	PreviewData.FlowSpeed = WeaponData->AimFlowSpeed;

	PreviewData.bOnlySameHeight = true;

	//남은 사용 횟수가 없으면 Preview 사용 X
	if (!CheckUseCounting()) {
		PreviewData.Reset();
		return false;
	}

	switch (Stats.AttackType) {
	case EAttackType::Melee:
	{
		if (PreviewData.AttackDirection == EWeaponAttackDirection::Vertical) {
			PreviewData.bShowAttackSector = false;
			PreviewData.bShowAttackPath = true;
			PreviewData.PathRadius = FMath::Max(Stats.AttackRadius, 1.f);
		}
		else{
			PreviewData.bShowAttackSector = true;
			PreviewData.bOnlySameHeight = true;
		}
		break;
	}
	case EAttackType::Shoot_HS:
	case EAttackType::Shoot:
	{
		PreviewData.bShowAttackSector = true;
		PreviewData.bShowAttackPath = true;
		PreviewData.bOnlySameHeight = true;
		PreviewData.PathRadius = GetBulletMeshRadius(WeaponData->ShootPathPreviewRadius);

		//PathRadius가 AttackDegree로 생긴 영역을 덮을 경우 Sector 비표시
		float HalfRad = FMath::DegreesToRadians(PreviewData.HalfAngleDegree);
		float DegreeHalfWidthAtMaxRange = PreviewData.PreviewRange * FMath::Sin(HalfRad);

		if (PreviewData.PathRadius >= DegreeHalfWidthAtMaxRange) {
			PreviewData.bShowAttackSector = false;
		}
		break;
	}
	case EAttackType::Throw:
		PreviewData.bShowAttackRangeCircle = true;
		PreviewData.bOnlySameHeight = false;
		PreviewData.bBlockByWall = false;
		break;
	default:
		PreviewData.Reset();
		return false;
	}

	return PreviewData.CheckUsingAnyVisual();
}

//자식 클래스의 AdditionalCollider 물리 설정을 WeaponCollider와 동일하게 설정
void AWeapon::CopyCollisionSetting(UPrimitiveComponent* AdditionalCollider)
{
	if (!AdditionalCollider || !WeaponCollider) return;

	AdditionalCollider->SetCollisionEnabled(WeaponCollider->GetCollisionEnabled());
	AdditionalCollider->SetCollisionObjectType(WeaponCollider->GetCollisionObjectType());
	AdditionalCollider->SetGenerateOverlapEvents(WeaponCollider->GetGenerateOverlapEvents());

	//AdditionalCollider의 채널 설정을 WeaponCollider과 동일하게 설정
	for (int32 ChannelIndex = 0; ChannelIndex < ECC_MAX; ++ChannelIndex) {
		const ECollisionChannel Channel = static_cast<ECollisionChannel>(ChannelIndex);

		AdditionalCollider->SetCollisionResponseToChannel(Channel, WeaponCollider->GetCollisionResponseToChannel(Channel));
	}

	AdditionalCollider->SetSimulatePhysics(false);
	AdditionalCollider->SetEnableGravity(false);

	AdditionalCollider->SetNotifyRigidBodyCollision(true);
	AdditionalCollider->SetAllUseCCD(true);

	//무기가 장착 상태이면 장착 중인 플레이어와는 충돌하지 않도록 설정
	if (EquippedPlayer) {
		bool bIgnorePlayer = WeaponCollider->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
		AdditionalCollider->IgnoreActorWhenMoving(EquippedPlayer, bIgnorePlayer);
	}
}

bool AWeapon::CheckCustomAdditionalAnimation(EFunctionInterActionReason Reason, FEquipmentActionAnimation& OutAnimation)
{
	if (!WeaponData) return false;
	FEquipmentActionAnimation* FoundAnimation = WeaponData->AdditionalAnimation.Find(Reason);

	if (!FoundAnimation || !FoundAnimation->IsValid()) return false;

	OutAnimation = *FoundAnimation;

	return true;
}

//무기가 가진 자체 공격 전처리 메커니즘 함수
bool AWeapon::BeforeAttackWeaponFunction()
{
	UE_LOG(LogTemp, Warning, TEXT("weapon before attack Function!"));
	return true;
}

bool AWeapon::InteractionWeaponFunction(EFunctionInterActionReason Reason)
{
	UE_LOG(LogTemp, Warning, TEXT("weapon interaction Function!"));
	return true;
}

void AWeapon::ReleaseAttackWeaponFunction()
{
	if (!HasAuthority()) return;
	if (!EquippedPlayer) return;

	UE_LOG(LogTemp, Warning, TEXT("Attack Released with weapon own Function!"));

	if (EquippedPlayer->bNowHoldingAttack) {
		EquippedPlayer->bNowHoldingAttack = false;
		EquippedPlayer->LastAttackTime = GetWorld()->GetTimeSeconds();
	}
}

void AWeapon::AdditionalUnEquipWeaponFunction()
{
	UE_LOG(LogTemp, Warning, TEXT("Weapon UnEquip Function!"));
}
