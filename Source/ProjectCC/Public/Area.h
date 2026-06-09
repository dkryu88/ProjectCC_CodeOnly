// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Area.generated.h"

class UBoxComponent;
class UEffectManagerComponent;
class AMapConstructor;
class APlayer_Character;

UCLASS()
class PROJECTCC_API AArea : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AArea();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Area")
	TObjectPtr<UBoxComponent> AreaDetectCollider;
	//차후에 이펙트로 실질적 처리 (이펙트 제작 후 매쉬 제거)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Area")
	USceneComponent* MeshPivot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Area")
	TObjectPtr<UStaticMeshComponent> TestMesh;
	UPROPERTY()
	UEffectManagerComponent* EffectManagerComp;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AreaDetail")
	float AreaDuration = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AreaDetail")
	float AreaEffectInterval = 0.25;
	UPROPERTY(EditDefaultsOnly)
	FIntVector GridLocation = FIntVector::ZeroValue;
	
public:
	//Area 공용 정보
	UPROPERTY()
	TObjectPtr<AMapConstructor> NowMap = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<APlayer_Character> OwnPlayer;
	//Area 그룹 관리-----------------------------------
	//Area들중 좌상단(루트)
	UPROPERTY()
	TObjectPtr<AArea> RootArea = nullptr;
	UPROPERTY()
	bool bIsRoot = false;
	UPROPERTY()
	TArray<AArea*> AllArea;

	//Area 내부에 있는 Actor들 확인
	TSet<TWeakObjectPtr<AActor>> GroupActorsInArea;

	//플레이어 별 Persist Effect interval 차등 적용
	TMap<TWeakObjectPtr<AActor>, FTimerHandle> ActorRefreshTimerHandles;

	//같은 RootArea를 가진 Area들 관리
	TMap<TWeakObjectPtr<AActor>, TSet<TWeakObjectPtr<AArea>>> GroupOverlapAreaSet;

	//Area 내의 특정 Player의 Stay Effect 발동 타이머 시작 (Begin Overlap시 발동)
	void StartActorRefreshTimer(AActor* OtherActor);
	//특정 Player의 stay Effect 발동 타이머 종료 
	void StopActorRefreshTimer(AActor* OtherActor);
	//ActorKey를 사용하여 해당 Actor 타이머 정지
	void StopActorRefreshTimerByKey(TWeakObjectPtr<AActor> ActorKey);
	//대상 Actor에 대해 StayAreaEffect 적용 / 장판에서 벗어난 Actor를 정리
	void RefreshActorEffect(TWeakObjectPtr<AActor> ActorPtr);

	// Area 생성 시 이미 안에 있는 액터 등록
	void RegisterInitialOverlaps();

protected:
	UFUNCTION()
	void OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UFUNCTION(BlueprintNativeEvent, Category = "AreaEffect")
	void ApplyInAreaEffect(AActor* OtherActor);
	UFUNCTION(BlueprintNativeEvent, Category = "AreaEffect")
	void ApplyOutAreaEffect(AActor* OtherActor);
	UFUNCTION(BlueprintNativeEvent, Category = "AreaEffect")
	void ApplyStayAreaEffect(AActor* OtherActor);

public:	
	//Spawn 가능한 Grid 좌표인지 확인
	FIntVector CheckCanSpawnOnTheLocation(FIntVector CandidateLocation);
	//각 Area들을 Grid 좌표(x, y, z)에서 초기화
	bool InitializeArea(int32 x, int32 y, int32 z);
	//Area Spawn 좌표를 기준으로 Area Spawn Transform 획득
	FTransform GetSpawnTransform();
	//Area 크기를 Grid에 맞게 설정
	void ApplyAreaSizeFromGrid();
	//Area의 Transform을 재설정
	void ResetTransformFromGrid();
	//Area 데이터 설정(Area를 생성하는 Owner에서 호출)
	void SetAreaData(float Duration, float Interval);
	//Area 그룹 Overlap 처리
	void HandleGroupBeginOverlapFromArea(AArea* area, AActor* OtherActor);
	void HandleGroupEndOverlapFromArea(AArea* area, AActor* OtherActor);

	//Area 그룹 지속Effect Interval 타이머 설정
	void ClearGroupEffects();

	//Area 그룹의 RootArea 획득
	AArea* GetRootArea();
	//Area가 같은 Grid 위치에 있는지 확인
	bool IsSameGrid(const FIntVector& grid);
	//같은 위치의 Area를 제거
	void DestroyOnlySameGridArea();
	//Area 그룹의 Root Area를 소속된 다른 Area로 변경
	void TransferRootToOtherArea(AArea* NewRoot);
	//Grid 위치에 Area가 있는지 없는지 확인
	static AArea* FindExistingAreaAtGrid(UWorld* World, AMapConstructor* Map, const FIntVector& Grid, AArea* IgnoreArea = nullptr);

	//Grid 좌표 획득
	FIntVector GetGridLocation() { return GridLocation; }
	//현재 맵 획득
	AMapConstructor* GetCurrentMap() { return NowMap; }

};

/*알아두기*/
// TMap<자료형A, 자료형B> : 자료형A, 자료형B를 매칭 시켜 저장
// TSet<자료형> : 자료형에 맞는 값을 저장 (중복 X) 