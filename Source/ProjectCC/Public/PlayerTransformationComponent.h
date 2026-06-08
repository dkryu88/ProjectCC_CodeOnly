// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerTransformations.h"
#include "PlayerTransformationComponent.generated.h"

class APlayer_Character;
class UStaticMeshComponent;
class UPlayerTransformationDataAsset;
class UMeshComponent;
class UMaterialInterface;

USTRUCT()
struct FTransformationMeshBackup {
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> MeshComponent = nullptr;

	UPROPERTY(Transient)
	TArray <TObjectPtr<UMaterialInterface>> OriginalMaterials;

	UPROPERTY(Transient)
	bool bOriginalVisible = true;

	UPROPERTY(Transient)
	bool bOriginalHideden = false;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTCC_API UPlayerTransformationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerTransformationComponent();
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	UFUNCTION(BlueprintCallable, Category="Transformation")
	bool StartTransformation(UPlayerTransformationDataAsset* TransformationData, APlayer_Character* UsedPlayer, float CustomDuration = -1.f);

	UFUNCTION(BlueprintCallable, Category="Transformation")
	void StopTransformation(bool bEndEffect);

	UFUNCTION(BlueprintCallable, Category="Transformation")
	bool IsTransformed() const { return CurrentTransformation.bActive; }

	UPROPERTY()
	TObjectPtr<APlayer_Character> AppliedPlayer;

	UPROPERTY(ReplicatedUsing = OnRep_Transformation)
	FPlayerTransformation CurrentTransformation;

public:
	//변신 행동 제약
	bool ResolveInputRule(EPlayerInputResult Rule);
	bool GetInputContinueResult(EPlayerInputResult Rule);

	bool CanMoveDuringTransfomation();
	bool CanJumpDuringTransformation();
	bool CanDodgeDuringTransformation();
	bool CanAttackDuringTransformation();
	bool CanUseItemDuringTransformation();
	bool CanInteractionDuringTransformation();

	void GetActiveTransformationMeshes(TArray<UMeshComponent*>& OutMeshes);
	void RemoveTransformationByRule(bool bUseEndEffect, int32 Priority, EPlayerTransformationType RemoveType);

	void NotifyHittedDuringTransformation(APlayer_Character* AttackedPlayer);

protected:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TransformStaticMesh;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> TransformSkeletalMesh;

	FTimerHandle ShortStopTimerHandle;

	UFUNCTION()
	void OnRep_Transformation();

	UFUNCTION(Server, Reliable)
	void Server_ResolveInputRule(EPlayerInputResult Rule);
	bool ResolveInputRule_Internal(EPlayerInputResult Rule);
	void CopyTransformationData(UPlayerTransformationDataAsset* DataAsset, FPlayerTransformation& NewData, APlayer_Character* UsedPlayer, float CustomDuration);

	//변신/변신 해제 시 플레이어의 매쉬, Visual 설정
	void CreateVisualMesh();
	void ApplyTransformationVisual();
	void ApplyNormalVisual();
	void UpdateTransformMeshLocation();

	void ShortStopTransformation();
	void RestoreShortStoppedTransformation();
};
