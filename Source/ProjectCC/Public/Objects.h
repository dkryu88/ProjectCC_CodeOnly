// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player_FunctionInterActionReason.h"
#include "Animation/EquipmentAnimation.h"
#include "Objects.generated.h"

class AMapConstructor;
class AArea;

UENUM(BlueprintType)
enum class EObjectsType :uint8 {
	//일반
	Normal		UMETA(DisplayName = "Normal"),
	//설치
	Install		UMETA(DisplayName = "Installation"),
	//투척
	Throwable	UMETA(DisplayName = "Throwable"),
	//장착
	Equip		UMETA(DisplayName = "Equip"),
	//발사
	Projectile	UMETA(DisplayName = "Projectile"),
	//지원
	Support		UMETA(DisplayName = "Support")
};

UENUM(BlueprintType)
enum class EObjectDamageType : uint8 {
	//받은 피해량과 관련없이 고정 데미지로 받음
	Fix			UMETA(DisplayName = "FixDamage"),
	//받은 피해량을 온전히 데미지로 받음
	Full		UMETA(DisplayName = "FullDamage"),
	//받은 피해량을 배수로 받음
	Multiply	UMETA(DisplayName = "MultiplyDamage"),
};

class UStaticMeshComponent;
class UBoxComponent;
class USphereComponent;
class UPrimitiveComponent;
class UObjectsDataAsset;
class UObjectsFunction;
class APlayer_Character;
class AMatch_PlayerController;
class UEffectManagerComponent;

