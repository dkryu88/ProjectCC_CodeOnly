// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_GiftBox.h"
#include "PlayMode_Match.h"
#include "Kismet/GameplayStatics.h"
#include "Item.h"

void AObjects_GiftBox::Func_Destroy_Implementation() {
	if (HasAuthority()) {
		SpawnRandomGradeItem();
	}
}


void AObjects_GiftBox::SpawnRandomGradeItem() {
	UWorld* World = GetWorld();
	if (!World) return;
	//APlayMode_Match* PMM = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
	APlayMode_Match* PMM = Cast<APlayMode_Match>(World->GetAuthGameMode());	//서버 사이드 로직에서 더 안전하게 게임 모드를 가져오는 방법
	if (!PMM) return;

	// 확률 계산
	int32 RandomRoll = FMath::RandRange(1, 100);
	EGrade SelectedGrade;

	if (RandomRoll <= 50)	SelectedGrade = EGrade::Grade_B;	// 50%
	else if (RandomRoll <= 85)	SelectedGrade = EGrade::Grade_A;	// 35%
	else						SelectedGrade = EGrade::Grade_S;	// 15%

	const TArray<TSubclassOf<AItem>>& ItemList = PMM->GetItemListByGrade(SelectedGrade);

	if (ItemList.Num() <= 0)return;

	int32 RandomIndex = FMath::RandRange(0, ItemList.Num() - 1);
	TSubclassOf<AItem> SelectedItemClass = ItemList[RandomIndex];

	if (SelectedItemClass) {
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		World->SpawnActor<AItem>(SelectedItemClass, GetActorLocation() + FVector(0.f, 0.f, 50.f), GetActorRotation(), SpawnParams);
	}

}
