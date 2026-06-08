// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Player_Character.h"
#include "ItemDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"

AItem::AItem() {
	bReplicates = true;
	SetReplicateMovement(true);
	EquipmentType = EEquipmentType::Item;
	//아이템 물리 collider 생성 후 Root로 승격
	ItemCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("ItemPhysicsCollider"));
	PhysicsCollider = ItemCollider;
	SetRootComponent(ItemCollider);
	//Mesh Pivot 설정 후 MeshPivot과 PickupCollider을 ItemCollider 하위로 부착
	MeshPivot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshPivot"));
	MeshPivot->SetupAttachment(ItemCollider);
	PickupCollider->SetupAttachment(ItemCollider);
	Mesh->SetupAttachment(MeshPivot);
}

void AItem::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);
	if (MeshPivot) {
		MeshPivot->SetRelativeLocation(MeshLocation);
		MeshPivot->SetRelativeRotation(MeshRotation);
	}
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(MeshScale);
	}
	SetSizeofItemColliderwithMesh();
}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	if (ItemCollider) {
		//ItemCollider의 물리 설정
		SetPhysicsCollider(ItemCollider);
	}
	//회전 축 고정 (x, y축 기준 회전을 완전 제거)
	ItemCollider->BodyInstance.bLockXRotation = true;
	ItemCollider->BodyInstance.bLockYRotation = true;
	//아이템 회전 초기값 (Z축 기준으로만 회전시키고 나머지는 회전 X)
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//아이템 회전 설정 (Z축 기준으로만 회전시키고 나머지는 회전 X)
	if (!bIsEquipped && MeshPivot) {
		float Mesh_Yaw = GetActorRotation().Yaw;
		SetActorRotation(FRotator(0.f, Mesh_Yaw, 0.f));
		MeshPivot->AddLocalRotation(FRotator(0.f, SpinSpeed * DeltaTime, 0.f));
	}
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

void AItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AItem, NowUseCount);
}

bool AItem::PickedByPlayer(APlayer_Character* Player) {
	if (!HasAuthority()) return false;
	if (!Player) return false;
	//플레이어가 아이템을 주웠다면 플레이어에서 아이템 획득 함수 실행
	return Player->PickItem(this);
}

//아이템 장착
void AItem::Equip(APlayer_Character* Player) {
	if (!HasAuthority()) return;
	//장착 플레이어 지정
	if (!Player) return;
	//아이템를 장착된 상태로 변경
	SetEquipState(Player);
	if (ItemData) {
		//최초 생성된 아이템의 사용 횟수 초기화
		if (NowUseCount == -2) NowUseCount = ItemData->MaxUseCount;
		//아이템 고유 효과 획득
		EquipEffect(Player);
	}
}

//아이템 사용 효과 발동 처리
bool AItem::ApplyUseEffect() {
	if (!HasAuthority()) return false;
	return UseEffect(EquippedPlayer);
}

//아이템 해제
void AItem::UnEquip(APlayer_Character* Player) {
	if (!HasAuthority()) return;
	if (ItemCollider && EquippedPlayer) {
		ItemCollider->IgnoreActorWhenMoving(EquippedPlayer, false);
		EquippedPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
	}
	SetWorldState();
	//장착 해제 시 LifeTime이 10초 미만이면 10초로 연장
	if (LifeTime < 10.f) {
		LifeTime = 10.f;
	}
	//던졌을 때 아이템의 회전을 초기값으로 변경 (Z축 기준으로만 회전시키고 나머지는 회전 X)
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	bIsEquipped = false;
}

//아이템 사용
void AItem::UseItem() {
	if (!HasAuthority()) return;
	if (!ItemData) return;
	//사용 횟수 차감
	if (NowUseCount > -1) {
		if (ApplyUseEffect()) {
			NowUseCount -= 1;
		}
	}
	if (NowUseCount <= 0) {
		AllUseEffect(EquippedPlayer);
		Destroy();
	}
}

//아이템 사용횟수 검사
bool AItem::CheckUseCounting() {
	if (!ItemData) return false;
	if (NowUseCount <= 0) return false;
	return true;

}

//아이템 이름 반환 (다른 cpp 파일에서 이름 사용시)
FName AItem::ItemName() {
	return this->ItemData->ItemName;
}

UBoxComponent* AItem::GetItemCollider()
{
	return ItemCollider;
}

void AItem::ApplyEquipState()
{
	Super::ApplyEquipState();

	//아이템 매쉬와 물리 설정 변경
	if (Mesh) {
		Mesh->SetHiddenInGame(true);
	}
	if (ItemCollider) {
		SetActorLocation(ItemCollider->GetComponentLocation());
		SetActorRotation(FRotator(0.f, ItemCollider->GetComponentRotation().Yaw, 0.f));
		ItemCollider->SetSimulatePhysics(false);
		ItemCollider->SetEnableGravity(false);
		ItemCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	if (MeshPivot) {
		MeshPivot->SetRelativeLocation(MeshLocation);
		MeshPivot->SetRelativeRotation(MeshRotation);
	}
	if (!EquippedPlayer) {
		UE_LOG(LogTemp, Error, TEXT("No Detected EquippedPlayer"));
		return; 
	}
	//아이템을 슬롯에 장착
	AttachToComponent(EquippedPlayer->ItemSlot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void AItem::ApplyWorldState()
{
	Super::ApplyWorldState();

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	if (Mesh) {
		Mesh->SetHiddenInGame(false);
	}
	if (ItemCollider) {
		ItemCollider->SetSimulatePhysics(true);
		ItemCollider->SetEnableGravity(true);
		ItemCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemCollider->SetCollisionResponseToAllChannels(ECR_Block);
		ItemCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		ItemCollider->WakeAllRigidBodies();
	}
}

//Item Collider 크기를 계산
void AItem::SetSizeofItemColliderwithMesh()
{
	if (!Mesh || !Mesh->GetStaticMesh() || !ItemCollider) return;
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
	ItemCollider->SetBoxExtent(ColliderSize);
}
