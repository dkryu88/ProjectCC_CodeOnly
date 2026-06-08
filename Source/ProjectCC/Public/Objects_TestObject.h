// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_TestObject.generated.h"

/**
 * 테스트용 물체입니다 다 쓰셨으면 지워주세요~
 */

UCLASS()
class PROJECTCC_API AObjects_TestObject : public AObjects
{
	GENERATED_BODY()
public:
	AObjects_TestObject(const FObjectInitializer& ObjectInitializer);
public:
	virtual void Func_Persist_Implementation(float DeltaTime) override;
	//생성 시 작동 기능
	virtual void Func_Spawn_Implementation() override;
	//파괴 시 작동 기능
	virtual void Func_Destroy_Implementation() override;
	//소멸 시 작동 기능
	virtual void Func_ZeroLife_Implementation() override;
	//장착 시 작동 기능
	virtual void Func_Equip_Implementation(APlayer_Character* Player) override;
	//장착 해제 시 작동 기능
	virtual void Func_UnEquip_Implementation(APlayer_Character* Player) override;
	//상호작용 시 작동 기능
	virtual void Func_Interaction_Implementation(APlayer_Character* Player) override;
	//투척 시 작동 기능
	virtual void Func_Throw_Implementation(APlayer_Character* Player) override;
	//피격 시 작동 기능
	virtual void Func_AttackedByPlayer_Implementation(APlayer_Character* AttackPlayer) override;
};
