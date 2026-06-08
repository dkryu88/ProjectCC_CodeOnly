// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_NGradeRandomWeaponBox.h"
#include "Player_Character.h"
#include "PlayMode_match.h"
#include "ItemDataAsset.h"
#include "Weapon.h"
#include "Kismet/GameplayStatics.h"

bool AItem_NGradeRandomWeaponBox::UseEffect_Implementation(APlayer_Character* Player) {
	Super::UseEffect_Implementation(Player);

	if (!HasAuthority() || !Player || !ItemData) return false;

	UWorld* World = GetWorld();
	APlayMode_Match* PMM = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
	if (!PMM) return false;
	
	// 아이템 데이터의 등급을 매치 등급으로 변환
	EGrade TargetGrade = MapItemGradeToMatchGrade(ItemData->ItemGrade);

	// 매치 모드에서 무기 리스트 가져오기
	const TArray<TSubclassOf<AWeapon>>& WeaponList = PMM->GetWeaponListByGrade(TargetGrade);

	if (WeaponList.Num() > 0) {
		int32 RandomIndex = FMath::RandRange(0, WeaponList.Num() - 1);
		TSubclassOf<AWeapon> SelectedWeaponClass = WeaponList[RandomIndex];

		if (SelectedWeaponClass) {
			FTransform SpawnTransform = Player->GetActorTransform();
			FVector ForwardLocation = Player->GetActorLocation() + (Player->GetActorForwardVector() * 50.f);
			SpawnTransform.SetLocation(ForwardLocation);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AWeapon* SpawnedWeapon = World->SpawnActor<AWeapon>(SelectedWeaponClass, SpawnTransform, SpawnParams);

			if (SpawnedWeapon) {
				float Strength = 0.f;
				FVector PlayerVelocity = Player->GetVelocity();
				PlayerVelocity.Z = 0.f;

				if (PlayerVelocity.SizeSquared() < FMath::Square(0.5f)) {
					Strength = Player->PutStrength;
				}
				else {
					Strength = Player->MoveStrength;
				}

				Player->ApplyThrow(SpawnedWeapon, Strength, 0.f, 0.5f);
				return true;
			}
		}
	}

	return false;
}

EGrade AItem_NGradeRandomWeaponBox::MapItemGradeToMatchGrade(EItemGrade InItemGrade)
{
	switch (InItemGrade)
	{
	case EItemGrade::S:
		return EGrade::Grade_S;

	case EItemGrade::A:
		return EGrade::Grade_A;

	case EItemGrade::B:
	default:
		return EGrade::Grade_B;
	}
}