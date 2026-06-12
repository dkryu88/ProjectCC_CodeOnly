// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment.h"
#include "Item.generated.h"

class UItemDataAsset;
class UItemEffect;
class APlayer_Character;
class UBoxComponent;

UCLASS()
class PROJECTCC_API AItem : public AEquipment
{
	GENERATED_BODY()

public:
	AItem();
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual bool PickedByPlayer(APlayer_Character* Player) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:
	//각 아이템 데이터
	UPROPERTY(EditDefaultsOnly, Category = "ItemData")
	TObjectPtr<UItemDataAsset> ItemData;
	//남은 사용 횟수
	UPROPERTY(Replicated)
	int32 NowUseCount = -2;
	//아이템 회전 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coin")
	float SpinSpeed = 120.f;
	//아이템 물리 Collider
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	UBoxComponent* ItemCollider;
	//아이템 물리 Collider 크기 조정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	float Radius = 25.f;
	/*--ItemCollider과 독립적으로 수정하기 위함--*/
	//아이템 Mesh의 기준점
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	USceneComponent* MeshPivot;
	//Mesh의 위치 기준
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FVector MeshLocation = FVector::ZeroVector;
	//Mesh의 회전 기준
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FRotator MeshRotation = FRotator::ZeroRotator;
	//Mesh의 크기 기준
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FVector MeshScale = FVector(1.f, 1.f, 1.f);
	//아이템 물리 Collider 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector SizeMagnification = FVector(1.5f, 1.5f, 1.5f);
	//아이템 물리 Collider 크기 보정값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector ColliderOffset = FVector(0.f, 0.f, 0.f);
	/*--------------------------------------------*/
public:
	//아이템 장착
	void Equip(APlayer_Character* Player);
	//아이템 사용
	void UseItem();
	//아이템 사용 효과
	bool ApplyUseEffect();
	//아이템 사용 횟수 검사
	bool CheckUseCounting();
	//아이템 해제
	void UnEquip(APlayer_Character* Player);
	//아이템 이름 반환
	FName ItemName();
	//아이템 물리 Collider 반환 (물리 계산시 사용)
	UBoxComponent* GetItemCollider();
	//장착 상태 적용
	virtual void ApplyEquipState() override;
	//해제 상태 적용
	virtual void ApplyWorldState() override;
	//아이템 물리 Collider 사이즈 설정
	void SetSizeofItemColliderwithMesh();

	// [사운드]=============================
	// 아이템 픽업 사운드 : 생성자에서 위치주소로 찾게해서 모든 자식 아이템들에서 공통으로 사용 가능
	UPROPERTY(VisibleAnywhere, Category="Sound")
	TObjectPtr<class USoundBase> ItemPickupSound;
	// 아이템 사용 사운드 : 자식 클래스의 블루프린트에서 참조하여 사용하게 함(부모클래스에서 1번만 선언해 모든 자식클래스에서 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<class USoundBase> ItemUseSound;
};

//효과 구현은 Equipment에 있는 효과 함수를 Override 하여 사용
//Ex : 사용 효과 : virtual void UseEffect_Implementation(APlayer_Character* Player) override;