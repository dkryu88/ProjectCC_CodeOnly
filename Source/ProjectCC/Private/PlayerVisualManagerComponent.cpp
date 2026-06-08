// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerVisualManagerComponent.h"
#include "Player_Character.h"
#include "Player_State.h"
#include "VisualMaterialLibraryDataAsset.h"
#include "PlayerTransformationComponent.h"
#include "Weapon.h"
#include "Objects.h"
#include "GameFramework/Pawn.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialTypes.h"
#include "TimerManager.h"

// Sets default values for this component's properties
UPlayerVisualManagerComponent::UPlayerVisualManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UPlayerVisualManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickEnabled(false);
}

// Called every frame
void UPlayerVisualManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FName EffectName;
	FVisualEffectRequest HighestEffect;

	if (!GetHighestPriorityEffect(EffectName, HighestEffect)) {
		SetComponentTickEnabled(false);
		return;
	}

	if (!RequestNeedsTick(HighestEffect)) {
		SetComponentTickEnabled(false);
		return;
	}

	UpdateMIDParameters();
}


void UPlayerVisualManagerComponent::Multi_AddVisualEffect_Implementation(FName EffectName, FVisualEffectRequest EffectData)
{
	if (EffectName.IsNone()) {
		return;
	}

	EffectData.ApplyOrder = VisualApplyOrder;
	VisualApplyOrder += 1;

	ActiveVisualEffects.Add(EffectName, EffectData);

	if (GetWorld()) {
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UPlayerVisualManagerComponent::RefreshVisuals);
	}
}


void UPlayerVisualManagerComponent::Multi_RemoveVisualEffect_Implementation(FName EffectName)
{
	if (EffectName.IsNone()) return;

	ActiveVisualEffects.Remove(EffectName);

	if (GetWorld()) {
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UPlayerVisualManagerComponent::RefreshVisuals);
	}
}

void UPlayerVisualManagerComponent::Multi_RefreshVisuals_Implementation() {
	if (GetWorld()) {
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UPlayerVisualManagerComponent::RefreshVisuals);
	}

	RefreshPortraitMaterials();
}

void UPlayerVisualManagerComponent::Multi_RestoreActorVisuals_Implementation(AActor* TargetActor)
{
	if (!IsValid(TargetActor)) return;

	TArray<UMeshComponent*> Meshes;
	TargetActor->GetComponents<UMeshComponent>(Meshes);

	for (UMeshComponent* Mesh : Meshes) {
		RestoreMesh(Mesh);
	}

	RefreshPortraitMaterials();
}

void UPlayerVisualManagerComponent::RefreshVisuals() {
	FName EffectName;
	FVisualEffectRequest HighestEffect;

	if (!GetHighestPriorityEffect(EffectName, HighestEffect)) {
		RestoreOriginalMaterials();
		SetComponentTickEnabled(false);
		return;
	}
	//안전성을 위해 복원 후 적용
	RestoreOriginalMaterials();

	TArray<UMeshComponent*> TargetMeshes = GetTargetMeshes(HighestEffect);

	for (UMeshComponent* Mesh : TargetMeshes) {
		ApplyEffectToMesh(Mesh, HighestEffect);
	}

	UpdateMIDParameters();

	SetComponentTickEnabled(RequestNeedsTick(HighestEffect));
}

void UPlayerVisualManagerComponent::RefreshPortraitMaterials()
{
	APlayer_Character* Player = Cast<APlayer_Character>(GetOwner());
	if (!IsValid(Player)) return;

	APlayer_State* PS = Player->GetThePlayerState();
	if (!IsValid(PS)) return;

	int32 PortraitId = PS->GetPortraitId();
	
	if (PortraitId < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portrait] PortraitId is not initialized yet. PS=%s"), *GetNameSafe(PS));
		return;
	}

	TArray<UMeshComponent*> Meshes;

	Player->GetComponents<UMeshComponent>(Meshes);

	if (Player->TransformationComp) {
		Player->TransformationComp->GetActiveTransformationMeshes(Meshes);
	}
	for (UMeshComponent* Mesh : Meshes) {
		ApplyPortraitIdToMesh(Mesh, PortraitId);
	}
}

