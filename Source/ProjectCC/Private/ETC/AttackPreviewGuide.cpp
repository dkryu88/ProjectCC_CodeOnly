// Fill out your copyright notice in the Description page of Project Settings.


#include "ETC/AttackPreviewGuide.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MapConstructor.h"

AAttackPreviewGuide::AAttackPreviewGuide()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RangeMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RangeMesh"));
	SetRootComponent(RangeMesh);

	PathMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("PathMesh"));
	PathMesh->SetupAttachment(RangeMesh);

	SecondPathMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SecondPathMesh"));
	SecondPathMesh->SetupAttachment(RangeMesh);

	RangeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PathMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SecondPathMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RangeMesh->SetGenerateOverlapEvents(false);
	PathMesh->SetGenerateOverlapEvents(false);
	SecondPathMesh->SetGenerateOverlapEvents(false);

	RangeMesh->SetCastShadow(false);
	PathMesh->SetCastShadow(false);
	SecondPathMesh->SetCastShadow(false);

	RangeMesh->TranslucencySortPriority = 0;
	PathMesh->TranslucencySortPriority = 10;
	SecondPathMesh->TranslucencySortPriority = 12;

	SetActorHiddenInGame(true);
}

void AAttackPreviewGuide::UpdatePreview(AMapConstructor* Map, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData)
{
	if (!Map || !PreviewData.CheckUsingAnyVisual()) {
		HidePreview();
		return;
	}

	FVector Forward2D = Forward;
	Forward2D.Z = 0.f;

	if (Forward2D.IsNearlyZero()) Forward2D = FVector::ForwardVector;
	else Forward2D = Forward2D.GetSafeNormal();

	SetActorHiddenInGame(false);

	if (ShouldRebuild(Map, Origin, Forward, PreviewData)) {
		RebuildMeshes(Map, Origin, Forward, PreviewData);
		CachedOriginCell = GetOriginCell(Map, Origin);
		CachedForward2D = Forward2D;

		bCachedShowAttackSector = PreviewData.bShowAttackSector;
		bCachedShowAttackRangeCircle = PreviewData.bShowAttackRangeCircle;
		bCachedShowAttackPath = PreviewData.bShowAttackPath;
		bCachedShowAttackSecondPath = PreviewData.bShowAttackSecondPath;
		bCachedBlockByWall = PreviewData.bBlockByWall;

		bCachedOnlySameHeight = PreviewData.bOnlySameHeight;
		CachedBaseGridZ = PreviewData.BaseGridZ;

		CachedPreviewRange = PreviewData.PreviewRange;
		CachedSecondPreviewRange = PreviewData.SecondPreviewRange;
		CachedHalfAngleDegree = PreviewData.HalfAngleDegree;
		CachedPathRadius = PreviewData.PathRadius;
		CachedSecondPathRadius = PreviewData.SecondPathRadius;
	}

	float PathRangeOverride = -1.f;
	float SecondPathRangeOverride = -1.f;

	FAimPreviewVisualData SecondPathData = PreviewData;
	if (PreviewData.bShowAttackSecondPath) {
		SecondPathData.bShowAttackPath = true;
		SecondPathData.PreviewRange = PreviewData.SecondPreviewRange;
		SecondPathData.PreviewRadius = PreviewData.SecondPathRadius;
		SecondPathData.PathRadius = PreviewData.SecondPathRadius;
		SecondPathData.PathZOffset = PreviewData.PathZOffset + 1.0f;

		if (PreviewData.bBlockByWall) {
			FVector TheForward2D = Forward;
			TheForward2D.Z = 0.f;

			if (TheForward2D.IsNearlyZero()) TheForward2D = FVector::ForwardVector;
			else TheForward2D = TheForward2D.GetSafeNormal();

			FVector SecondPathEnd = Origin + TheForward2D * SecondPathData.PreviewRange;

			float HitDistance = -1.f;
			bool bLineOfSight = HavingPreviewLineOfSight(Origin, SecondPathEnd, SecondPathData, &HitDistance, true);

			if (!bLineOfSight && HitDistance >= 0.f)
			{
				float BlockedOverPath = 15.f;
				SecondPathRangeOverride = FMath::Clamp(HitDistance + BlockedOverPath, 0.f, SecondPathData.PreviewRange);
			}
		}
	}

	if (PreviewData.bShowAttackPath && PreviewData.bBlockByWall) {
		FVector PathEnd = Origin + Forward2D * PreviewData.PreviewRange;
		float HitDistance = -1.f;
		bool bLineOfSight = HavingPreviewLineOfSight(Origin, PathEnd, PreviewData, &HitDistance, true);

		if (!bLineOfSight && HitDistance >= 0.f) {
			float BlockedOverPath = 15.f;

			PathRangeOverride = FMath::Clamp(HitDistance + BlockedOverPath, 0.f, PreviewData.PreviewRange);
		}
	}
	if (RangeMID) UpdateMaterialParams(RangeMID, Origin, Forward, PreviewData);
	if (PathMID) UpdateMaterialParams(PathMID, Origin, Forward, PreviewData, PathRangeOverride);
	if (SecondPathMID) UpdateMaterialParams(SecondPathMID, Origin, Forward, SecondPathData, SecondPathRangeOverride);
}

