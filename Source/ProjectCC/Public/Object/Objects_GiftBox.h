// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects.h"
#include "Objects_GiftBox.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCC_API AObjects_GiftBox : public AObjects
{
	GENERATED_BODY()

protected:

	virtual void Func_Destroy_Implementation() override;


private:
	void SpawnRandomGradeItem();
};
