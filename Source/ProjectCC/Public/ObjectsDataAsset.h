// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Engine/DataAsset.h"
#include "Animation/EquipmentAnimation.h"
#include "Player_FunctionInterActionReason.h"
#include "ObjectsDataAsset.generated.h"

/**
 * 
 */

enum class EFunctionInterActionReason : uint8;

UENUM(BlueprintType)
enum class EBulletObjectsHitAction : uint8
{
	Destroy       UMETA(DisplayName = "Destroy On Hit"),
	BecomeNormal  UMETA(DisplayName = "Become Normal On Hit")
};

UCLASS(BlueprintType)
class PROJECTCC_API UObjectsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	//물체 타입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EObjectsType Type;
	
	//물체가 받는 데미지 타입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EObjectDamageType DamageType;

	//물체가 받는 데미지 배율(데미지 타입 : 배수)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DamageManification = 1.f;

	//체력 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bUseHP = true;
	
	//물체의 체력(0이 되면 파괴)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DefaultHP = 50.f;
	
	//라이프타임 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bUseLifeTime = true;
	
	//최초 생성부터 소멸까지 걸리는 시간 (장착 중에는 닳지 않음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DefaultLifeTime = 20.f;
	
	//장착 가능 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bCanEquip = false;
	
	//움직임 가능 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bCanMove = false;

	//상호작용 가능 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bCanInteraction = false;

	//움직임 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Move_Speed = 0.f;

	//피격 대상 넉백
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float KnockBackStrength = 750.f;

	//공격 대상 넉백 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KnockBackScale = 1.f;

	//지속 효과 주기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FunctionInterval = 1.f;
	
	//상호작용 최소 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float InterActionInterval = 1.f;

	//물체가 소유자와의 충돌 무시 여부 (Throwable / Projectile)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bIgnoreOwnerCollision = false;

	//물체의 HP Widget 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bUseHPWidget = false;

	//물체 지속 효과 유무
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bHavePassiveFunction = false;

	//물체 무게 (0 ~ 2)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Weight = 1;

	//물체 아이콘 (UI)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> ObjectsIcon = nullptr;

	//물체 기본 애니메이션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Grip")
	TObjectPtr<UAnimSequence> ObjectsGripSequence;

	//투척/발사체 히트 로직
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object|Hit")
	EBulletObjectsHitAction HitAction = EBulletObjectsHitAction::Destroy;

	//물체 행동별 애니메이션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Throw")
	TMap<EFunctionInterActionReason, FEquipmentActionAnimation> AdditionalAnimation;
};