bool UPlayerVisualManagerComponent::RequestNeedsTick(const FVisualEffectRequest& EffectData)
{
	for (const FVisualScalarParameter& Param : EffectData.ScalarParameters) {
		if (Param.ValueMode != EVisualScalarRangeSource::Constant) {
			return true;
		}
	}

	return false;
}

void UPlayerVisualManagerComponent::ApplyEffectToMesh(UMeshComponent* Mesh, const FVisualEffectRequest& EffectData)
{
	if (!IsValid(Mesh)) return;
	if (OriginalMaterialsMap.Contains(Mesh)) return;

	FVisualMeshBackup Backup;
	Backup.Mesh = Mesh;
	Backup.bOriginalVisible = Mesh->GetVisibleFlag();
	Backup.bOriginalHiddenInGame = Mesh->bHiddenInGame;

	int32 MaterialCount = Mesh->GetNumMaterials();

	for (int32 i = 0; i < MaterialCount; ++i) {
		Backup.OriginalMaterials.Add(Mesh->GetMaterial(i));
	}

	bool bOwnerView = IsOwnerLocalView();
	EVisualVisibilityMode VisibilityMode = bOwnerView ? EffectData.OwnerVisibilityMode : EffectData.OtherVisibilityMode;
	ApplyVisibilityToMesh(Mesh, VisibilityMode);

	//Material 처리가 필요없는 경우 백업 데이터만 넣고 리턴
	if (EffectData.MaterialSource == EVisualMaterialSource::None) {
		OriginalMaterialsMap.Add(Mesh, Backup);
		return;
	}

	bool bNeedsMID = RequestHasAnyMaterialParameters(EffectData);

	for (int32 i = 0; i < MaterialCount; ++i) {
		UMaterialInterface* OriginalMaterial = nullptr;
		if (Backup.OriginalMaterials.IsValidIndex(i)) {
			OriginalMaterial = Backup.OriginalMaterials[i].Get();
		}
		else {
			OriginalMaterial = Mesh->GetMaterial(i);
		}

		UMaterialInterface* ResolvedMaterial = ResolveMaterialForSlot(Mesh, i, OriginalMaterial, EffectData);

		if (!ResolvedMaterial) continue;

		if (bNeedsMID) {
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ResolvedMaterial, this);

			if (!MID) continue;

			Backup.DynamicMaterials.Add(MID);
			Mesh->SetMaterial(i, MID);

			ApplyParametersToMID(MID, EffectData);
		}
		else {
			Mesh->SetMaterial(i, ResolvedMaterial);
		}
	}

	OriginalMaterialsMap.Add(Mesh, Backup);
}

void UPlayerVisualManagerComponent::ApplyVisibilityToMesh(UMeshComponent* Mesh, EVisualVisibilityMode Mode)
{
	if (!IsValid(Mesh)) return;
	bool bPropagate = false;

	switch (Mode) {
	case EVisualVisibilityMode::Hide:
		Mesh->SetVisibility(false, bPropagate);
		Mesh->SetHiddenInGame(true, bPropagate);
		break;
	case EVisualVisibilityMode::Show:
		Mesh->SetVisibility(true, bPropagate);
		Mesh->SetHiddenInGame(false, bPropagate);
		break;
	case EVisualVisibilityMode::Keep:
	default:
		break;
	}
}

void UPlayerVisualManagerComponent::ApplyPortraitIdToMesh(UMeshComponent* Mesh, int32 PortraitId)
{
	if (!IsValid(Mesh)) return;

	int32 MaterialCount = Mesh->GetNumMaterials();
	
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex) {
		UMaterialInterface* CurrentMaterial = Mesh->GetMaterial(MaterialIndex);
		if (!CurrentMaterial) continue;

		float UsingPortraitColor = 0.f;

		bool bHasUsingPortraitColor = CurrentMaterial->GetScalarParameterValue(FMaterialParameterInfo(UsingPortraitColorParamName), UsingPortraitColor);
	
		//PortraitId를 사용하는 머터리얼이 아닌 경우 스킵
		if (!bHasUsingPortraitColor || UsingPortraitColor < 0.5f) continue;

		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
		if (!MID) MID = Mesh->CreateDynamicMaterialInstance(MaterialIndex, CurrentMaterial);
		if (!MID) continue;

		MID->SetScalarParameterValue(PortraitIdParamName, (float)PortraitId);
	}
}