void AAttackPreviewGuide::HidePreview()
{
	SetActorHiddenInGame(true);

	if (RangeMesh) {
		RangeMesh->SetVisibility(false);
		RangeMesh->ClearAllMeshSections();
	}
	if (PathMesh) {
		PathMesh->SetVisibility(false);
		PathMesh->ClearAllMeshSections();
	}
	if (SecondPathMesh) {
		SecondPathMesh->SetVisibility(false);
		SecondPathMesh->ClearAllMeshSections();
	}

	RangeMID = nullptr;
	PathMID = nullptr;
	SecondPathMID = nullptr;

	CachedOriginCell = FIntPoint(100, 100);
	CachedForward2D = FVector::ZeroVector;

	bCachedShowAttackSector = false;
	bCachedShowAttackRangeCircle = false;
	bCachedShowAttackPath = false;
	bCachedShowAttackSecondPath = false;
	bCachedBlockByWall = false;

	bCachedOnlySameHeight = true;
	CachedBaseGridZ = -1;

	CachedPreviewRange = -1.f;
	CachedSecondPreviewRange = -1.f;
	CachedHalfAngleDegree = -1.f;
	CachedPathRadius = -1.f;
	CachedSecondPathRadius = -1.f;
}

FIntPoint AAttackPreviewGuide::GetOriginCell(AMapConstructor* Map, const FVector& Origin)
{
	if (!Map || Map->BlockSize <= 0.f) return FIntPoint::ZeroValue;

	FVector Local = Origin - Map->GetActorLocation();

	return FIntPoint(FMath::FloorToInt(Local.X / Map->BlockSize), FMath::FloorToInt(Local.Y / Map->BlockSize));
}

