// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerTransformationComponent.h"
#include "Player_Character.h"
#include "Weapon.h"
#include "Objects.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PlayerTransformationEffect.h"
#include "PlayerTransformationDataAsset.h"
#include "PlayerTransformationAnimation.h"
#include "PlayerConditionComponent.h"
#include "Player_CharacterWidget.h"
#include "PlayerConditionComponent.h"
#include "NiagaraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"


// Sets default values for this component's properties
UPlayerTransformationComponent::UPlayerTransformationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UPlayerTransformationComponent::BeginPlay()
{
	Super::BeginPlay();

	AppliedPlayer = Cast<APlayer_Character>(GetOwner());

	CreateVisualMesh();

	if (CurrentTransformation.bActive) {
		ApplyTransformationVisual();

		if (CurrentTransformation.TransformPersistEffect.NiagaraEffect) {
			StartTransformLoopEffect_Local(CurrentTransformation.TransformPersistEffect);
		}
	}
	else {
		ApplyNormalVisual();
		SetComponentTickEnabled(false);
		return;
	}

}

// Called every frame
void UPlayerTransformationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!AppliedPlayer) {
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}
	if (!CurrentTransformation.bActive) return;

	UpdateTransformMeshLocation();

	if (bNoShowingPlayerOverlayMaterialDuringTransformation) {
		EnforcePlayerOverlayNoShowing_Local();
	}

	if (!AppliedPlayer || !AppliedPlayer->HasAuthority()) return;

	if (CurrentTransformation.Duration > 0.f) {
		CurrentTransformation.RemainingDuration -= DeltaTime;
		if (CurrentTransformation.RemainingDuration < 0.f) {
			StopTransformation(true);
			return;
		}
	}

	if (CurrentTransformation.TransformationEffect) {
		CurrentTransformation.TransformationEffect->PersistEffect(AppliedPlayer, this, CurrentTransformation, DeltaTime);
	}

}

void UPlayerTransformationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPlayerTransformationComponent, CurrentTransformation);
}
//변신 데이터 획득 시 즉시 변신 상태 적용
void UPlayerTransformationComponent::OnRep_Transformation()
{
	CreateVisualMesh();

	if (CurrentTransformation.bActive) {
		ApplyTransformationVisual();

		if (CurrentTransformation.TransformPersistEffect.NiagaraEffect) {
			StartTransformLoopEffect_Local(CurrentTransformation.TransformPersistEffect);
		}
	}
	else {
		StopTransformLoopEffect_Local();
		ApplyNormalVisual();
		SetPlayerOverlayNoShowing_Local(false);
		SetComponentTickEnabled(false);
	}

	if (!AppliedPlayer) {
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}
	if (AppliedPlayer) {
		UPlayer_CharacterWidget* Player_CharacterWidget = Cast<UPlayer_CharacterWidget>(AppliedPlayer->WidgetComponent->GetUserWidgetObject());
		if (Player_CharacterWidget) {
			Player_CharacterWidget->SetUI();
		}
	}
}
void UPlayerTransformationComponent::Server_ResolveInputRule_Implementation(EPlayerInputResult Rule)
{
	if (!AppliedPlayer)
	{
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}

	if (!AppliedPlayer || !AppliedPlayer->HasAuthority())
	{
		return;
	}

	if (!CurrentTransformation.bActive)
	{
		return;
	}

	ResolveInputRule_Internal(Rule);
}