UMaterialInterface* UPlayerVisualManagerComponent::ResolveMaterialForSlot(UMeshComponent* Mesh, int32 MaterialIndex, UMaterialInterface* OriginalMaterial, const FVisualEffectRequest& EffectData)
{
	if (!IsValid(Mesh)) return nullptr;
	if (EffectData.MaterialSource == EVisualMaterialSource::None) return nullptr;
	if (EffectData.MaterialSource == EVisualMaterialSource::OriginalMaterial) return OriginalMaterial;
	if (EffectData.MaterialSource == EVisualMaterialSource::OverrideMaterial) {
		if (MaterialLibrary && !EffectData.MaterialSetKey.IsNone()) {
			AActor* MeshOwner = Mesh->GetOwner();
			if (UMaterialInterface* RegistryMaterial = MaterialLibrary->ResolveMaterial(MeshOwner, EffectData.MaterialSetKey, Mesh, MaterialIndex)) {
				return RegistryMaterial;
			}
		}

		if (EffectData.FallbackMaterial) {
			return EffectData.FallbackMaterial;
		}
	}
	return OriginalMaterial;
}

void UPlayerVisualManagerComponent::ApplyParametersToMID(UMaterialInstanceDynamic* MID, const FVisualEffectRequest& EffectData)
{
	if (!IsValid(MID)) return;

	bool bOwnerView = IsOwnerLocalView();

	for (const FVisualScalarParameter& Param : EffectData.ScalarParameters) {
		if (Param.ParamName.IsNone()) continue;

		float Value = ResolveScalarValue(Param);
		MID->SetScalarParameterValue(Param.ParamName, Value);
	}

	for (const FVisualVectorParameter& Param : EffectData.VectorParameters) {
		if (Param.ParamName.IsNone()) continue;

		FLinearColor Value = bOwnerView ? Param.OwnerValue : Param.OtherValue;
		MID->SetVectorParameterValue(Param.ParamName, Value);
	}
}

//기준에 맞는 파라미터값 획득 (기준이 추가되면 여기에도 추가 필요)
float UPlayerVisualManagerComponent::GetVisualScalarSourceValue(const FVisualScalarParameter& Param)
{
	AActor* OwnerActor = GetOwner();

	switch (Param.ValueMode) {
	case EVisualScalarRangeSource::Speed2D: {
		FVector PlayerVelocity = GetOwner()->GetVelocity();
		return FVector(PlayerVelocity.X, PlayerVelocity.Y, 0.f).Size();
	}
	case EVisualScalarRangeSource::HP_Ratio:
	{
		APlayer_Character* Player = Cast<APlayer_Character>(GetOwner());
		return Player->HP / Player->BaseStats.Max_HP;
	}
	case EVisualScalarRangeSource::HP_Value:
	{
		APlayer_Character* Player = Cast<APlayer_Character>(GetOwner());
		return Player->HP;
	}
	case EVisualScalarRangeSource::Constant:
	default:
		return 0.f;
	}
}

bool UPlayerVisualManagerComponent::IsVisualEffectTargetMesh(UMeshComponent* Mesh)
{
	if (!IsValid(Mesh)) return false;
	if (Mesh->GetFName() == FName(TEXT("WeaponCollider")) || Mesh->GetFName() == FName(TEXT("PhysicsCollider"))) return false;

	if (AObjects* ObjectOwner = Cast<AObjects>(Mesh->GetOwner())) {
		if (ObjectOwner->GetObjectPhysicsCollider() == Mesh) {
			return false;
		}
	}

	if (AWeapon* WeaponOwner = Cast<AWeapon>(Mesh->GetOwner())) {
		if (WeaponOwner->GetweaponCollider() == Mesh) {
			return false;
		}
	}

	return true;
}

