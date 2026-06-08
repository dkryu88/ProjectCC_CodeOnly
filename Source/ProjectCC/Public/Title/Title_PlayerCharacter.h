// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Title_PlayerCharacter.generated.h"

UCLASS()
class PROJECTCC_API ATitle_PlayerCharacter : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATitle_PlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Title Preview")
	void AddPreviewYaw(float YawChange);

	UFUNCTION()
	void TurnToFront();

	UFUNCTION()
	void StopTurnToFront();

	UFUNCTION(BlueprintPure, Category = "Title Preview")
	USkeletalMeshComponent* GetPreviewMesh() const { return PreviewMesh; }

protected:
	//프리뷰 캐릭터의 루트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene;

	//프리뷰 캐릭터의 매쉬 (기존 캐릭터와 동일)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> PreviewMesh;

	//회전 시 매쉬만 돌리게 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title Preview")
	bool bRotateMeshOnly = true;

	//캐릭터 회전 속도 (마우스)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Title Preview")
	float DragRotateSpeed = 0.5f;

	//캐릭터 회전 속도 (초기화)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Title Preview")
	float ReturnRotateSpeed = 3.f;

	//캐릭터 초기 회전값
	UPROPERTY(VisibleAnywhere, Category="Title Preview")
	float DefaultYaw = 0.f;

	//앞을 향해 회전하는지에 대한 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Title Preview")
	bool bReturnToFront = false;

};
