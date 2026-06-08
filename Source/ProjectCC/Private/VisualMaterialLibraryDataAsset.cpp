// Fill out your copyright notice in the Description page of Project Settings.


#include "VisualMaterialLibraryDataAsset.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Modules/ModuleManager.h"
#endif

UMaterialInterface* UVisualMaterialLibraryDataAsset::ResolveMaterial(AActor* TargetActor, FName MaterialSetKey, UMeshComponent* Mesh, int32 MaterialIndex)
{
	if (MaterialSetKey.IsNone() || !IsValid(Mesh)) return nullptr;

	FVisualMaterialLibrarySet* MaterialSet = FindMaterialSet(MaterialSetKey);

	if (!MaterialSet) return nullptr;

	UClass* TargetClass = IsValid(TargetActor) ? TargetActor->GetClass() : nullptr;
	const FVisualMaterialLibraryEntry* BestEntry = nullptr;
	//일치하는 정도를 확인 (ExactClassOnly가 false일 경우)
	int32 BestScore = MIN_int32;

	for (const FVisualMaterialLibraryEntry& Entry : MaterialSet->Entries) {
		bool bMatched = true;
		int32 Score = 0;

		if (Entry.ActorClass)
		{
			if (!TargetClass) bMatched = false;
			else {
				//bExactClassOnly가 false일 경우 대상 클래스의 자식 클래스도 포함
				bool bClassMatched = Entry.bExactClassOnly ? TargetClass == Entry.ActorClass : TargetClass->IsChildOf(Entry.ActorClass);
				
				if (!bClassMatched) bMatched = false;
				else Score += (TargetClass == Entry.ActorClass) ? 100 : 50;
			}
		}

		if (Entry.StaticMeshAsset) {
			UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Mesh);

			if (!StaticMeshComp || StaticMeshComp->GetStaticMesh() != Entry.StaticMeshAsset) bMatched = false;
			else Score += 80;
		}

		if (Entry.SkeletalMeshAsset) {
			USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Mesh);

			if (!SkeletalMeshComp || SkeletalMeshComp->GetSkeletalMeshAsset() != Entry.SkeletalMeshAsset) bMatched = false;
			else Score += 80;
		}

		if (!Entry.ActorClass && !Entry.StaticMeshAsset && !Entry.SkeletalMeshAsset) bMatched = false;
		if (!bMatched) continue;
		
		if (Entry.SlotRules.Num() > 0) {
			Score += 10;
		}

		if (Score > BestScore) {
			BestScore = Score;
			BestEntry = &Entry;
		}
	}

	if (!BestEntry) {
		return MaterialSet->FallbackMaterial;
	}

	//조건으로 적합한 Material 검색
	for (const FVisualMaterialSlotRule& Rule : BestEntry->SlotRules) {
		if (!Rule.Material) continue;

		bool bSlotMatched = Rule.MaterialIndex == -1 || Rule.MaterialIndex == MaterialIndex;
		if (!bSlotMatched) continue;

		bool bMeshNameMatched = Rule.MeshCompName.IsNone() || Mesh->GetFName() == Rule.MeshCompName;
		if (!bMeshNameMatched) continue;

		bool bTagMatched = Rule.CompTag.IsNone() || Mesh->ComponentHasTag(Rule.CompTag);
		if (!bTagMatched) continue;

		if (!Rule.MaterialSlotName.IsNone()) {
			int32 SlotIndexFromName = Mesh->GetMaterialIndex(Rule.MaterialSlotName);
			if (SlotIndexFromName != MaterialIndex) {
				continue;
			}
		}

		return Rule.Material;

	}

	if (BestEntry->DefaultMaterial)
	{
		return BestEntry->DefaultMaterial;
	}

	return MaterialSet->FallbackMaterial;

}

FVisualMaterialLibrarySet* UVisualMaterialLibraryDataAsset::FindMaterialSet(FName MaterialSetKey)
{
	if (MaterialSetKey.IsNone()) return nullptr;

	for (FVisualMaterialLibrarySet& Set : MaterialSets) {
		if (Set.MaterialSetKey == MaterialSetKey) {
			return &Set;
		}
	}

	return nullptr;
}