bool AAttackPreviewGuide::ShouldRebuild(AMapConstructor* Map, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData)
{
	if (!Map) return false;

	FIntPoint CurrentCell = GetOriginCell(Map, Origin);
	FVector Forward2D = Forward;
	Forward2D.Z = 0.f;

	if (Forward2D.IsNearlyZero()) Forward2D = FVector::ForwardVector;
	else Forward2D = Forward2D.GetSafeNormal();

	if (!CachedForward2D.Equals(Forward2D, 0.001f)) return true;

	if (CachedOriginCell != CurrentCell) return true;

	if (bCachedShowAttackSector != PreviewData.bShowAttackSector) return true;
	if (bCachedShowAttackRangeCircle != PreviewData.bShowAttackRangeCircle) return true;
	if (bCachedShowAttackPath != PreviewData.bShowAttackPath) return true;
	if (bCachedBlockByWall != PreviewData.bBlockByWall) return true;
	if (bCachedShowAttackSecondPath != PreviewData.bShowAttackSecondPath) return true;

	if (bCachedOnlySameHeight != PreviewData.bOnlySameHeight) return true;
	if (CachedBaseGridZ != PreviewData.BaseGridZ) return true;

	if (!FMath::IsNearlyEqual(CachedPreviewRange, PreviewData.PreviewRange, 1.f)) return true;
	if (!FMath::IsNearlyEqual(CachedSecondPreviewRange, PreviewData.SecondPreviewRange, 1.f)) return true;
	if (!FMath::IsNearlyEqual(CachedHalfAngleDegree, PreviewData.HalfAngleDegree, 0.1f)) return true;
	if (!FMath::IsNearlyEqual(CachedPathRadius, PreviewData.PathRadius, 0.1f)) return true;
	if (!FMath::IsNearlyEqual(CachedSecondPathRadius, PreviewData.SecondPathRadius, 0.1f)) return true;

	return false;
}

void AAttackPreviewGuide::RebuildMeshes(AMapConstructor* Map, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData)
{
	if (!Map) return;

	RangeMesh->ClearAllMeshSections();
	PathMesh->ClearAllMeshSections();
	SecondPathMesh->ClearAllMeshSections();

	RangeMesh->SetVisibility(false);
	PathMesh->SetVisibility(false);
	SecondPathMesh->SetVisibility(false);

	RangeMID = nullptr;
	PathMID = nullptr;
	SecondPathMID = nullptr;

	if (PreviewData.bShowAttackSector || PreviewData.bShowAttackRangeCircle) {
		RangeMesh->SetVisibility(true);
		BuildTopFaceMesh(Map, Origin, Forward, PreviewData, PreviewData.RangeZOffset, RangeMesh);
		UMaterialInterface* UseMaterial = PreviewData.bShowAttackRangeCircle ? CircleMaterial : SectorMaterial;

		if (UseMaterial) {
			RangeMID = UMaterialInstanceDynamic::Create(UseMaterial, this);
			RangeMesh->SetMaterial(0, RangeMID);
		}

		RangeMesh->TranslucencySortPriority = 0;
	}

	if (PreviewData.bShowAttackPath) {
		PathMesh->SetVisibility(true);
		BuildTopFaceMesh(Map, Origin, Forward, PreviewData, PreviewData.PathZOffset, PathMesh);
		if (PathMaterial) {
			PathMID = UMaterialInstanceDynamic::Create(PathMaterial, this);
			PathMesh->SetMaterial(0, PathMID);
		}

		PathMesh->TranslucencySortPriority = 10;
	}

	if (PreviewData.bShowAttackSecondPath) {
		SecondPathMesh->SetVisibility(true);
		FAimPreviewVisualData SecondPathData = PreviewData;
		SecondPathData.bShowAttackPath = true;
		SecondPathData.PreviewRange = PreviewData.SecondPreviewRange;
		SecondPathData.PreviewRadius = PreviewData.SecondPathRadius;
		SecondPathData.PathRadius = PreviewData.SecondPathRadius;
		SecondPathData.PathZOffset = PreviewData.PathZOffset + 1.f;

		BuildTopFaceMesh(Map, Origin, Forward, SecondPathData, SecondPathData.PathZOffset, SecondPathMesh);
		if (PathMaterial) {
			SecondPathMID = UMaterialInstanceDynamic::Create(PathMaterial, this);
			SecondPathMesh->SetMaterial(0, SecondPathMID);
		}

		SecondPathMesh->TranslucencySortPriority = 12;
	}
}

