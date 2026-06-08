// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_WoodBox.generated.h"
/**
 * 
 */
UCLASS()
class PROJECTCC_API AObjects_WoodBox : public AObjects
{
	GENERATED_BODY()

public:
	AObjects_WoodBox(const FObjectInitializer& ObjectInitializer);
	
protected:
	UPROPERTY(EditAnywhere, Category="CollisionHitBox")
	class UBoxComponent* HitBox;

	virtual void Func_Destroy_Implementation() override;

private:
	void SpawnRandomGradeWeapon();
};
