// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Materials/MaterialInterface.h"
#include "VisualMaterialLibraryDataAsset.generated.h"

class UMeshComponent;
class UStaticMesh;
class USkeletalMesh;

/**
 * VisualMaterialLibrary내부 무기/물체에 대해 특정 MeshComp/ComponentTag/MaterialSlot의 규칙을 설정
 * 대부분의 경우 Default Material만 사용/필요시 여기서 적용 Material 검색
 */

//Material 검색 조건
USTRUCT(BlueprintType)
struct FVisualMaterialSlotRule
{
	GENERATED_BODY()
	//매쉬 컴포넌트 이름 (비워두면 이름으로 검사 X)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	FName MeshCompName = NAME_None;
	
	//컴포넌트 태그 (비워두면 Tag로 검사 X)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	FName CompTag = NAME_None;

	//머터리얼 슬롯의 이름 (비워두면 슬롯 이름으로 검사 X)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	FName MaterialSlotName = NAME_None;

	//Index가 -1이면 모든 Material Slot에 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	int32 MaterialIndex = -1;

	//조건에 부합하는 머터리얼 포인터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	TObjectPtr<UMaterialInterface> Material = nullptr;
};

//특정 Actor 또는 Mesh에 대해 MaterialSetKey 상태일 때 어떤 머터리얼을 사용하는지 저장
USTRUCT(BlueprintType)
struct FVisualMaterialLibraryEntry 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	TSubclassOf<AActor> ActorClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	TObjectPtr<UStaticMesh> StaticMeshAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	TObjectPtr<USkeletalMesh> SkeletalMeshAsset = nullptr;

	//조건에 맞는 머터리얼이 없을 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	TObjectPtr<UMaterialInterface> DefaultMaterial = nullptr;

	//머터리얼 검색 규칙 (여러 Material로 구성될 수 있으므로 Array 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	TArray<FVisualMaterialSlotRule> SlotRules;

	//bExactClassOnly가 True면 정확히 같은 클래스만 허용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	bool bExactClassOnly = false;

	//에디터에서 자동으로 생성된 Entry인지 표시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	bool bGenerated = false;
};

//에디터 내에서 DataAsset에 자동으로 무기/물체/서포트 등을 지정하기 위해 사용
USTRUCT(BlueprintType)
struct FVisualMaterialAutoGenerateRule
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto")
	FName ActorFolderPath = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto")
	FName MaterialFolderPath = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto")
	TSubclassOf<AActor> RequireParentClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto")
	FString ActorPrefixToRemove = TEXT("BP_");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto")
	FString MaterialPrefix = TEXT("MI_");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto")
	FString MaterialSuffix = TEXT("");
};

USTRUCT(BlueprintType)
struct FVisualMaterialLibrarySet {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	FName MaterialSetKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	TObjectPtr<UMaterialInterface> FallbackMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	TArray<FVisualMaterialLibraryEntry> Entries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Material")
	TArray<FVisualMaterialAutoGenerateRule> AutoGenerateRules;
};

//VisualManager가 특정 액터에 대해 머터리얼을 변경할 때 참조하는 데이터에셋
//에디터에서 자동으로 Entry를 생성하며 런타임에서 게임 내부를 탐색하지 않음
UCLASS()
class PROJECTCC_API UVisualMaterialLibraryDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Material")
	TArray<FVisualMaterialLibrarySet> MaterialSets;

	//검색 결과 머터리얼
	UMaterialInterface* ResolveMaterial(AActor* TargetActor, FName MaterialSetKey, UMeshComponent* Mesh, int32 MaterialIndex);

private:
	FVisualMaterialLibrarySet* FindMaterialSet(FName MaterialSetKey);

//에디터 한정으로 실행할 함수
#if WITH_EDITOR
public:
	UFUNCTION(CallInEditor, Category="Auto Generate")
	void RebuildGeneratedEntries();

private:
	UMaterialInterface* FIndMaterialByNameInForder(FName FolderPath, const FString& MaterialName);

	bool HasManualEntryForActorClass(const FVisualMaterialLibrarySet& Set, UClass* ActorClass);

#endif
};
