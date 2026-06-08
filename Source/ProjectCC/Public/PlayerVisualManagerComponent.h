// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerVisualManagerComponent.generated.h"

class UMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UVisualMaterialLibraryDataAsset;

UENUM(BlueprintType)
enum class EVisualMaterialSource : uint8
{
	//변경 머터리얼 없음 (Visibility만 변경)
	None				UMETA(DisplayName = "None Material"),
	//기본 머터리얼
	OriginalMaterial	UMETA(DisplayName = "Original Material"),
	//재적용 머터리얼
	OverrideMaterial	UMETA(DisplayName = "Override Material")
};

UENUM(BlueprintType)
enum class EVisualVisibilityMode : uint8
{
	Keep UMETA(DisplayName = "Keep"),
	Show UMETA(DisplayName = "Show"),
	Hide UMETA(DisplayName = "Hide")
};

UENUM(BlueprintType)
enum class EVisualScalarRangeSource : uint8
{
	Constant, //단일값
	Speed2D,  //속도에 따라 변경
	HP_Ratio, //체력 비율에 따라 변경
	HP_Value,  //체력 값에 따라 변경
};

UENUM(BlueprintType)
enum class EVisualScalarRangeMapping : uint8
{
	Constant, //단일값
	Linear, //직선 증가
	Squared, //곡선 증가
	Step, //특정 값으로 비교해서 x 또는 y
	Curve //커스텀 곡선 증가
};

USTRUCT(BlueprintType)
struct FVisualScalarRange {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	float MinValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	float MaxValue = 1.f;

};

//머터리얼 파라미터 (float)
USTRUCT(BlueprintType)
struct FVisualScalarParameter {
	GENERATED_BODY()
	//적용 Condition/Transformation 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	FName ParamName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	EVisualScalarRangeSource ValueMode = EVisualScalarRangeSource::Constant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	FVisualScalarRange OwnerRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	FVisualScalarRange OtherRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	EVisualScalarRangeMapping MappingMode = EVisualScalarRangeMapping::Constant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	float MinValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	float MaxValue = 600.f;


	//StepValue보다 큰지 작은지를 판별하여 값 결정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	float StepValue = 5.f;

	//우상향(false) < - > 좌상향(true) 값 변경 그래프 결정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	bool bInvertCurve = false;

	//Custom 모드일 때 사용할 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	FName CustomSourceName = NAME_None;

	//Curve 모드일 때 사용할 커브 결정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	FRuntimeFloatCurve MappingCurve;
};

//머터리얼 파라미터 (Vector)
USTRUCT(BlueprintType)
struct FVisualVectorParameter
{
	GENERATED_BODY()
	//적용 Condition/Transformation 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	FName ParamName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	FLinearColor OwnerValue = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	FLinearColor OtherValue = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct FVisualEffectRequest {
	GENERATED_BODY()
	//적용 순서 (뒤에 적용될수록 높음) (동일 우선순위의 효과 적용시 판별)
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	int32 ApplyOrder = 0;
	//우선순위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	int32 Priority = 0;
	
	//변신 컴포넌트가 만든 Transform Mesh를 포함하는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAffectTransformationMesh = true;

	//머터리얼 재적용 여부 확인
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Material")
	EVisualMaterialSource MaterialSource = EVisualMaterialSource::None;

	//재적용 머터리얼을 찾기 위한 키워드 (변신, Condition 이름과 동일)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Material")
	FName MaterialSetKey = NAME_None;

	//재적용 머터리얼 찾기 실패 시 사용할 머터리얼
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Material")
	TObjectPtr<UMaterialInterface> FallbackMaterial = nullptr;

	//각 머터리얼의 Parameter들의 값 (Scalar) <ex : opacity>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Parameters")
	TArray<FVisualScalarParameter> ScalarParameters;
	//각 머터리얼의 Parameter들의 값 (Vector) <ex : ColorTint>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Parameters")
	TArray<FVisualVectorParameter> VectorParameters;

	//Visual 처리 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Visibility")
	EVisualVisibilityMode OwnerVisibilityMode = EVisualVisibilityMode::Keep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Visibility")
	EVisualVisibilityMode OtherVisibilityMode = EVisualVisibilityMode::Keep;

	//적용 대상 설정//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Target")
	bool bAffectPlayerMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Target")
	bool bAffectWeapon = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Target")
	bool bAffectObjects = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Target")
	bool bAffectSupport = true;
};

