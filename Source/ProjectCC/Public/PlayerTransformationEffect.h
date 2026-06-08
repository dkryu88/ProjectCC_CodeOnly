// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PlayerVisualManagerComponent.h"
#include "PlayerTransformationEffect.generated.h"

/**
 * 
 */
class APlayer_Character;
struct FPlayerTransformation;
class UPlayerTransformationComponent;

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class PROJECTCC_API UPlayerTransformationEffect : public UObject
{
	GENERATED_BODY()
	
public:
	virtual void StartEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* usedPlayer);
	virtual void PersistEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, float DeltaTime);
	virtual void HittedEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* AttackedPlayer);
	virtual void EndEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData);

	//EndEffect를 포함한 변신 해제시 호출되는 모든 기능
	virtual void EndFunction(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, bool bUseEndEffect);
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual")
	FName VisualEffectName = TEXT("TransformationVisualDefault");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual")
	FVisualEffectRequest VisualData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual")
	bool bUseVisualManager = false;

protected:
	//아래 함수들은 VisualManager 사용시 호출//

	virtual bool BuildVisualEffectRequest(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* UsedPlayer, FVisualEffectRequest& OutRequest);
	
	FName ResolveVisualEffectName(const FPlayerTransformation& TransformationData);

	void AddVisualEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData, APlayer_Character* UsedPlayer);

	void RemoveVisualEffect(APlayer_Character* Player, UPlayerTransformationComponent* TransformationComp, FPlayerTransformation& TransformationData);
};
