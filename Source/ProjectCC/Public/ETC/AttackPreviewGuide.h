// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponDataAsset.h"
#include "AttackPreviewGuide.generated.h"

/**
 *
 */

class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AMapConstructor;

USTRUCT(BlueprintType)
struct FAimPreviewTopTile
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Center = FVector::ZeroVector;

	UPROPERTY()
	float Size = 100.f;
};

USTRUCT(BlueprintType)
struct FAimPreviewVisualData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bVisible = false;
	//근접/발사/ShootHS Degree 범위
	UPROPERTY()
	bool bShowAttackSector = false;
	//투척 최대 사거리 원
	UPROPERTY()
	bool bShowAttackRangeCircle = false;
	//발사/ShootHS 공격의 실제 발사 경로
	UPROPERTY()
	bool bShowAttackPath = false;
	//UnrealUnit 기준 사거리
	UPROPERTY()
	float PreviewRange = 0.f;
	//Preview에 사용할 AttackRadius
	UPROPERTY()
	float PreviewRadius = 0.f;
	//부채꼴 절반 각도 (근접)
	UPROPERTY()
	float HalfAngleDegree = 0.f;
	//발사 경로 폭의 절반 (발사/ShootHS)
	UPROPERTY()
	float PathRadius = 10.f;
	//같은 높이에만 표시할 것인지 여부
	UPROPERTY()
	bool bOnlySameHeight = true;
	//플레이어의 위치 기준 블럭의 Z 좌표
	UPROPERTY()
	int32 BaseGridZ = 0;
	//Preview의 Z Offset 값 (완전 바닥과 동일하게 하면 겹쳐서 깜빡임)
	UPROPERTY()
	float RangeZOffset = 1.f;
	UPROPERTY()
	float PathZOffset = 1.f;
	//Preview 내부 Opacity
	UPROPERTY()
	float FillOpacity = 0.15f;
	//Preview 테두리 Opacity
	UPROPERTY()
	float EdgeOpacity = 0.85f;
	//Preview 테두리 폭
	UPROPERTY()
	float EdgeWidth = 8.f;
	//Preview 테두리 부드러움 정도
	UPROPERTY()
	float EdgeSoftness = 2.f;
	//Preview 내부 투영도
	UPROPERTY()
	float InnerTransparentRatio = 0.25f;
	//Preview 내부 Opacity Gradient 정도
	UPROPERTY()
	float GradientPower = 1.5f;
	//Preview 패턴 길이
	UPROPERTY()
	float PatternLength = 80.f;
	//Preview Texture 흐름 속도
	UPROPERTY()
	float FlowSpeed = 0.01f;
	//블럭에 Preview가 막힐지 여부
	UPROPERTY()
	bool bBlockByWall = true;
	//무기 공격 방향
	UPROPERTY()
	EWeaponAttackDirection AttackDirection = EWeaponAttackDirection::Horizontal;
	//Preview 데이터 초기화
	void Reset() {
		bVisible = false;
		bShowAttackSector = false;
		bShowAttackRangeCircle = false;
		bShowAttackPath = false;
		bBlockByWall = true;

		PreviewRange = 0.f;
		PreviewRadius = 0.f;
		HalfAngleDegree = 0.f;
		PathRadius = 10.f;

		bOnlySameHeight = true;
		BaseGridZ = 0;

		RangeZOffset = 1.f;
		PathZOffset = 2.f;

		FillOpacity = 0.15f;
		EdgeOpacity = 0.85f;

		EdgeWidth = 8.f;
		EdgeSoftness = 2.f;

		InnerTransparentRatio = 0.25f;
		GradientPower = 1.5f;
		PatternLength = 80.f;
		FlowSpeed = 0.2f;
	}

	bool CheckUsingAnyVisual() const
	{
		return bVisible && PreviewRange > 0.f && (bShowAttackSector || bShowAttackRangeCircle || bShowAttackPath);
	}

	float GetMeshBuildRange() const
	{
		//테두리/Glow가 범위 밖으로 살짝 퍼져도 잘리지 않도록 여유를 둠
		return PreviewRange + EdgeWidth + EdgeSoftness + 20.f;
	}
};

UCLASS()
class PROJECTCC_API AAttackPreviewGuide : public AActor {
	GENERATED_BODY()

public:
	AAttackPreviewGuide();

	void UpdatePreview(AMapConstructor* Map, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData);
	void HidePreview();

protected:
	UPROPERTY(VisibleAnywhere, Category = "AttackPreview")
	TObjectPtr<UProceduralMeshComponent> RangeMesh;

	UPROPERTY(VisibleAnywhere, Category = "AttackPreview")
	TObjectPtr<UProceduralMeshComponent> PathMesh;

	UPROPERTY(EditAnywhere, Category = "AttackPreview")
	TObjectPtr<UMaterialInterface> SectorMaterial;

	UPROPERTY(EditAnywhere, Category = "AttackPreview")
	TObjectPtr<UMaterialInterface> CircleMaterial;

	UPROPERTY(EditAnywhere, Category = "AttackPreview")
	TObjectPtr<UMaterialInterface> PathMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RangeMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PathMID;

private:
	FIntPoint CachedOriginCell = FIntPoint(100, 100);

	bool bCachedShowAttackSector = false;
	bool bCachedShowAttackRangeCircle = false;
	bool bCachedShowAttackPath = false;
	bool bCachedOnlySameHeight = true;

	int32 CachedBaseGridZ = -1;

	float CachedPreviewRange = -1.f;
	float CachedHalfAngleDegree = -1.f;
	float CachedPathRadius = -1.f;
	FVector CachedForward2D = FVector::ZeroVector;

	bool bCachedBlockByWall = true;

	FIntPoint GetOriginCell(AMapConstructor* Map, const FVector& Origin);

	bool ShouldRebuild(AMapConstructor* Map, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData);
	void RebuildMeshes(AMapConstructor* Map, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData);
	bool HavingPreviewLineOfSight(const FVector& Origin, const FVector& TileCenter, const FAimPreviewVisualData& PreviewData, float* OutHitDistance, bool bIsPathType);

	void GetPreviewTopTiles(AMapConstructor* Map, const FVector& Origin, const FAimPreviewVisualData& PreviewData, TArray<FAimPreviewTopTile>& OutTiles);
	void BuildTopFaceMesh(AMapConstructor* Map, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData, float ZOffset, UProceduralMeshComponent* TargetMesh);
	void UpdateMaterialParams(UMaterialInstanceDynamic* MID, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData, float RangeOverride = -1.f);
};