float UPlayerVisualManagerComponent::ResolveScalarValue(const FVisualScalarParameter& Param) {
	bool bOwnerView = IsOwnerLocalView();
	const FVisualScalarRange& Range = bOwnerView ? Param.OwnerRange : Param.OtherRange;

	if (Param.ValueMode == EVisualScalarRangeSource::Constant) {
		return Range.MaxValue;
	}

	float SourceValue = GetVisualScalarSourceValue(Param);
	float GraphValue = 0.f;

	if (!FMath::IsNearlyEqual(Param.MaxValue, Param.MinValue)) {
		GraphValue = (SourceValue - Param.MinValue) / (Param.MaxValue - Param.MinValue);
	}

	GraphValue = FMath::Clamp(GraphValue, 0.f, 1.f);

	if (Param.bInvertCurve) GraphValue = 1.f - GraphValue;

	switch (Param.MappingMode) {
	case EVisualScalarRangeMapping::Linear:
		break;

	case EVisualScalarRangeMapping::Squared:
		GraphValue *= GraphValue;

	case EVisualScalarRangeMapping::Step:
		GraphValue = SourceValue > Param.StepValue ? 1.f : 0.f;
		break;

	case EVisualScalarRangeMapping::Curve:
		if (const FRichCurve* Curve = Param.MappingCurve.GetRichCurveConst()) {
			GraphValue = Curve->Eval(GraphValue);
			GraphValue = FMath::Clamp(GraphValue, 0.f, 1.f);
		}
		break;
	case EVisualScalarRangeMapping::Constant:
	default:
		return Range.MaxValue;
	}

	return FMath::Lerp(Range.MinValue, Range.MaxValue, GraphValue);
}

bool UPlayerVisualManagerComponent::RequestHasAnyMaterialParameters(const FVisualEffectRequest& EffectData)
{
	return EffectData.ScalarParameters.Num() > 0 || EffectData.VectorParameters.Num() > 0;
}

void UPlayerVisualManagerComponent::UpdateMIDParameters()
{
	FName EffectName;
	FVisualEffectRequest HighestEffect;

	if (!GetHighestPriorityEffect(EffectName, HighestEffect)) return;

	for (auto& Pair : OriginalMaterialsMap) {
		FVisualMeshBackup& Backup = Pair.Value;
		for (UMaterialInstanceDynamic* MID : Backup.DynamicMaterials) {
			ApplyParametersToMID(MID, HighestEffect);
		}
	}
}

void UPlayerVisualManagerComponent::RestoreOriginalMaterials() {
	for (auto& Pair : OriginalMaterialsMap) {
		UMeshComponent* Mesh = Pair.Key;

		if (!IsValid(Mesh)) continue;

		FVisualMeshBackup& Backup = Pair.Value;
		int32 RestoreCount = FMath::Min(Backup.OriginalMaterials.Num(), Mesh->GetNumMaterials());

		for (int32 i = 0; i < RestoreCount; ++i) {
			Mesh->SetMaterial(i, Backup.OriginalMaterials[i]);
		}

		bool bPropagate = false;

		Mesh->SetVisibility(Backup.bOriginalVisible, bPropagate);
		Mesh->SetHiddenInGame(Backup.bOriginalHiddenInGame, bPropagate);
	}

	OriginalMaterialsMap.Empty();
}

void UPlayerVisualManagerComponent::RestoreMesh(UMeshComponent* Mesh) {
	if (!IsValid(Mesh)) return;
	if (!OriginalMaterialsMap.Contains(Mesh)) return;

	FVisualMeshBackup Backup = OriginalMaterialsMap[Mesh];

	int32 RestoreCount = FMath::Min(Backup.OriginalMaterials.Num(), Mesh->GetNumMaterials());

	for (int32 i = 0; i < RestoreCount; i++) {
		Mesh->SetMaterial(i, Backup.OriginalMaterials[i]);
	}

	bool bPropagate = false;

	Mesh->SetVisibility(Backup.bOriginalVisible, bPropagate);
	Mesh->SetHiddenInGame(Backup.bOriginalHiddenInGame, bPropagate);

	OriginalMaterialsMap.Remove(Mesh);
}