//변신 매쉬 초기화 및 설정
void UPlayerTransformationComponent::CreateVisualMesh()
{
	if (!AppliedPlayer) {
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}
	if (!AppliedPlayer) return;

	if (!TransformStaticMesh) {
		TransformStaticMesh = NewObject<UStaticMeshComponent>(AppliedPlayer, TEXT("TransformStaticMesh"));
		TransformStaticMesh->RegisterComponent();
		TransformStaticMesh->AttachToComponent(AppliedPlayer->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		TransformStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TransformStaticMesh->SetVisibility(false, true);
	}
	if (!TransformSkeletalMesh) {
		TransformSkeletalMesh = NewObject<USkeletalMeshComponent>(AppliedPlayer, TEXT("TransformSkeletalMesh"));
		TransformSkeletalMesh->RegisterComponent();
		TransformSkeletalMesh->AttachToComponent(AppliedPlayer->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		TransformSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TransformSkeletalMesh->SetVisibility(false, true);
	}
}
//플레이어 매쉬를 변신 매쉬로 적용
void UPlayerTransformationComponent::ApplyTransformationVisual()
{
	if (!AppliedPlayer) {
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}
	if (!AppliedPlayer) return;

	CreateVisualMesh();

	if (TransformStaticMesh) {
		TransformStaticMesh->SetStaticMesh(CurrentTransformation.TransformStaticMesh);
		TransformStaticMesh->SetRelativeScale3D(CurrentTransformation.MeshScale);
		TransformStaticMesh->SetVisibility(CurrentTransformation.TransformStaticMesh != nullptr, true);
	}

	if (TransformSkeletalMesh) {
		TransformSkeletalMesh->SetSkeletalMesh(CurrentTransformation.TransformSkeletalMesh);
		TransformSkeletalMesh->SetRelativeScale3D(CurrentTransformation.MeshScale);

		bool bUseSkeletalTransform = CurrentTransformation.TransformSkeletalMesh != nullptr;

		TransformSkeletalMesh->SetVisibility(bUseSkeletalTransform, true);

		if (bUseSkeletalTransform) {
			TransformSkeletalMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			if (CurrentTransformation.TransformAnimClass) {
				TransformSkeletalMesh->SetAnimInstanceClass(CurrentTransformation.TransformAnimClass);
			}
		}

		if (UPlayerTransformationAnimation* Anim = Cast<UPlayerTransformationAnimation>(TransformSkeletalMesh->GetAnimInstance())) {
			Anim->SetOwnerPlayer(AppliedPlayer);
			Anim->SetTransformationComponent(this);
		}
	}

	UpdateTransformMeshLocation();

	if (AppliedPlayer && AppliedPlayer->VisualManagerComp) {
		AppliedPlayer->VisualManagerComp->RefreshPortraitMaterials();
	}

	SetPlayerOverlayNoShowing_Local(true);
	SetComponentTickEnabled(true);
}
//플레이어 매쉬를 복원
void UPlayerTransformationComponent::ApplyNormalVisual()
{
	if (!AppliedPlayer) {
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}

	if (!AppliedPlayer) {
		return;
	}

	if (TransformStaticMesh) {
		TransformStaticMesh->SetVisibility(false, true);
		TransformStaticMesh->SetStaticMesh(nullptr);
	}

	if (TransformSkeletalMesh) {
		TransformSkeletalMesh->SetVisibility(false, true);
		TransformSkeletalMesh->SetAnimInstanceClass(nullptr);
		TransformSkeletalMesh->SetLeaderPoseComponent(nullptr);
		TransformSkeletalMesh->SetSkeletalMesh(nullptr);
	}

	if (AppliedPlayer->VisualManagerComp) {
		if (AppliedPlayer->NowWeapon) {
			AppliedPlayer->VisualManagerComp->Multi_RestoreActorVisuals(AppliedPlayer->NowWeapon);
		}
		if (AppliedPlayer->NowObjects) {
			AppliedPlayer->VisualManagerComp->Multi_RestoreActorVisuals(AppliedPlayer->NowObjects);
		}
		if (AppliedPlayer->NowSupport) {
			AppliedPlayer->VisualManagerComp->Multi_RestoreActorVisuals(AppliedPlayer->NowSupport);
		}

		AppliedPlayer->VisualManagerComp->Multi_RefreshVisuals();

		AppliedPlayer->VisualManagerComp->RefreshPortraitMaterials();
	}

}
//변신 매쉬 위치/회전값 갱신
void UPlayerTransformationComponent::UpdateTransformMeshLocation()
{
	if (!AppliedPlayer || (!TransformStaticMesh && !TransformSkeletalMesh)) {
		return;
	}

	FVector TargetLocation = AppliedPlayer->GetActorLocation() + CurrentTransformation.MeshOffset;
	FRotator TargetRotation = FRotator(0.f, AppliedPlayer->GetActorRotation().Yaw, 0.f) + CurrentTransformation.MeshRotationOffset;

	if (TransformStaticMesh) {
		TransformStaticMesh->SetWorldLocation(TargetLocation);
		TransformStaticMesh->SetWorldRotation(TargetRotation);
	}

	if (TransformSkeletalMesh) {
		TransformSkeletalMesh->SetWorldLocation(TargetLocation);
		TransformSkeletalMesh->SetWorldRotation(TargetRotation);
	}

}
bool UPlayerTransformationComponent::ResolveInputRule_Internal(EPlayerInputResult Rule)
{
	if (!CurrentTransformation.bActive) return true;
	switch (Rule)
	{
	case EPlayerInputResult::CanAction:
		return true;
	case EPlayerInputResult::CantAction:
		return false;
	case EPlayerInputResult::StopTransform:
		StopTransformation(true);
		return false;
	case EPlayerInputResult::StopTransformInst:
		ShortStopTransformation();
		return false;
	case EPlayerInputResult::StopTransformAndKeep:
		StopTransformation(true);
		return true;
	case EPlayerInputResult::Counting:
		CurrentTransformation.RemainingCount--;
		if (CurrentTransformation.RemainingCount <= 0)
		{
			StopTransformation(true);
		}
		return true;
	default:
		return true;
	}
}
//데이터에셋에서 변신 데이터 획득
void UPlayerTransformationComponent::CopyTransformationData(UPlayerTransformationDataAsset* DataAsset, FPlayerTransformation& NewData, APlayer_Character* UsedPlayer, float CustomDuration)
{
	if (!DataAsset) return;

	if (!AppliedPlayer) {
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}

	NewData.TransformationName = DataAsset->TransformationName;
	NewData.TransformSkeletalMesh = DataAsset->TransformSkeletalMesh;
	NewData.TransformAnimClass = DataAsset->TransformAnimClass;
	NewData.TransformStaticMesh = DataAsset->TransformStaticMesh;

	TSubclassOf<UPlayerTransformationEffect> EffectClass = DataAsset->TransformationEffect;
	if (!EffectClass && DataAsset->bUseVisualManager) {
		EffectClass = UPlayerTransformationEffect::StaticClass();
	}
	NewData.PlayerTransformationEffect = EffectClass;
	NewData.TransformationEffect = nullptr;

	if (AppliedPlayer && AppliedPlayer->HasAuthority() && EffectClass) {
		NewData.TransformationEffect = NewObject<UPlayerTransformationEffect>(this, EffectClass);
	}

	NewData.bUseVisualManager = DataAsset->bUseVisualManager;
	NewData.VisualEffectName = DataAsset->VisualEffectName;
	NewData.VisualData = DataAsset->VisualData;

	NewData.MeshOffset = DataAsset->MeshOffset;
	NewData.MeshRotationOffset = DataAsset->MeshRotationOffset;
	NewData.MeshScale = DataAsset->MeshScale;
	NewData.Duration = CustomDuration > 0.f ? CustomDuration : DataAsset->Duration;

	NewData.ShotTimeToStopTransformation = DataAsset->ShortTimeToStopTransformation;
	NewData.TotalCountToStopTransformation = DataAsset->TotalCountToStopTransformation;
	NewData.RemainingDuration = NewData.Duration;
	NewData.RemainingCount = DataAsset->TotalCountToStopTransformation;

	NewData.Priority = DataAsset->Priority;
	NewData.bExposureCharacterWidget = DataAsset->bExposureCharacterWidget;

	NewData.bHidePlayerEffectsFromOthers = DataAsset->bHidePlayerEffectsFromOthers;
	NewData.TransformStartEffect = DataAsset->TransformStartEffect;
	NewData.TransformPersistEffect = DataAsset->TransformPersistEffect;
	NewData.TransformEndEffect = DataAsset->TransformEndEffect;

	NewData.TransformationType = DataAsset->TransformationType;
	NewData.MoveRule = DataAsset->MoveRule;
	NewData.JumpRule = DataAsset->JumpRule;
	NewData.DodgeRule = DataAsset->DodgeRule;
	NewData.AttackRule = DataAsset->AttackRule;
	NewData.AimRule = DataAsset->AimRule;
	NewData.HittedRule = DataAsset->HittedRule;
	NewData.UseItemRule = DataAsset->UseItemRule;

	NewData.CausePlayer = UsedPlayer;
}
//변신 일시 중지 적용
void UPlayerTransformationComponent::ShortStopTransformation()
{
	if (!AppliedPlayer || !AppliedPlayer->HasAuthority()) return;
	if (!CurrentTransformation.bActive) return;

	ApplyNormalVisual();

	UPlayer_CharacterWidget* Player_CharacterWidget = Cast<UPlayer_CharacterWidget>(AppliedPlayer->WidgetComponent->GetUserWidgetObject());
	if (Player_CharacterWidget) {
		Player_CharacterWidget->SetUI();
	}

	GetWorld()->GetTimerManager().ClearTimer(ShortStopTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(ShortStopTimerHandle, this, &UPlayerTransformationComponent::RestoreShortStoppedTransformation, CurrentTransformation.ShotTimeToStopTransformation, false);
}
//일시 중지된 변신 복구
void UPlayerTransformationComponent::RestoreShortStoppedTransformation()
{
	if (!AppliedPlayer || !AppliedPlayer->HasAuthority()) return;
	if (!CurrentTransformation.bActive) return;

	ApplyTransformationVisual();

	UPlayer_CharacterWidget* Player_CharacterWidget = Cast<UPlayer_CharacterWidget>(AppliedPlayer->WidgetComponent->GetUserWidgetObject());
	if (Player_CharacterWidget) {
		Player_CharacterWidget->SetUI();
	}

	AppliedPlayer->ForceNetUpdate();
}
//변신 중 플레이어 Input에 대한 결과 반환
bool UPlayerTransformationComponent::ResolveInputRule(EPlayerInputResult Rule)
{
	if (!CurrentTransformation.bActive)
	{
		return true;
	}

	if (!AppliedPlayer)
	{
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}

	if (!AppliedPlayer)
	{
		return true;
	}

	// 서버라면 바로 실제 처리
	if (AppliedPlayer->HasAuthority())
	{
		return ResolveInputRule_Internal(Rule);
	}

	// 로컬 클라이언트라면 상태 변경이 필요한 규칙만 서버에 요청
	switch (Rule)
	{
	case EPlayerInputResult::StopTransform:
	case EPlayerInputResult::StopTransformInst:
	case EPlayerInputResult::StopTransformAndKeep:
	case EPlayerInputResult::Counting:
		Server_ResolveInputRule(Rule);
		break;

	default:
		break;
	}

	// 로컬에서는 입력을 계속할지 여부만 반환
	return GetInputContinueResult(Rule);
}

bool UPlayerTransformationComponent::GetInputContinueResult(EPlayerInputResult Rule)
{
	switch (Rule)
	{
	case EPlayerInputResult::CanAction:
		return true;

	case EPlayerInputResult::CantAction:
		return false;

	case EPlayerInputResult::StopTransform:
		return false;

	case EPlayerInputResult::StopTransformInst:
		return false;

	case EPlayerInputResult::StopTransformAndKeep:
		return true;

	case EPlayerInputResult::Counting:
		return true;

	default:
		return true;
	}
}

void UPlayerTransformationComponent::GetActiveTransformationMeshes(TArray<UMeshComponent*>& OutMeshes)
{
	if (TransformStaticMesh && TransformStaticMesh->IsVisible()) {
		OutMeshes.AddUnique(TransformStaticMesh);
	}

	if (TransformSkeletalMesh && TransformSkeletalMesh->IsVisible()) {
		OutMeshes.AddUnique(TransformSkeletalMesh);
	}
}

void UPlayerTransformationComponent::RemoveTransformationByRule(bool bUseEndEffect, int32 Priority, EPlayerTransformationType RemoveType)
{
	//Priority 기반 제거 (Priority가 5이면 무조건 제거)
	if (CurrentTransformation.Priority < Priority) {
		StopTransformation(bUseEndEffect);
	}
	//타입 기반 제거 (타입이 None이면 무조건 제거, Priority가 높으면 제거 X)
	if ((CurrentTransformation.TransformationType == RemoveType || RemoveType == EPlayerTransformationType::None) && CurrentTransformation.Priority < Priority) {
		StopTransformation(bUseEndEffect);
	}
}

//변신 시작 및 적용
bool UPlayerTransformationComponent::StartTransformation(UPlayerTransformationDataAsset* TransformationData, APlayer_Character* UsedPlayer, float CustomDuration)
{
	if (!AppliedPlayer)
	{
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}

	if (!AppliedPlayer || !AppliedPlayer->HasAuthority() || !TransformationData) {
		return false;
	}

	//새로 적용하려고 하는 변신이 기존에 적용 중인 변신보다 우선 순위가 낮은 경우 무시
	if (CurrentTransformation.bActive) {
		if (CurrentTransformation.Priority > TransformationData->Priority) return false;
	}

	if (AppliedPlayer->ConditionComp) {
		if (!AppliedPlayer->ConditionComp->CanTransformationGetVisualSlot(TransformationData->Priority)) {
			return false;
		}
	}

	//적용 확정된 변신 우선 순위가 기존 적용중인 변신보다 높은 경우 변신 교체(변신 끝 효과 x)
	if (CurrentTransformation.bActive) {
		StopTransformation(false, !TransformationData->bHidePlayerEffectsFromOthers);
	}

	if (AppliedPlayer->ConditionComp) {
		AppliedPlayer->ConditionComp->RemoveAnimationConditionsForTransformation(TransformationData->Priority, false);
	}

	FPlayerTransformation NewTransformation;
	CopyTransformationData(TransformationData, NewTransformation, UsedPlayer, CustomDuration);

	CurrentTransformation = NewTransformation;
	CurrentTransformation.bActive = true;

	if (CurrentTransformation.bHidePlayerEffectsFromOthers) AppliedPlayer->Multicast_RefreshPersistEffectVisibility();
	if (AppliedPlayer->EffectManagerComp) {
		FGameEffectContext Context;
		Context.SourceActor = AppliedPlayer;
		Context.SourceComponent = AppliedPlayer->GetMesh();
		Context.WorldLocation = AppliedPlayer->GetActorLocation();
		Context.WorldRotation = AppliedPlayer->GetActorRotation();

		AppliedPlayer->EffectManagerComp->PlayGameEffect_Multicast(CurrentTransformation.TransformStartEffect, Context);
	}
	if (AppliedPlayer->ConditionComp) {
		AppliedPlayer->ConditionComp->Multicast_RefreshConditionOverlayVisual();
	}

	if (CurrentTransformation.TransformPersistEffect.NiagaraEffect) Multicast_StartTransformLoopEffect(CurrentTransformation.TransformPersistEffect);

	//위젯 설정
	UPlayer_CharacterWidget* Player_CharacterWidget = Cast<UPlayer_CharacterWidget>(AppliedPlayer->WidgetComponent->GetUserWidgetObject());
	if (Player_CharacterWidget) {
		Player_CharacterWidget->SetUI();
	}

	CreateVisualMesh();
	ApplyTransformationVisual();

	bool bNeedStaticMesh = CurrentTransformation.TransformStaticMesh != nullptr;
	bool bNeedSkeletalMesh = CurrentTransformation.TransformSkeletalMesh != nullptr;
	bool bStaticOK = !bNeedStaticMesh || (TransformStaticMesh && TransformStaticMesh->GetStaticMesh() == CurrentTransformation.TransformStaticMesh && TransformStaticMesh->IsVisible());
	bool bSkeletalOK = !bNeedSkeletalMesh || (TransformSkeletalMesh && TransformSkeletalMesh->GetSkeletalMeshAsset() == CurrentTransformation.TransformSkeletalMesh && TransformSkeletalMesh->IsVisible());
	
	if (!bStaticOK || !bSkeletalOK) {
		ApplyNormalVisual();
		CurrentTransformation = FPlayerTransformation();
		CurrentTransformation.bActive = false;

		SetPlayerOverlayNoShowing_Local(false);

		SetComponentTickEnabled(false);
		AppliedPlayer->ForceNetUpdate();

		return false;
	}

	if (CurrentTransformation.TransformationEffect) {
		CurrentTransformation.TransformationEffect->StartEffect(AppliedPlayer, this, CurrentTransformation, UsedPlayer);
	}

	AppliedPlayer->SetReplicateMovement(true);
	AppliedPlayer->ForceNetUpdate();

	return true;
}
//변신 중지
void UPlayerTransformationComponent::StopTransformation(bool bEndEffect, bool bRefreshEffect)
{
	if (!AppliedPlayer) {
		AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	}

	if (!AppliedPlayer || !AppliedPlayer->HasAuthority()) return;
	if (!CurrentTransformation.bActive) return;

	GetWorld()->GetTimerManager().ClearTimer(ShortStopTimerHandle);

	if (IsValid(CurrentTransformation.TransformationEffect)) {
		CurrentTransformation.TransformationEffect->EndFunction(AppliedPlayer, this, CurrentTransformation, bEndEffect);
	}

	bool bHavingHidedEffect = CurrentTransformation.bHidePlayerEffectsFromOthers;

	Multicast_StopTransformLoopEffect();
	if (AppliedPlayer->EffectManagerComp) {
		FGameEffectContext Context;
		Context.SourceActor = AppliedPlayer;
		Context.SourceComponent = AppliedPlayer->GetRootComponent();
		Context.WorldLocation = AppliedPlayer->GetActorLocation();
		Context.WorldRotation = AppliedPlayer->GetActorRotation();

		AppliedPlayer->EffectManagerComp->PlayGameEffect_Multicast(CurrentTransformation.TransformEndEffect, Context);
	}

	ApplyNormalVisual();
	CurrentTransformation = FPlayerTransformation();
	CurrentTransformation.bActive = false;

	SetPlayerOverlayNoShowing_Local(false);
	
	if (AppliedPlayer->ConditionComp) {
		AppliedPlayer->ConditionComp->Multicast_RefreshConditionOverlayVisual();
	}

	//위젯 설정 복원
	UPlayer_CharacterWidget* Player_CharacterWidget = Cast<UPlayer_CharacterWidget>(AppliedPlayer->WidgetComponent->GetUserWidgetObject());
	if (Player_CharacterWidget) {
		Player_CharacterWidget->SetUI();
	}

	if (bHavingHidedEffect && bRefreshEffect) AppliedPlayer->Multicast_RefreshPersistEffectVisibility();

	SetComponentTickEnabled(false);
	AppliedPlayer->SetReplicateMovement(true);
	AppliedPlayer->ForceNetUpdate();
}

void UPlayerTransformationComponent::NotifyPlayerOverlayMaterialChanged()
{
	if (!bNoShowingPlayerOverlayMaterialDuringTransformation) return;

	EnforcePlayerOverlayNoShowing_Local();
}

void UPlayerTransformationComponent::NotifyHittedDuringTransformation(APlayer_Character* AttackedPlayer)
{
	ResolveInputRule(CurrentTransformation.HittedRule);
	if (CurrentTransformation.TransformationEffect) {
		CurrentTransformation.TransformationEffect->HittedEffect(AppliedPlayer, this, CurrentTransformation, AttackedPlayer);
	}
}

void UPlayerTransformationComponent::SetPlayerOverlayNoShowing_Local(bool bNoShowing)
{
	if (bNoShowingPlayerOverlayMaterialDuringTransformation == bNoShowing) {
		if(bNoShowing) EnforcePlayerOverlayNoShowing_Local();
		return;
	}
	
	bNoShowingPlayerOverlayMaterialDuringTransformation = bNoShowing;

	if (bNoShowing) EnforcePlayerOverlayNoShowing_Local();
	else RestorePlayerOverlayToShowing_Local();
}

void UPlayerTransformationComponent::EnforcePlayerOverlayNoShowing_Local()
{
	if (!AppliedPlayer) AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	if (!AppliedPlayer || !AppliedPlayer->GetMesh()) return;

	UMeshComponent* MeshComp = AppliedPlayer->GetMesh();

	UMaterialInstanceDynamic* OverlayMID = GetOrCreateSuppressibleOverlayMID(MeshComp);
	if (!OverlayMID) return;

	OverlayMID->SetScalarParameterValue(TEXT("Opacity"), 0.f);

	MeshComp->MarkRenderStateDirty();
}

void UPlayerTransformationComponent::RestorePlayerOverlayToShowing_Local()
{
	if (!AppliedPlayer) AppliedPlayer = Cast<APlayer_Character>(GetOwner());

	for (auto& Pair : NoShowingOverlayOriginalOpacityMap) {
		UMaterialInstanceDynamic* MID = Pair.Key.Get();

		if (!MID) continue;

		float OriginalOpacity = Pair.Value;
		MID->SetScalarParameterValue(TEXT("Opacity"), OriginalOpacity);
	}

	if (AppliedPlayer && AppliedPlayer->GetMesh()) AppliedPlayer->GetMesh()->MarkRenderStateDirty();

	NoShowingOverlayOriginalOpacityMap.Empty();
	OwnedNoShowingOverlayMIDs.Empty();
}

UMaterialInstanceDynamic* UPlayerTransformationComponent::GetOrCreateSuppressibleOverlayMID(UMeshComponent* MeshComp)
{
	if (!MeshComp) return nullptr;
	UMaterialInterface* CurrentOverlay = MeshComp->GetOverlayMaterial();

	if (!CurrentOverlay) return nullptr;

	if (UMaterialInstanceDynamic* CurrentMID = Cast<UMaterialInstanceDynamic>(CurrentOverlay)) {
		if (!NoShowingOverlayOriginalOpacityMap.Contains(CurrentMID)) {
			float OriginalOpacity = ReadOverlayOpacity(CurrentMID);
			NoShowingOverlayOriginalOpacityMap.Add(CurrentMID, OriginalOpacity);
		}

		return CurrentMID;
	}

	float OriginalOpacity = ReadOverlayOpacity(CurrentOverlay);
	UMaterialInstanceDynamic* NewMID = UMaterialInstanceDynamic::Create(CurrentOverlay, this);
	if (!NewMID) return nullptr;
	NewMID->SetScalarParameterValue(TEXT("Opacity"), OriginalOpacity);

	OwnedNoShowingOverlayMIDs.Add(NewMID);
	NoShowingOverlayOriginalOpacityMap.Add(NewMID, OriginalOpacity);

	MeshComp->SetOverlayMaterial(NewMID);
	MeshComp->MarkRenderStateDirty();

	return NewMID;
}

float UPlayerTransformationComponent::ReadOverlayOpacity(UMaterialInterface* Material)
{
	if (!Material) return 1.f;

	float Value = 1.f;
	Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Opacity")), Value);

	return Value;
}

bool UPlayerTransformationComponent::CheckHidePlayerEffectsFromOthers()
{
	return CurrentTransformation.bActive && CurrentTransformation.bHidePlayerEffectsFromOthers;
}

void UPlayerTransformationComponent::StartTransformLoopEffect_Local(const FGameEffectData& EffectData)
{
	if (!AppliedPlayer) AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	if (!AppliedPlayer || !EffectData.NiagaraEffect) return;

	bool bShouldShow = AppliedPlayer->ShouldShowGameEffectForThisClient(EffectData);

	if (!TransformEffectComp) {
		TransformEffectComp = NewObject<UNiagaraComponent>(AppliedPlayer, UNiagaraComponent::StaticClass(), TEXT("TransformLoopEffectComp"));

		if (!TransformEffectComp) return;

		TransformEffectComp->RegisterComponent();
		TransformEffectComp->AttachToComponent(AppliedPlayer->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	}

	TransformEffectComp->SetAsset(EffectData.NiagaraEffect);
	TransformEffectComp->SetRelativeLocation(EffectData.LocationOffset);
	TransformEffectComp->SetRelativeRotation(EffectData.RotationOffset);
	TransformEffectComp->SetRelativeScale3D(EffectData.Scale);

	TransformEffectComp->SetVisibility(bShouldShow, true);
	TransformEffectComp->SetHiddenInGame(!bShouldShow, true);

	if (bShouldShow) TransformEffectComp->Activate(true);
	else TransformEffectComp->Deactivate();
}

void UPlayerTransformationComponent::Multicast_StartTransformLoopEffect_Implementation(FGameEffectData EffectData)
{
	StartTransformLoopEffect_Local(EffectData);
}

void UPlayerTransformationComponent::StopTransformLoopEffect_Local()
{
	if (!TransformEffectComp) return;

	TransformEffectComp->Deactivate();
	TransformEffectComp->SetVisibility(false, true);
	TransformEffectComp->SetHiddenInGame(true, true);
}

void UPlayerTransformationComponent::Multicast_StopTransformLoopEffect_Implementation()
{
	StopTransformLoopEffect_Local();
}

void UPlayerTransformationComponent::RefreshTransformLoopEffectVisibility()
{
	if (!AppliedPlayer) AppliedPlayer = Cast<APlayer_Character>(GetOwner());
	if (!AppliedPlayer) return;
	if (!TransformEffectComp || !CurrentTransformation.bActive) return;

	FGameEffectData& EffectData = CurrentTransformation.TransformPersistEffect;
	bool bShouldShow = AppliedPlayer->ShouldShowGameEffectForThisClient(EffectData);

	TransformEffectComp->SetVisibility(bShouldShow, true);
	TransformEffectComp->SetHiddenInGame(!bShouldShow, true);

	if (bShouldShow) {
		if (!TransformEffectComp->IsActive()) TransformEffectComp->Activate(true);
	}
	else {
		TransformEffectComp->Deactivate();
	}
}

bool UPlayerTransformationComponent::CanMoveDuringTransfomation()
{
	return ResolveInputRule(CurrentTransformation.MoveRule);
}

bool UPlayerTransformationComponent::CanJumpDuringTransformation()
{
	return ResolveInputRule(CurrentTransformation.JumpRule);
}

bool UPlayerTransformationComponent::CanDodgeDuringTransformation()
{
	return ResolveInputRule(CurrentTransformation.DodgeRule);
}

bool UPlayerTransformationComponent::CanAimDuringTransformation()
{
	return ResolveInputRule(CurrentTransformation.AimRule);
}

bool UPlayerTransformationComponent::CanAttackDuringTransformation()
{
	return ResolveInputRule(CurrentTransformation.AttackRule);
}

bool UPlayerTransformationComponent::CanUseItemDuringTransformation()
{
	return ResolveInputRule(CurrentTransformation.UseItemRule);
}

bool UPlayerTransformationComponent::CanInteractionDuringTransformation()
{
	return ResolveInputRule(CurrentTransformation.InteractionRule);
}

