// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapData.h"
#include "MapConstructor.generated.h"

class ADamageBlock;
class UMatch_EventListDataAsset;

UCLASS()
class PROJECTCC_API AMapConstructor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMapConstructor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Max_X = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Max_Y = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Max_Z = 15;
	UPROPERTY(VisibleAnywhere)
	USceneComponent* Root;
	UPROPERTY(VisibleInstanceOnly)
	TArray<EBlockType> MapData;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Map")
	TArray<FIntVector> FloorBlocksData;
	//각 블럭의 크기
	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	float BlockSize = 100.f;
	//일반 블럭 인스턴스 맵
	UPROPERTY(VisibleAnywhere)
	UInstancedStaticMeshComponent* Normal_ISM;
	//투명 블럭 인스턴스 맵
	UPROPERTY(VisibleAnywhere)
	UInstancedStaticMeshComponent* Transparency_ISM;
	//데미지 블럭 배열
	UPROPERTY()
	TArray<TObjectPtr<ADamageBlock>> Damage_ISM;
	//데미지 블럭 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	TSubclassOf<ADamageBlock> DamageBlock;
	//일반 블럭 매쉬
	UPROPERTY(EditAnywhere, Category="Block Mesh")
	UStaticMesh* Normal_Mesh;
	//투명 블럭 매쉬 (Material에서 Alpha == 0)
	UPROPERTY(EditAnywhere, Category="Block Mesh")
	UStaticMesh* Transparency_Mesh;
	//데이터를 저장할 맵데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
	UMapData* MapInfo = nullptr;
	//맵데이터 백업 파일 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
	FString FileName = "Default";
	//맵데이터 백업 파일 위치
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Map")
	FString FilePath = "/Game/DataAssets/Map/MapBackup";
	//맵 특화 이벤트 리스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event")
	TObjectPtr<UMatch_EventListDataAsset> Match_MapEventList;
	

public:
	//맵 생성/초기화
	UFUNCTION(BlueprintCallable)
	void BuildMap();
	//블럭을 생성/삭제
	UFUNCTION(BlueprintCallable)
	void SetBlock(int32 _x, int32 _y, int32 _z, EBlockType Type);
	//월드 좌표를 맵 단위로 변환
	UFUNCTION(BlueprintCallable)
	bool WorldToMapGrid(const FVector& WorldLocation, int32& mapx, int32& mapy, int32& mapz);
	//월드 좌표를 맵 단위의 최상단 블록 위치로 변환
	bool WorldToGridTopBlock(const FVector& WorldLocation, int32& outX, int32& outY, int32& outZ);
	//마우스 클릭 좌표를 맵 단위로 변환
	UFUNCTION(BlueprintCallable, Category="MapEdit")
	bool HitToMapGrid(const FHitResult& HitLocation, bool bIsCreateBlock, int32& outx, int32& outy, int32& outz);
	//맵 데이터 저장하기
	UFUNCTION(CallInEditor, Category="MapEdit")
	void SaveToDataAsset();
	//맵 데이터 불러오기
	UFUNCTION(CallInEditor, Category="MapEdit")
	void LoadFromDataAsset();
	//맵 데이터 백업
	UFUNCTION(CallInEditor, Category="MapBackup")
	void ExportMapToFile();
	//백업 데이터 로드
	UFUNCTION(CallInEditor, Category="MapBackup")
	void ImportMapFromFile();
	//맵에 존재하는 바닥 블럭(최상단 블럭이면서 NormalType 블럭)들을 확인 및 저장
	void GetAllFloorBlockLocation(int32 MinSafeFloor = 0);
	//맵 좌표가 실제하는지 확인
	UFUNCTION(BlueprintCallable)
	bool IsValidGrid(int32 InX, int32 InY, int32 InZ);
	//좌표에 있는 블럭이 일반 블럭인지 확인
	UFUNCTION(BlueprintCallable)
	bool IsNormalBlock(int32 InX, int32 InY, int32 InZ);
	//좌표의 블럭이 최상단 블럭인지 확인(위에 다른 블럭이 없음)
	UFUNCTION(BlueprintCallable)
	bool IsTopBlock(int32 InX, int32 InY, int32 InZ);
	//좌표의 블럭이 안전 블럭인지 확인
	UFUNCTION(BlueprintCallable)
	bool IsSafeBlock(const FIntVector& Grid);
	//좌표의 블럭이 빈 블럭인지 확인
	UFUNCTION(BlueprintCallable)
	bool IsEmptyBlock(int32 InX, int32 InY, int32 InZ);
	//좌표 근처에서 가장 가까운 TopBlock 획득 
	UFUNCTION(BlueprintCallable)
	bool FindNearestTopBlock(const FVector& WorldLocation, int32& OutX, int32& OutY, int32& OutZ, int32 XYSearchRadius = 1);
	//맵 좌표를 월드 좌표로 전환
	UFUNCTION(BlueprintCallable)
	FVector GridToWorldCenter(int32 InX, int32 InY, int32 InZ);
	//월드 좌표를 맵 좌표(최상단 블럭 위)로 변환
	UFUNCTION(BlueprintCallable)
	FVector GetTopBlockLocationFromWorld(FVector WorldLocation);

public:
	//각 노멀블럭의 UV를 랜덤하게 변경
	int32 AddNormalBlockWithRandomUV(UInstancedStaticMeshComponent* ISMComp, const FTransform& Transform, int32 GridX, int32 GridY, int32 GridZ);
	//바닥 블럭 위에 무언가가 있는지 확인
	bool IsEmptyOnFloorBlock(FIntVector& Grid, const TArray<AActor*> ActorsToIgnore);
	//방향 벡터를 가장 가까운 8방향 그리드 방향으로 전환
	FIntPoint WorldDirectionToGridDirection(const FVector& Direction);
	//그리드 방향을 월드 방향 벡터로 변환
	FVector GridDirectionToWorldDirection(const FIntPoint& GridDirection);
	//Forward 기준 오른쪽 그리드 방향 획득
	FIntPoint GetRightGridDirectionFromForward(const FIntPoint& GridForward);
	//그리드 방향의 Yaw 획득
	float GridDirectionYaw(const FIntPoint& GridDirection);
	//특정 X,Y 좌표의 최상단 블럭 Z 확인 (OutZ가 -1이면 없는 것)
	bool FindTopBlockZAtXY(int32 InX, int32 InY, int32& OutZ);
	//특정 범위 내의 TopBlock들 획득
	void GetTopBlocksInRange(const FVector& WorldOrigin, float Range, TArray<FIntVector>& OutGrids);
public:
	int32 Index(int32 x, int32 y, int32 z) {
		return x * (Max_Y * Max_Z) + (y * Max_Z) + z;
	}
	void ClearDamageBlocks();
};