bool AAttackPreviewGuide::HavingPreviewLineOfSight(const FVector& Origin, const FVector& TileCenter, const FAimPreviewVisualData& PreviewData, float* OutHitDistance, bool bIsPathType)
{
	if (OutHitDistance) *OutHitDistance = -1.f;
	if (!GetWorld()) return true;
	FVector Start = Origin;
	FVector End = TileCenter;

	Start.Z += 80.f;
	End.Z += 80.f;

	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	if (AActor* OwnerActor = GetOwner()) {
		Params.AddIgnoredActor(OwnerActor);
	}

	FCollisionObjectQueryParams ObjectParams;

	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	if (bIsPathType) {
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);
	}

	FHitResult Hit;
	bool bBlocked = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, Params);

	if (bBlocked && OutHitDistance) *OutHitDistance = FVector::Dist2D(Start, Hit.ImpactPoint);

	return !bBlocked;
}

void AAttackPreviewGuide::GetPreviewTopTiles(AMapConstructor* Map, const FVector& Origin, const FAimPreviewVisualData& PreviewData, TArray<FAimPreviewTopTile>& OutTiles)
{
	OutTiles.Reset();
	if (!Map) return;

	TArray<FIntVector> TopGrids;
	Map->GetTopBlocksInRange(Origin, PreviewData.GetMeshBuildRange(), TopGrids);

	for (const FIntVector& Grid : TopGrids) {
		//Preview가 같은 높이만을 처리할 때, GridZ가 같은 높이(기준 높이)가 아니라면 스킵
		if (PreviewData.bOnlySameHeight && Grid.Z != PreviewData.BaseGridZ) continue;

		FAimPreviewTopTile Tile;
		Tile.Center = Map->GridToWorldCenter(Grid.X, Grid.Y, Grid.Z);
		Tile.Size = Map->BlockSize;

		OutTiles.Add(Tile);
	}
}

