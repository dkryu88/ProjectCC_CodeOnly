// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment.h"
#include "WeaponDataAsset.h"
#include "ETC/ObjectsLaunchData.h"
#include "Player_FunctionInterActionReason.h"
#include "ETC/AttackPreviewGuide.h"
#include "Weapon.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnWeaponUseCountChanged);

class UWeaponDataAsset;
class UWeaponEffect;
class UBoxComponent;
class UCapsuleComponent;
class UPrimitiveComponent;
struct FWeaponStats;
enum class EFunctionInterActionReason : uint8;

UCLASS()
class PROJECTCC_API AWeapon : public AEquipment
{
	GENERATED_BODY()
	
public:
	AWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual bool PickedByPlayer(APlayer_Character* player) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	//UI Delegate 바인드
	FOnWeaponUseCountChanged OnWeaponUseCountChanged;
public:
	//무기에 속하는 객체들의 클래스 포인터
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeapon> Weapon;
	//각 무기 데이터
	UPROPERTY(EditDefaultsOnly, Category = "WeaponData")
	TObjectPtr<UWeaponDataAsset> WeaponData;
	//남은 사용 횟수 (서버에서 관리)
	UPROPERTY(ReplicatedUsing = OnRep_NowUseCount)
	int32 NowUseCount = -2;
	UFUNCTION()
	void OnRep_NowUseCount();
	//무기 물리 Collider
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UPrimitiveComponent> WeaponCollider;
	//무기 Mesh의 손잡이 Socket 이름
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	FName GripSocketName = FName("SK_WeaponGripPoint");
	//무기 장착 해제 시 복원 Pivot 위치/회전
	UPROPERTY()
	FVector DefaultMeshPivotLocation = FVector::ZeroVector;
	UPROPERTY()
	FRotator DefaultMeshPivotRotation = FRotator::ZeroRotator;
	//무기 물리 Collider 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FVector SizeMagnification = FVector(1.f, 1.f, 1.f);
	//무기 물리 Collider 크기 보정값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FVector ColliderOffset = FVector(0.f, 0.f, 0.f);
	/*--WeaponCollider과 독립적으로 수정하기 위함--*/
	//아이템 Mesh의 기준점
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
	/*--------------------------------------------*/
	UPROPERTY(Replicated)
	bool bHaveThrowDamage = false;
	UPROPERTY(Replicated)
	float ThrowDamage = 0.f;
	UFUNCTION()
	void OnWeaponHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	FTimerHandle ThrowDamageClearTimerHandle;
public:
	//현재 무기를 세팅
	void SetWeapon(TSubclassOf<AWeapon> InClass) { Weapon = InClass; }
	//현재 무기를 반환
	TSubclassOf<AWeapon>GetWeapon() { return Weapon; }
	//현재 무기 타입을 반환
	EAttackType GetWeaponAttackType() { return WeaponData->Stats.AttackType; }
	//무기 스탯 획득
	FWeaponStats* GetWeaponStats();
	//무기 무게 획득
	void GetWeaponInfo(APlayer_Character* EPlayer);
	//무기 장착
	void Equip(APlayer_Character* Player);
	//무기 해제
	void UnEquip(APlayer_Character* Player);
	//무기 상태 정리 (무기 해제/파괴 시)
	virtual void AdditionalUnEquipWeaponFunction();
	//무기 던지기 상태 설정
	void BeginWeaponThrow(APlayer_Character* Player, float damage);
	void EndWeaponThrow();
	//무기 사용 효과
	bool ApplyUseEffect();
	//무기 적중 효과
	void ApplyHitEffect(AActor* TPlayer);
	//무기 사용 횟수 검사
	bool CheckUseCounting();
	//무기 사용
	void UseWeapon();
	//무기 발사
	void ShootorThrow(APlayer_Character* Player, FVector TargetPoint);
	//무기 이름 반환
	FName WeaponName();
	//무기 물리 Collider 반환 (물리 계산시 사용)
	UPrimitiveComponent* GetweaponCollider();
	//무기 무게 반환
	float GetWeaponWeight() { return WeaponData->Weight; }
	//무기 선딜레이 반환 (자식 클래스에서 값 재정의)
	float GetAttackEarlierDelay() { return WeaponData->Stats.AttackEarlierDelay; }
	//무기가 던져진 상태인지 확인
	bool CheckWeaponIsNowThrown() { return bHaveThrowDamage; }
	//무기 Mesh 크기를 Collider에 반영(기본 Box형태의 WeaponCollider의 경우
	void SetSizeofWeaponColliderwithMesh(UBoxComponent* Box);
	//무기 Mesh 크기를 Collider에 반영(Sphere형태의 WeaponCollider의 경우)
	void SetSizeofSphereColliderWithMesh(USphereComponent* Sphere);
	//무기 Mesh 크기를 Collider에 반영(Capsule형태의 WeaponCollider의 경우)
	void SetSizeofCapsuleColliderWithMesh(UCapsuleComponent* Capsule);
	//자식 클래스에 추가 Collider가 있을 경우 WeaponCollider의 설정을 복사
	void CopyCollisionSetting(UPrimitiveComponent* AdditionalCollider);
	//장착 상태 적용
	virtual void ApplyEquipState() override;
	//해제 상태 적용
	virtual void ApplyWorldState() override;

	//--투척 무기 투척 계산--//
	FVector GetBulletSpawnLocation(APlayer_Character* Player);
	bool BuildBulletLaunchData(APlayer_Character* Player, const FVector& TheTargetLocation, FObjectLaunchData& OutLaunchData);
	FVector ClampTargetByRange2D(const FVector& TheStartLocation, const FVector& TheTargetLocation, float Range);
	FVector CalculateThrowLaunchVelocity(const FVector& TheStartLocation, const FVector& TheTargetLocation, float ArcHeight = 100.f);
	FVector EvaluateLaunchLocation(FObjectLaunchData& LaunchData, float Time);
	//발사 무기 Bullet 크기 계산
	float GetBulletMeshRadius(float Radius);

	//무기 Preview 세팅 //Preview를 안쓰는 경우 (Override해서 Reset 후 return false) //이외에 WeaponData와 다르게 표기가 필요한 경우 Override해서 데이터 조절
	virtual bool BuildAimPreviewData(APlayer_Character* Player, FAimPreviewVisualData& PreviewData);

	/*무기별 애니메이션*/
	//무기의 특수 애니메이션 사용 여부 확인
	virtual bool CheckCustomAdditionalAnimation(EFunctionInterActionReason Reason, FEquipmentActionAnimation& OutAnimation);
	
	/*무기별 특수 공격 기능*/
	//무기로 타격 적용 직전 데미지 배율, 대상 플레이어 회전 여부를 결정 (자식 클래스에서 재설정)
	virtual float OnPreHit(APlayer_Character* TargetPlayer, bool& bSkipRotation);
	//공격 전처리 기능 (return 값은 일반 공격 함수를 실행시킬지 여부)
	virtual bool BeforeAttackWeaponFunction();
	//플레이어의 행동 별 실행 기능 (return 값은 입력 행동을 지속할지 여부)
	virtual bool InteractionWeaponFunction(EFunctionInterActionReason Reason);
	//공격 릴리즈 기능
	virtual void ReleaseAttackWeaponFunction();
};

//효과 구현은 Equipment에 있는 효과 함수를 Override 하여 사용
//Ex : 사용 효과 : virtual void UseEffect_Implementation(APlayer_Character* Player) override;