//변신/Condition 해제 시 복구용 Material/Visibility 데이터
USTRUCT()
struct FVisualMeshBackup {
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UMeshComponent> Mesh = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	UPROPERTY()
	bool bOriginalVisible = true;

	UPROPERTY()
	bool bOriginalHiddenInGame = false;

};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTCC_API UPlayerVisualManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerVisualManagerComponent();
	// Called when the game starts
	virtual void BeginPlay() override;
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	//서버에 모든 클라이언트에 대해 EffectName Visual 효과 추가 요청
	UFUNCTION(NetMulticast, Reliable)
	void Multi_AddVisualEffect(FName EffectName, FVisualEffectRequest EffectData);
	//서버에 모든 클라이언트에 대해 EffectName Visual 효과 제거 요청
	UFUNCTION(NetMulticast, Reliable)
	void Multi_RemoveVisualEffect(FName EffectName);
	//서버에 모든 클라이언트에 대해 Visual 효과 갱신
	UFUNCTION(NetMulticast, Reliable)
	void Multi_RefreshVisuals();
	//서버에 모든 클라이언트에 대해 백업 데이터로 TargetActor Visual 원상 복구
	UFUNCTION(NetMulticast, Reliable)
	void Multi_RestoreActorVisuals(AActor* TargetActor);

	void RefreshVisuals();
	void RefreshPortraitMaterials();
	void RestoreOriginalMaterials();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Material")
	TObjectPtr<UVisualMaterialLibraryDataAsset> MaterialLibrary = nullptr;

protected:
	UPROPERTY()
	TMap<FName, FVisualEffectRequest> ActiveVisualEffects;

	UPROPERTY()
	TMap<TObjectPtr<UMeshComponent>, FVisualMeshBackup> OriginalMaterialsMap;

	UPROPERTY(EditDefaultsOnly, Category="Portrait Material")
	FName PortraitIdParamName = TEXT("PortraitId");

	UPROPERTY(EditDefaultsOnly, Category="Portrait Material")
	FName UsingPortraitColorParamName = TEXT("UsingPortraitColor");

	int32 VisualApplyOrder = 0;
protected:
	//적용 대상이 된 모든 mesh를 확인 (플레이어와 플레이어 장착물들을 조건으로 검사)
	TArray<UMeshComponent*> GetTargetMeshes(const FVisualEffectRequest& EffectData);
	//가장 높은 우선순위의 Visual Effect 확인
	bool GetHighestPriorityEffect(FName& OutName, FVisualEffectRequest& OutEffect);
	//현재 적용하는 플레이어가 자신인지 다른 플레이어인지 구별
	bool IsOwnerLocalView() const;
	//파라미터가 Constant인지 시간에 따라 변화하는 값인지 확인
	bool RequestNeedsTick(const FVisualEffectRequest& EffectData);
	//각 매쉬에 Visual Effect를 적용
	void ApplyEffectToMesh(UMeshComponent* Mesh, const FVisualEffectRequest& EffectData);
	//각 매쉬에 Visibility 적용 (VisualMaterialSource가 None인 경우)
	void ApplyVisibilityToMesh(UMeshComponent* Mesh, EVisualVisibilityMode Mode);
	//각 머터리얼 중 Portrait Id로 Color를 결정하는 머터리얼에 Portrait Id 적용
	void ApplyPortraitIdToMesh(UMeshComponent* Mesh, int32 PortraitId);
	//각 매쉬를 기본 상태로 복원
	void RestoreMesh(UMeshComponent* Mesh);
	//머터리얼의 파라미터를 갱신
	void UpdateMIDParameters();
	//시간에 따라 변동되는 파라미터의 값을 확인 (그래프 형태)
	float ResolveScalarValue(const FVisualScalarParameter& Param);
	//머터리얼 파라미터를 바꿔야하는지 확인
	bool RequestHasAnyMaterialParameters(const FVisualEffectRequest& EffectData);
	//각 플레이어에게 적용될 Material을 구분해서 획득
	UMaterialInterface* ResolveMaterialForSlot(UMeshComponent* Mesh, int32 MaterialIndex, UMaterialInterface* OriginalMaterial, const FVisualEffectRequest& EffectData);
	//각 머터리얼의 파라미터를 적용
	void ApplyParametersToMID(UMaterialInstanceDynamic* MID, const FVisualEffectRequest& EffectData);
	//머터리얼 파라미터에 제공할 기준에 맞는 값을 획득
	float GetVisualScalarSourceValue(const FVisualScalarParameter& Param);
	//StaticMeshCollider는 제외
	bool IsVisualEffectTargetMesh(UMeshComponent* Mesh);
};