#if WITH_EDITOR

void UVisualMaterialLibraryDataAsset::RebuildGeneratedEntries() {
	Modify();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	for (FVisualMaterialLibrarySet& Set : MaterialSets) {
		if (Set.MaterialSetKey.IsNone()) continue;

		Set.Entries.RemoveAll([](const FVisualMaterialLibraryEntry& Entry) {
			return Entry.bGenerated;
		});

		for (const FVisualMaterialAutoGenerateRule& Rule : Set.AutoGenerateRules) {
			if (Rule.ActorFolderPath.IsNone() || Rule.MaterialFolderPath.IsNone()) continue;

			TArray<FAssetData> BlueprintAssets;

			FARFilter BlueprintFilter;
			BlueprintFilter.PackagePaths.Add(Rule.ActorFolderPath);
			BlueprintFilter.bRecursivePaths = true;
			BlueprintFilter.bRecursiveClasses = true;
			BlueprintFilter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());

			AssetRegistry.GetAssets(BlueprintFilter, BlueprintAssets);

			for (const FAssetData& BlueprintAsset : BlueprintAssets) {
				UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintAsset.GetAsset());

				if (!Blueprint || !Blueprint->GeneratedClass) continue;

				if (Rule.RequireParentClass && !Blueprint->GeneratedClass->IsChildOf(Rule.RequireParentClass)) continue;

				if (HasManualEntryForActorClass(Set, Blueprint->GeneratedClass)) continue;

				FString ActorName = BlueprintAsset.AssetName.ToString();

				if (!Rule.ActorPrefixToRemove.IsEmpty()) {
					ActorName.RemoveFromStart(Rule.ActorPrefixToRemove);
				}

				const FString ExpectedMaterialName = Rule.MaterialPrefix + ActorName + Rule.MaterialSuffix;

				UMaterialInterface* FoundMaterial = FIndMaterialByNameInForder(Rule.MaterialFolderPath, ExpectedMaterialName);

				if (!FoundMaterial) {
					UE_LOG(LogTemp, Warning, TEXT("[VisualMaterialRegistry] Material Not Found. Actor = %s ExpectedMaterial = %s "), *BlueprintAsset.AssetName.ToString(), *ExpectedMaterialName);
					continue;
				}

				FVisualMaterialLibraryEntry NewEntry;
				NewEntry.ActorClass = Blueprint->GeneratedClass;
				NewEntry.DefaultMaterial = FoundMaterial;
				NewEntry.bExactClassOnly = false;
				NewEntry.bGenerated = true;

				Set.Entries.Add(NewEntry);
			}
		}
	}

	MarkPackageDirty();
	PostEditChange();
}

UMaterialInterface* UVisualMaterialLibraryDataAsset::FIndMaterialByNameInForder(FName FolderPath, const FString& MaterialName) {
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> MaterialAssets;

	FARFilter MaterialFilter;
	MaterialFilter.PackagePaths.Add(FolderPath);
	MaterialFilter.bRecursivePaths = true;
	MaterialFilter.bRecursiveClasses = true;

	AssetRegistry.GetAssets(MaterialFilter, MaterialAssets);

	for (const FAssetData& MaterialAsset : MaterialAssets) {
		if (MaterialAsset.AssetName.ToString() != MaterialName) {
			continue;
		}

		UObject* LoadedAsset = MaterialAsset.GetAsset();

		if (UMaterialInterface* Material = Cast<UMaterialInterface>(LoadedAsset)) {
			return Material;
		}
	}

	return nullptr;
}

bool UVisualMaterialLibraryDataAsset::HasManualEntryForActorClass(const FVisualMaterialLibrarySet& Set, UClass* ActorClass)
{
	if (!ActorClass) return false;

	for (const FVisualMaterialLibraryEntry& Entry : Set.Entries) {
		if (Entry.bGenerated) continue;
		if (!Entry.ActorClass) continue;
		if (Entry.ActorClass == ActorClass) return true;
	}

	return false;
}

#endif