UCLASS()
class PROJECTCC_API AObjects : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AObjects(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void LifeSpanExpired() override;
	virtual float TakeDamage(float damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:
	UPROPERTY(ReplicatedUsing = OnRep_Type, EditAnywhere, BlueprintReadOnly)
	EObjectsType Type = EObjectsType::Normal;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EObjectDamageType DamageType = EObjectDamageType::Full;
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPrimitiveComponent> PhysicsCollider;
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* InterActionCollider;
	UPROPERTY()
	UEffectManagerComponent* EffectManagerComp;
	//물체 Mesh의 기준점
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USceneComponent* MeshPivot;
	//Mesh의 위치 기준
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector MeshLocation = FVector::ZeroVector;
	//Mesh의 회전 기준
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FRotator MeshRotation = FRotator::ZeroRotator;
	//Mesh의 크기 기준
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector MeshScale = FVector(1.f, 1.f, 1.f);
	//물체 물리 Collider 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector SizeMagnification = FVector(1.f, 1.f, 1.f);
	//물체 물리 Collider 크기 보정값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector ColliderOffset = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	float XOffset = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	float YOffset = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	float ZOffset = 0.f;

	//물체 HP UI
	UPROPERTY(VisibleAnywhere, Category = "UI")
	class UWidgetComponent* HPWidgetComp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UObjects_HPWidget> Objects_HPWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	float HPWidgetOffsetZ = 40.f;

	//물체의 체력
	UPROPERTY(ReplicatedUsing = OnRep_HP)
	float HP = 0.f;
	UFUNCTION()
	void OnRep_HP();
	//물체의 생존 시간
	UPROPERTY(Replicated)
	float LifeTime = 0.f;
	//물체의 장착 여부
	UPROPERTY(ReplicatedUsing = OnRep_IsEquipped)
	bool bIsEquipped = false;
	//물체가 장착/장착해제 되면 즉시 호출회든 함수(OnRep) (용도 : 서버 동기화)
	UFUNCTION()
	void OnRep_IsEquipped();
	UFUNCTION()
	void OnRep_Type();
	UPROPERTY(Replicated)
	bool bHaveThrowDamage = false;
	UPROPERTY(Replicated)
	float ThrowDamage = 0.f;
	//물체 기능 On Off (Normal, Install 타입)
	UPROPERTY(Replicated)
	bool bNowActivated = false;
	//물체의 지속 효과 적용 간격
	UPROPERTY(Replicated)
	float FunctionInterval = 1.f;
	//물체 소유 플레이어
	UPROPERTY(ReplicatedUsing = OnRep_OwnPlayer)
	TObjectPtr<APlayer_Character> OwnPlayer;
	//충돌 대상 플레이어
	UPROPERTY()
	TObjectPtr<APlayer_Character> LastHitPlayer;
	//물체 소유 플레이어 컨트롤러
	UPROPERTY(Replicated)
	TObjectPtr<AMatch_PlayerController> OwnPlayerController;
	//물체 지속 기능 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bHavePassiveFunction = false;

	//투사체/발사체 TickMovement 이동 관련 데이터
	UPROPERTY(Transient)
	bool bTickMoveActive = false;
	UPROPERTY(Transient)
	FVector TickMoveStartLocation = FVector::ZeroVector;
	UPROPERTY(Transient)
	FVector TickMoveInitialVelocity = FVector::ZeroVector;
	UPROPERTY(Transient)
	FVector TickMoveGravity = FVector::ZeroVector;
	UPROPERTY(Transient)
	float TickMoveElapsedTime = 0.f;
	
	UPROPERTY(Transient)
	bool bHavingHitPoint = false;
	UPROPERTY(Transient)
	FVector HitPoint = FVector::ZeroVector;

	//투사체 착지/충돌 처리
	UFUNCTION(BlueprintCallable)
	void OnPhysicsColliderHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	//투사체/발사체 시작 위치
	UPROPERTY(Replicated, BlueprintReadOnly)
	FVector StartLocation = FVector::ZeroVector;
	//투사체 도착 위치
	UPROPERTY(Replicated, BlueprintReadOnly)
	FVector TargetLocation = FVector::ZeroVector;
	//투사체/발사체 이동 방향
	UPROPERTY(Replicated, BlueprintReadOnly)
	FVector MoveDirection = FVector::ZeroVector;
	//투사체/발사체 최대 거리
	UPROPERTY(Replicated, BlueprintReadOnly)
	float AttackRange;
	//투사체/발사체 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Move_Speed = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float MoveDistance = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float PassedTime = 0.f;
	//투사체 포물선 이동 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThrowFlyTime = 1.f;
	//투사체 포물선 최대 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThrowMaxHeight = 50.f;
	//물체 기본 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UObjectsDataAsset> ObjectsData;
	//물체 데미지 (투척/발사 타입)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HitDamage = 0.f;

	UFUNCTION()
	virtual void OnRep_OwnPlayer();

public:
	//물체 UI 갱신
	void RefreshHPWidget();
	//물체 UI 위치 재설정
	void UpdateHPWidgetWorldTransform();
	//물체의 초기 물리 설정
	void SetPhysicsCollider();
	//각 타입별 물체 추가 물리 설정
	void ApplyCurrentState();
	//각 물체별 추가 물리 설정
	virtual void ApplyAdditionalSetting();
	//물체 중 OwnerPlayer에 의해 색이 바뀌는 머터리얼이 있는 경우 그 색을 Player의 PortraitId로 적용
	void ApplyPortraitIdColorToMesh();
	//초기 물체 데이터 설정
	void SetObjectsStat();
	//물체 Mesh 크기를 BoxCollider에 반영
	void SetSizeofBoxColliderwithMesh(UBoxComponent* Collider);
	//물체 Mesh 크기를 SphereCollider에 반영
	void SetSizeofSphereColliderwithMesh(USphereComponent* Collider);
	//클라이언트가 물체 데미지 적용
	float ApplyDamageInternal(float Damage, APlayer_Character* AttackPlayer, AActor* DamageCauser, bool bApplyKnockBack, bool bForceDamage);
	//물체 넉백 적용
	void ApplyKnockBack(FVector& AttackDir, float Strength, float UpStrength);
	//플레이어가 물체 획득 시 확인
	bool PickedByPlayer(APlayer_Character* Player);
	//플레이어가 획득한 물체 정보 획득
	void GetObjectInfo(APlayer_Character* Player);
	//물체 장착(Equip Type)
	void Equip(APlayer_Character* Player);
	//물체 장착(Support Type)
	void EquipSupport(APlayer_Character* Player);
	//물체 상태를 일반 상태로 변경
	void ApplyWorldState();
	//물체 장착 해제
	void UnEquip(APlayer_Character* Player);
	//물체 던지기 시 데미지 (Equip Type)
	void BeginObjectThrow(APlayer_Character* owner, float damage);
	void EndObjectThrow();
	//물체 상태를 장착 상태로 변경
	void ApplyEquipState();
	//물체 설치(Install Type)
	void ApplyInstallState();
	//물체 상호작용 상태
	void ApplyInteractionState(APlayer_Character* InterActionPlayer);
	//물체 발사(Projectile Type), 투척(Throwable Type) 상태
	void ApplyShootorThrowState();
	//물체 발사 시 충돌 무시/복구
	void EnableOwnerCollisionAgain();
	//대상 Actor가 소유자의 장착물인지 확인
	bool IsOwnerActor(AActor* OtherActor);
	//투척/발사체 TickMovement 이동 위치 갱신
	FVector UpdateTickMoveLocation(float Time);
	//투척/발사체 TickMovement 시작
	void StartTickMovement(const FVector& initialVelocity, bool bUseGravity);
	//투척/발사체 이동 경로 데이터 획득
	void ShootOrThrowWithLaunchData(APlayer_Character* UsePlayer, const FVector& TheStartLocation, const FVector& TheTargetLocation, const FVector& TheLaunchVelocity, bool bUseGravity);
	//물체가 소유자 혹은 소유자의 장착물과의 충돌을 무시하도록 설정
	void ApplyOwnerCollisionIgnore(bool bIgnore);
	//(Hit시) 대상이 소유자 혹은 소유자의 장착물인지 확인
	bool ShouldIgnoreOwnerCollisionActor(AActor* OtherActor);
	//물체 충돌 시 기능(Projectile/Throwable이 Tick으로 이동하여 OnHit가 발생하지 않으므로 이 함수로 직접 이동 필요)
	void HandleObjectsHit(const FHitResult& Hit);
	//물체 기본(Normal Type)
	void ApplyNormalState();
	//물체 지원 (Support Type)
	void ApplySupportState();
	//물체가 Hit한 Area가 생성될 위치 획득
	FVector GetAreaCenterLocation();
	//offsetx x offsety의 영역에 Area들 생성
	void SpawnArea(TSubclassOf<AArea> TheArea, const FVector& CenterLocation, AActor* AreaSpawner, APlayer_Character* AreaOwner, TArray<AArea*>* OutSpawnedArea, int32 offsetx, int32 offsety);
	//물체 상호작용 Collider 반환
	UBoxComponent* GetObjectInterActionCollider();
	//물체 물리 Collider 반환 (물리 계산시 사용)
	UPrimitiveComponent* GetObjectPhysicsCollider();

	//Bullet 물체를 Normal 타입으로 전환(Hit시)
	void ChangeToNormalType(const FHitResult& Hit);
public:
	AMapConstructor* NowMap;
	float TickInterval = 1.f;
	float CurrentTick = 0.f;
	float CurrentFTick = 0.f;
	float LastPassiveFunctionTime = -1.f;
	float LastInteractionTime = -1.f;

	FTimerHandle EnableThrowCollisionTimerhandle;

	//물리 Collider 초기화 상태
	bool bPhysicsColliderInitialized = false;
	//최초 생성 시 타입별 상태 적용 상태
	bool bRuntimeStateResolved = false;
	//현재 소유주의 Collision을 무시할지 여부
	bool bOwnerCollisionIgnored = false;
public:
	//생성 시 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_Spawn();
	//물체 지속 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_Persist(float DeltaTime);
	//파괴 시 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_Destroy();
	//소멸 시 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_ZeroLife();
	//장착 시 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_Equip(APlayer_Character* Player);
	//Bullet 물체가 Normal 타입으로 전환 시 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_BecomeNormalType(const FHitResult& Hit);
	//장착 해제 시 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_UnEquip(APlayer_Character* Player);
	//플레이어 적중 시 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_HitPlayer(APlayer_Character* Player);
	//상호작용 시 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_Interaction(APlayer_Character* Player);
	//투척 시 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_Throw(APlayer_Character* Player);
	//피격 시 작동 기능
	UFUNCTION(BlueprintNativeEvent)
	void Func_AttackedByPlayer(APlayer_Character* AttackPlayer);

};

//고유 기능 구현은 여기에 있는 기능 함수를 Override 하여 사용
//Ex : 사용 효과 : virtual void Func_Throw_Implementation(AObjects* Object ,APlayer_Character* Player) override;