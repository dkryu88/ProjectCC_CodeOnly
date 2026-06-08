// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Objects_FireBottle.h"
#include "Area.h"
#include "MapConstructor.h"

void AObjects_FireBottle::Func_Destroy_Implementation() {
	Super::Func_Destroy_Implementation();

	if (!HasAuthority()) return;
	
	UWorld* World = GetWorld();
	if (!World || !FireArea) return;

	SpawnArea(FireArea, GetAreaCenterLocation(), this, OwnPlayer, nullptr, 1, 1);
}


