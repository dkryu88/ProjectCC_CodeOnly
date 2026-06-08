// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_WoodBox.h"
#include "PlayMode_Match.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon.h"

AObjects_WoodBox::AObjects_WoodBox(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer) {
	Type = EObjectsType::Equip;

	if (PhysicsCollider) {
		PhysicsCollider->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		PhysicsCollider->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	}
}

void AObjects_WoodBox::Func_Destroy_Implementation() {
	if (HasAuthority()) {
		SpawnRandomGradeWeapon();
	}
}

void AObjects_WoodBox::SpawnRandomGradeWeapon() {
	UWorld* World = GetWorld();
	if (!World) return;

	APlayMode_Match* PMM = Cast<APlayMode_Match>(World->GetAuthGameMode());	//서버 사이드 로직에서 더 안전하게 게임 모드를 가져오는 방법
	if (!PMM) return;

	// 확률 계산
	int32 RandomRoll = FMath::RandRange(1, 100);
	EGrade SelectedGrade;

	if (RandomRoll <= 50)		SelectedGrade = EGrade::Grade_B;	// 50%
	else if (RandomRoll <= 85)	SelectedGrade = EGrade::Grade_A;	// 35%
	else						SelectedGrade = EGrade::Grade_S;	// 15%

	const TArray<TSubclassOf<AWeapon>>& WeaponList = PMM->GetWeaponListByGrade(SelectedGrade);

	if (WeaponList.Num() <= 0) return;

	int32 RandomIndex = FMath::RandRange(0, WeaponList.Num() - 1);
	TSubclassOf<AWeapon> SelectedWeaponClass = WeaponList[RandomIndex];

	if (SelectedWeaponClass) {
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		World->SpawnActor<AWeapon>(SelectedWeaponClass, GetActorLocation() + FVector(0.f, 0.f, 50.f), GetActorRotation(), SpawnParams);
	}

}