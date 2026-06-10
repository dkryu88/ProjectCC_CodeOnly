// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Equipment.generated.h"

class UStataicMeshComponent;
class UBoxComponent;
class APlayer_Character;
class AObjects;
class UPrimitiveComponent;
class UEffectManagerComponent;

UENUM(BlueprintType)
enum class EEquipmentType : uint8 {
	Item,
	Weapon
};

UENUM(BlueprintType)
enum class ECollisionType : uint8 {
	E_Pushable,
	E_WalkThrough
};

UCLASS()
class PROJECTCC_API AEquipment : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEquipment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	EEquipmentType GetEquipmentType() { return EquipmentType; }
	UBoxComponent* GetPickupCollider() { return PickupCollider; }
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(VisibleAnywhere, Category = "Equipment")
	TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere, Category="Equipment")
	UPrimitiveComponent* PhysicsCollider;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	TObjectPtr<UBoxComponent> PickupCollider;
	UPROPERTY()
	UEffectManagerComponent* EffectManagerComp;
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	EEquipmentType EquipmentType = EEquipmentType::Weapon;
	//이미 장착 중인 Equipment인지 확인 (서버에서 관리)
	UPROPERTY(ReplicatedUsing = OnRep_IsEquipped)
	bool bIsEquipped = false;
	//Equipment의 맵에 존재할 수 있는 시간
	UPROPERTY(Replicated)
	float LifeTime = 3000.f;
	//Equipment가 장착/장착해제 되면 즉시 호출회든 함수(OnRep)
	UFUNCTION()
	virtual void OnRep_IsEquipped();
	//장착 중인 플레이어 데이터
	UPROPERTY(Replicated)
	TObjectPtr<APlayer_Character> EquippedPlayer;
	//플레이어가 Equipment 획득시 검사
	virtual bool PickedByPlayer(APlayer_Character* Player);
	//플레이어가 장착 시 설정
	void SetWorldState();
	virtual void ApplyWorldState();
	//플레이어가 해제 시 설정
	void SetEquipState(APlayer_Character* Player);
	virtual void ApplyEquipState();
	//Equipment의 초기 물리 설정
	void SetPhysicsCollider(UPrimitiveComponent* Collider);
	//다른 곳에서 물리 Collider 접근용
	UPrimitiveComponent* GetPhysicsCollider() { return PhysicsCollider; }
	//다른 곳에서 매쉬 접근용
	UStaticMeshComponent* GetMesh() { return Mesh; }

public:
	float TickInterval = 1.f;
	float CurrentTick = 0.f;

public:
	//장착 효과
	UFUNCTION(BlueprintNativeEvent)
	void EquipEffect(APlayer_Character* Player);
	//장착 해제 효과
	UFUNCTION(BlueprintNativeEvent)
	void UnEquipEffect(APlayer_Character* Player);
	//공격 적중 효과 (Cast로 APlayer_Character인지 AObjects인지 판별 후 사용)
	UFUNCTION(BlueprintNativeEvent)
	void HitEffect(APlayer_Character* Player, AActor* Target);
	//단순 공격 효과 (비적중시에도 발동)
	UFUNCTION(BlueprintNativeEvent)
	bool UseEffect(APlayer_Character* Player);
	//사용 횟수 소진 효과 (장착 해제랑 별도)
	UFUNCTION(BlueprintNativeEvent)
	void AllUseEffect(APlayer_Character* Player);
};
