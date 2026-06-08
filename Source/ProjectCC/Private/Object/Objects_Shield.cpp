// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_Shield.h"
#include "Player_Character.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AObjects_Shield::AObjects_Shield(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBoxComponent>(TEXT("PhysicsCollider")))
{
	XOffset = 50.f;
	YOffset = 0.f;
	ZOffset = 15.f;
}


void AObjects_Shield::ApplyAdditionalSetting()
{
	if (Type == EObjectsType::Support) {
		if (!bIsEquipped) return;
		if (!OwnPlayer) return;

		PhysicsCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
		PhysicsCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); //LineTrace 공격을 막아내기 위함
		PhysicsCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel6, ECR_Block); //Bullet
		PhysicsCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
}

