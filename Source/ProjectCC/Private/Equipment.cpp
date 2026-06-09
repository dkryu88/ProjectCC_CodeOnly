// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "EffectManagerComponent.h"
#include "Components/BoxComponent.h"

// Sets default values
AEquipment::AEquipment(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	//자식 클래스의 매쉬 컴포넌트를 부모에서 미리 부착
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//Equipment의 물리 Collider 포인터 초기화 (Item, Weapon에서 생성)
	PhysicsCollider = nullptr;
	//Equipment의 플레이어의 장착 범위 감지 Collider
	PickupCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("PickupCollider"));
	PickupCollider->InitBoxExtent(FVector(50.f, 50.f, 50.f));
	PickupCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollider->SetCollisionObjectType(ECC_GameTraceChannel1);
	PickupCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	//ECC_GameTraceChannel1 <- 콜리전 프리셋 1번 (Interaction)
	PickupCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	PickupCollider->SetGenerateOverlapEvents(true);

	//이펙트 담당 컴포넌트
	EffectManagerComp = CreateDefaultSubobject<UEffectManagerComponent>(TEXT("EffectManager"));
}

// Called when the game starts or when spawned
void AEquipment::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AEquipment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEquipment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEquipment, bIsEquipped);
	DOREPLIFETIME(AEquipment, EquippedPlayer);
	DOREPLIFETIME(AEquipment, LifeTime);
}

bool AEquipment::PickedByPlayer(APlayer_Character* player) {
	return false;
}
//장착 상태 변경 시 서버에서 자동 호출
void AEquipment::OnRep_IsEquipped() {
	if (bIsEquipped) {
		ApplyEquipState();
	}
	else {
		ApplyWorldState();
	}
}

//플레이어가 장착 해제시 상태 설정
void AEquipment::SetWorldState() {
	if (!HasAuthority()) return;
	EquippedPlayer = nullptr;
	bIsEquipped = false;
	ApplyWorldState();
}

void AEquipment::ApplyWorldState()
{
	if (PickupCollider)
	{
		PickupCollider->SetGenerateOverlapEvents(true);
		PickupCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

//플레이어가 장착 시 상태 설정
void AEquipment::SetEquipState(APlayer_Character* Player) {
	if (!HasAuthority()) return;
	bIsEquipped = true;
	EquippedPlayer = Player;
	ApplyEquipState();
}

void AEquipment::ApplyEquipState()
{
	if (PickupCollider)
	{
		PickupCollider->SetGenerateOverlapEvents(false);
		PickupCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AEquipment::SetPhysicsCollider(UPrimitiveComponent* Collider)
{
	if (!Collider) return;
	PhysicsCollider = Collider;
	PhysicsCollider->SetUsingAbsoluteLocation(false);
	PhysicsCollider->SetUsingAbsoluteRotation(false);
	PhysicsCollider->SetMobility(EComponentMobility::Movable);
	PhysicsCollider->SetEnableGravity(true);
	PhysicsCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsCollider->SetCollisionObjectType(ECC_GameTraceChannel5);
	PhysicsCollider->SetCollisionProfileName(TEXT("EquipmentCollision"));
	PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Block);
	PhysicsCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	PhysicsCollider->SetSimulatePhysics(true);
	PhysicsCollider->SetMassOverrideInKg(NAME_None, 50.f, true);
}

//Equipment 각각의 고유 효과 함수 (자식 Class가 Override하여 사용)
//Equipment 장착 효과 (무기/아이템)
void AEquipment::EquipEffect_Implementation(APlayer_Character* Player) {
	UE_LOG(LogTemp, Warning, TEXT("Equipped"));
}
//Equipment 해제 효과 (무기)
void AEquipment::UnEquipEffect_Implementation(APlayer_Character* Player) {
	UE_LOG(LogTemp, Warning, TEXT("UnEquipped"));
}
//Equipment 적중 효과 (무기)
void AEquipment::HitEffect_Implementation(APlayer_Character* Player, AActor* Target) {
	UE_LOG(LogTemp, Warning, TEXT("Attacked"));
}

//Equipment 사용 효과 (공격 적중시 공격 적중 효과가 있다면 적중 효과만 발동) (무기/아이템)
bool AEquipment::UseEffect_Implementation(APlayer_Character* Player) {
	UE_LOG(LogTemp, Warning, TEXT("Used"));
	return true;
}

//Equipment 사용 횟수 소진 효과 (장착 해제 효과와 별도) (무기/아이템(사용 효과와 별도))
void AEquipment::AllUseEffect_Implementation(APlayer_Character* Player) {
	UE_LOG(LogTemp, Warning, TEXT("AllUsed"));
}