void AAttackPreviewGuide::BuildTopFaceMesh(AMapConstructor* Map, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData, float ZOffset, UProceduralMeshComponent* TargetMesh)
{
	if (!Map || !TargetMesh) return;

	bool bIsPathMesh = (TargetMesh == PathMesh || TargetMesh == SecondPathMesh);

	FVector Forward2D = Forward;
	Forward2D.Z = 0.f;

	if (Forward2D.IsNearlyZero()) Forward2D = FVector::ForwardVector;
	else Forward2D = Forward2D.GetSafeNormal();

	FVector Right2D = FVector::CrossProduct(FVector::UpVector, Forward2D).GetSafeNormal();
	if (Right2D.IsNearlyZero()) Right2D = FVector::RightVector;

	TArray<FAimPreviewTopTile> Tiles;
	GetPreviewTopTiles(Map, Origin, PreviewData, Tiles);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	Vertices.Reserve(Tiles.Num() * 4);
	Triangles.Reserve(Tiles.Num() * 6);
	Normals.Reserve(Tiles.Num() * 4);
	UVs.Reserve(Tiles.Num() * 4);
	Colors.Reserve(Tiles.Num() * 4);
	Tangents.Reserve(Tiles.Num() * 4);

	for (const FAimPreviewTopTile& Tile : Tiles) {
		// PathMesh일 때는 먼저 실제 Path 근처 타일만 검사 목록에 추가
		if (bIsPathMesh) {
			FVector ToTile = Tile.Center - Origin;
			ToTile.Z = 0.f;

			float X = FVector::DotProduct(ToTile, Forward2D);
			float Y = FVector::DotProduct(ToTile, Right2D);

			float TileHalf = Tile.Size * 0.5f;
			float XMargin = TileHalf * (FMath::Abs(Forward2D.X) + FMath::Abs(Forward2D.Y));
			float YMargin = TileHalf * (FMath::Abs(Right2D.X) + FMath::Abs(Right2D.Y));

			float VisualMargin = PreviewData.EdgeWidth + PreviewData.EdgeSoftness + 5.f;

			float PathHalfWidth = PreviewData.PathRadius + YMargin + VisualMargin;
			float PathRange = PreviewData.PreviewRange + XMargin + VisualMargin;

			if (X < -XMargin) continue;
			if (X > PathRange) continue;
			if (FMath::Abs(Y) > PathHalfWidth) continue;
		}

		//벽에 막히는지 확인
		if (PreviewData.bBlockByWall && !bIsPathMesh) {
			if (!HavingPreviewLineOfSight(Origin, Tile.Center, PreviewData, nullptr, false)) continue;
		}

		float Half = Tile.Size * 0.5f;
		FVector Center = Tile.Center + FVector(0.f, 0.f, ZOffset);
		int32 BaseIndex = Vertices.Num();

		Vertices.Add(Center + FVector(-Half, -Half, 0.f));
		Vertices.Add(Center + FVector(Half, -Half, 0.f));
		Vertices.Add(Center + FVector(Half, Half, 0.f));
		Vertices.Add(Center + FVector(-Half, Half, 0.f));

		Triangles.Add(BaseIndex + 0);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex + 2);

		Triangles.Add(BaseIndex + 0);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 3);

		UVs.Add(FVector2D(0.f, 0.f));
		UVs.Add(FVector2D(1.f, 0.f));
		UVs.Add(FVector2D(0.f, 1.f));
		UVs.Add(FVector2D(1.f, 1.f));

		for (int32 i = 0; i < 4; i++) {
			Normals.Add(FVector::UpVector);
			Colors.Add(FLinearColor::White);
			Tangents.Add(FProcMeshTangent(1.f, 0.f, 0.f));
		}
	}

	TargetMesh->ClearAllMeshSections();

	if (Vertices.Num() <= 0) return;

	TargetMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void AAttackPreviewGuide::UpdateMaterialParams(UMaterialInstanceDynamic* MID, const FVector& Origin, const FVector& Forward, const FAimPreviewVisualData& PreviewData, float RangeOverride)
{
	if (!MID) return;
	FVector Forward2D = Forward;
	Forward2D.Z = 0.f;

	if (Forward2D.IsNearlyZero()) Forward2D = FVector::ForwardVector;
	Forward2D = Forward2D.GetSafeNormal();

	float UseRange = RangeOverride >= 0.f ? RangeOverride : PreviewData.PreviewRange;

	MID->SetVectorParameterValue(TEXT("Origin"), FLinearColor(Origin.X, Origin.Y, Origin.Z, 1.f));
	MID->SetVectorParameterValue(TEXT("Forward"), FLinearColor(Forward2D.X, Forward2D.Y, Forward2D.Z, 1.f));

	MID->SetScalarParameterValue(TEXT("Range"), UseRange);
	MID->SetScalarParameterValue(TEXT("Radius"), PreviewData.PreviewRadius);
	MID->SetScalarParameterValue(TEXT("HalfAngleDegree"), PreviewData.HalfAngleDegree);
	MID->SetScalarParameterValue(TEXT("PathRadius"), PreviewData.PathRadius);

	MID->SetScalarParameterValue(TEXT("FillOpacity"), PreviewData.FillOpacity);
	MID->SetScalarParameterValue(TEXT("EdgeOpacity"), PreviewData.EdgeOpacity);
	MID->SetScalarParameterValue(TEXT("EdgeWidth"), PreviewData.EdgeWidth);
	MID->SetScalarParameterValue(TEXT("EdgeSoftness"), PreviewData.EdgeSoftness);
	MID->SetScalarParameterValue(TEXT("InnerTransparentRatio"), PreviewData.InnerTransparentRatio);
	MID->SetScalarParameterValue(TEXT("GradientPower"), PreviewData.GradientPower);
	MID->SetScalarParameterValue(TEXT("PatternLength"), PreviewData.PatternLength);
	MID->SetScalarParameterValue(TEXT("FlowSpeed"), PreviewData.FlowSpeed);
}