TArray<UMeshComponent*> UPlayerVisualManagerComponent::GetTargetMeshes(const FVisualEffectRequest& EffectData)
{
	TArray<UMeshComponent*> Result;
	AActor* Owner = GetOwner();

	if (!IsValid(Owner)) return Result;

	APlayer_Character* Player = Cast<APlayer_Character>(Owner);
	if (!Player) {
		TArray<UMeshComponent*> OwnerMeshes;
		Owner->GetComponents<UMeshComponent>(OwnerMeshes);

		for (UMeshComponent* Mesh : OwnerMeshes) {
			Result.AddUnique(Mesh);
		}

		return Result;
 	}

	if (EffectData.bAffectPlayerMesh && Player->GetMesh()) {
		Result.AddUnique(Player->GetMesh());
	}

	if (EffectData.bAffectWeapon && IsValid(Player->NowWeapon)) {
		TArray<UMeshComponent*> Meshes;
		Player->NowWeapon->GetComponents<UMeshComponent>(Meshes);

		for (UMeshComponent* Mesh : Meshes) {
			if (!IsVisualEffectTargetMesh(Mesh)) continue;
			Result.AddUnique(Mesh);
		}
	}

	if (EffectData.bAffectObjects && IsValid(Player->NowObjects)) {
		TArray<UMeshComponent*> Meshes;
		Player->NowObjects->GetComponents<UMeshComponent>(Meshes);

		for (UMeshComponent* Mesh : Meshes) {
			if (!IsVisualEffectTargetMesh(Mesh)) continue;
			Result.AddUnique(Mesh);
		}
	}

	if (EffectData.bAffectSupport && IsValid(Player->NowSupport)) {
		TArray<UMeshComponent*> Meshes;
		Player->NowSupport->GetComponents<UMeshComponent>(Meshes);

		for (UMeshComponent* Mesh : Meshes) {
			if (!IsVisualEffectTargetMesh(Mesh)) continue;
			Result.AddUnique(Mesh);
		}
	}

	if (EffectData.bAffectTransformationMesh) {
		if (UPlayerTransformationComponent* TransformComp = Player->FindComponentByClass<UPlayerTransformationComponent>()) {
			TArray<UMeshComponent*> TransformMeshes;
			TransformComp->GetActiveTransformationMeshes(TransformMeshes);

			for (UMeshComponent* Mesh : TransformMeshes) {
				Result.AddUnique(Mesh);
			}
		}
	}

	return Result;
}

bool UPlayerVisualManagerComponent::GetHighestPriorityEffect(FName& OutName, FVisualEffectRequest& OutEffect)
{
	bool bFound = false;
	int32 BestPriority = MIN_int32;
	int32 BestApplyOrder = MIN_int32;

	for (const auto& Pair : ActiveVisualEffects) {
		const FVisualEffectRequest& Effect = Pair.Value;
		
		bool bHigherPriority = Effect.Priority > BestPriority;
		//같은 우선순위인 경우 새로운 Effect를 적용
		bool bFinalSelectedEffect = bHigherPriority || (Effect.Priority == BestPriority && Effect.ApplyOrder > BestApplyOrder);

		if (!bFound || bFinalSelectedEffect) {
			OutName = Pair.Key;
			OutEffect = Effect;
			BestPriority = Effect.Priority;
			BestApplyOrder = Effect.ApplyOrder;
			bFound = true;
		}
	}

	return bFound;
}

//현재 적용하는 플레이어가 자신인지 다른 플레이어인지 구별
bool UPlayerVisualManagerComponent::IsOwnerLocalView() const
{
	const APawn* PawnOwner = Cast<APawn>(GetOwner());

	if (!PawnOwner) return false;

	return PawnOwner->IsLocallyControlled();
}
