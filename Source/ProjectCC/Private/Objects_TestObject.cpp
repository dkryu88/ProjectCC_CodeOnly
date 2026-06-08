// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects_TestObject.h"
#include "Player_Character.h"
#include "Components/BoxComponent.h"

AObjects_TestObject::AObjects_TestObject(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer.SetDefaultSubobjectClass<UBoxComponent>(TEXT("PhysicsCollider"))) {
}

void AObjects_TestObject::Func_Persist_Implementation(float DeltaTime)
{
	UE_LOG(LogTemp, Warning, TEXT("[Test]TickFunction"));
}

void AObjects_TestObject::Func_Spawn_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[Test]SpawnFunction"));
}

void AObjects_TestObject::Func_Destroy_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[Test]DestroyFunction"));
}

void AObjects_TestObject::Func_ZeroLife_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[Test]ZeroLifeFunction"));
}

void AObjects_TestObject::Func_Equip_Implementation(APlayer_Character* Player)
{
	UE_LOG(LogTemp, Warning, TEXT("[Test]EquipFunction Player: %s"), Player? *Player->GetName() : TEXT("NONE"));
}

void AObjects_TestObject::Func_UnEquip_Implementation(APlayer_Character* Player)
{
	UE_LOG(LogTemp, Warning, TEXT("[Test]UnEquipFunction Player: %s"), Player ? *Player->GetName() : TEXT("NONE"));
}

void AObjects_TestObject::Func_Interaction_Implementation(APlayer_Character* Player)
{
	UE_LOG(LogTemp, Warning, TEXT("[Test]InterActionFunction Player: %s"), Player ? *Player->GetName() : TEXT("NONE"));
}

void AObjects_TestObject::Func_Throw_Implementation(APlayer_Character* Player)
{
	UE_LOG(LogTemp, Warning, TEXT("[Test]ThrowFunction Player: %s"), Player ? *Player->GetName() : TEXT("NONE"));
}

void AObjects_TestObject::Func_AttackedByPlayer_Implementation(APlayer_Character* AttackPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("[Test]AttackedFunction Player: %s"), AttackPlayer ? *AttackPlayer->GetName() : TEXT("NONE"));
}

