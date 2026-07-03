// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerConditionComponent.h"
#include "Player_Character.h"
#include "Effect/FGameEffectData.h"
#include "Effect/GameEffectManagerComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerTransformationComponent.h"
#include "PlayerConditionDataAsset.h"

// Sets default values for this component's properties
UPlayerConditionComponent::UPlayerConditionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UPlayerConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	Player = Cast<APlayer_Character>(GetOwner());
}

// Called every frame
void UPlayerConditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Player) return;
	if (!Player->HasAuthority()) return;

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; --i) {
		FPlayerCondition& Condition = CurrentConditions[i];
		bool bTimeOver = false;

		if (Condition.Duration > 0.f) {
			Condition.Duration -= DeltaTime;
			bTimeOver = Condition.Duration <= 0.f;
		}
		
		if (Condition.ConditionEffect && Condition.EffectInterval > 0.f) {
			Condition.NextEffectTimer -= DeltaTime;
			while (Condition.NextEffectTimer <= 0.f) {
				Condition.NextEffectTimer += Condition.EffectInterval;
				Condition.ConditionEffect->PersistEffect(Player, this, Condition, Condition.EffectInterval);

				if (Condition.EffectCount > 0) {
					Condition.RemainingTickCount--;

					if (Condition.RemainingTickCount <= 0) break;
				}
			}
		}

		bool bTickEffectEnd = (Condition.EffectInterval > 0.f && Condition.EffectCount >= 0 && Condition.RemainingTickCount <= 0);

		if (bTimeOver || bTickEffectEnd) {
			if (!CurrentConditions.IsValidIndex(i)) continue;

			FPlayerCondition RemovedCondition = CurrentConditions[i];
			FName RemovedConditionName = RemovedCondition.ConditionName;
			
			CurrentConditions.RemoveAt(i);
			bool bHasSameConditionAfterRemove = HasSameConditionByName(RemovedConditionName);

			if (RemovedCondition.ConditionEffect) RemovedCondition.ConditionEffect->EndFunction(Player, this, RemovedCondition, true);

			if (!bHasSameConditionAfterRemove) {
				PlayConditionOnceEffect(RemovedCondition.ConditionEndEffect, RemovedCondition);
				PlayConditionEndSound(RemovedCondition);

				Multicast_StopConditionLoopEffect(RemovedConditionName);
				Multicast_StopConditionOverlay(RemovedConditionName, 0.5f);

				Multicast_StopConditionPersistSound(RemovedConditionName, RemovedCondition.ConditionSoundFadeOutTime);
			}
		}
	}
}

//상태 이상 적용 (추가/갱신)
void UPlayerConditionComponent::ApplyCondition(UPlayerConditionDataAsset* ConditionData, APlayer_Character* CausePlayer, float CustomDuration, bool bUseCustomValue, float CustomValue) {
	if (!Player || !Player->HasAuthority() || !ConditionData) return;
	if (IgnoreDebuff(ConditionData)) return;

	//중첩 가능한 Condition의 경우 적용 전 Interval 검사
	if (ConditionData->bCanMultiApply && ConditionData->MultiApplyInterval > 0.f && CheckCondition(ConditionData->ConditionName)) {
		UWorld* World = GetWorld();
		if (!World) return;

		float CurrentTime = World->GetTimeSeconds();
		
		if (float* NextApplyTime = MultiApplyIntervalMap.Find(ConditionData->ConditionName)) {
			if (CurrentTime < *NextApplyTime) return;
		}
	}
	
	//중첩 불가능한 Condition의 경우 갱신(기존 동일한 Condition 제거)
	if (!ConditionData->bCanMultiApply && CheckCondition(ConditionData->ConditionName)) {
		RemoveCondition(ConditionData->ConditionName);
	}
	
	//현재 같은 이름의 Condition이 여러개 있는지 확인 (남은 중첩이 있는지 확인)
	bool bAlreadyHadSameCondition = HasSameConditionByName(ConditionData->ConditionName);

	//중첩 가능한 Condition 이거나 플레이어에게 적용되지 않은 Condition인 경우
	//현재 플레이어 상태이상 배열에 추가
	FPlayerCondition NewCondition;
	NewCondition.ConditionName = ConditionData->ConditionName;
	NewCondition.ConditionType = ConditionData->ConditionType;
	//Condition을 적용할 때 Custom Duration을 사용한다면 그 Duration을 적용
	if (CustomDuration <= 0.f) {
		NewCondition.Duration = ConditionData->Duration;
	}
	else {
		NewCondition.Duration = CustomDuration;
	}
	//Condition을 적용할 때 Custom Value을 사용한다면 그 Value를 효과량에 적용
	if (bUseCustomValue) {
		NewCondition.EffectValue = CustomValue;
	}
	else {
		NewCondition.EffectValue = ConditionData->EffectValue;
	}
	NewCondition.HelthChange = ConditionData->HelthChange;
	NewCondition.EffectCount = ConditionData->EffectCount;
	NewCondition.EffectInterval = ConditionData->EffectInterval;
	NewCondition.bCanMultiApply = ConditionData->bCanMultiApply;
	NewCondition.Priority = ConditionData->Priority;
	NewCondition.bCanRemove = ConditionData->bCanRemove;
	NewCondition.CausePlayer = CausePlayer;

	NewCondition.ConditionAnimation = ConditionData->ConditionAnimation;
	NewCondition.ConditionMontage = ConditionData->ConditionMontage;
	
	NewCondition.JumpRule = ConditionData->JumpRule;
	NewCondition.AttackRule = ConditionData->AttackRule;
	NewCondition.DodgeRule = ConditionData->DodgeRule;
	NewCondition.HitRule = ConditionData->HitRule;
	NewCondition.BigHitRule = ConditionData->BigHitRule;

	NewCondition.ConditionStartEffect = ConditionData->ConditionStartEffect;
	NewCondition.ConditionPersistEffect = ConditionData->ConditionPersistEffect;
	NewCondition.ConditionEndEffect = ConditionData->ConditionEndEffect;

	NewCondition.ConditionStartSound = ConditionData->ConditionStartSound;
	NewCondition.ConditionPersistSound = ConditionData->ConditionPersistSound;
	NewCondition.ConditionEndSound = ConditionData->ConditionEndSound;
	NewCondition.ConditionSoundFadeOutTime = ConditionData->ConditionSoundFadeOutTime;
	
	NewCondition.ConditionOverlayMaterial = ConditionData->ConditionOverlayMaterial;

	NewCondition.NextEffectTimer = ConditionData->EffectInterval;
	NewCondition.RemainingTickCount = ConditionData->EffectCount;

	if (ConditionData->ConditionEffect) {
		NewCondition.ConditionEffect = NewObject<UPlayerConditionEffect>(this, ConditionData->ConditionEffect);
	}

	if (!TryGetVisualSlotForCondition(NewCondition, false)) return;

	if (NewCondition.ConditionEffect) {
		NewCondition.ConditionEffect->StartEffect(Player, this, NewCondition, CausePlayer);
	}

	PlayConditionStartSound(NewCondition);
	PlayConditionOnceEffect(NewCondition.ConditionStartEffect, NewCondition);

	CurrentConditions.Add(NewCondition);
	
	if (!bAlreadyHadSameCondition) {
		if (NewCondition.ConditionPersistSound) {
			Multicast_StartConditionPersistSound(NewCondition.ConditionName, NewCondition.ConditionPersistSound);
		}

		if (NewCondition.ConditionPersistEffect.NiagaraEffect) {
			FGameEffectRuntimeParams RuntimeParams;

			FGameEffectFloatParam LifeTimeParam;
			LifeTimeParam.ParamName = TEXT("User.LifeTime");
			LifeTimeParam.Value = NewCondition.Duration;

			RuntimeParams.FloatParams.Add(LifeTimeParam);

			Multicast_StartConditionLoopEffect(NewCondition.ConditionName, NewCondition.ConditionPersistEffect, RuntimeParams);
		}
		if (NewCondition.ConditionOverlayMaterial) {
			Multicast_StartConditionOverlay(NewCondition.ConditionName, NewCondition.ConditionOverlayMaterial, TEXT("Opacity"), NewCondition.Priority);
		}
	}
	
	if (ConditionData->bCanMultiApply && ConditionData->MultiApplyInterval > 0.f) {
		if (UWorld* World = GetWorld()) {
			MultiApplyIntervalMap.Add(ConditionData->ConditionName, World->GetTimeSeconds() + ConditionData->MultiApplyInterval);
		}
	}
} 

//특정 Condition을 제거 (같은 Condition이 여러개라면 제일 앞 제거)
void UPlayerConditionComponent::RemoveCondition(FName TargetConditionName) {
	for (int32 i = 0; i < CurrentConditions.Num(); i++) {
		if (CurrentConditions[i].ConditionName == TargetConditionName) {
			bool bHasSameConditionAfterRemove = HasSameConditionExceptIndex(TargetConditionName, i);
			if (!bHasSameConditionAfterRemove) {
				Multicast_StopConditionLoopEffect(TargetConditionName);
				Multicast_StopConditionOverlay(TargetConditionName, 0.5f);
				Multicast_StopConditionPersistSound(TargetConditionName, CurrentConditions[i].ConditionSoundFadeOutTime);
			}

			if (CurrentConditions[i].ConditionEffect) {
				CurrentConditions[i].ConditionEffect->EndFunction(Player, this, CurrentConditions[i], false);
			}
			CurrentConditions.RemoveAt(i);
			return;
		}
	}

	UPlayerTransformationComponent* TransformationComp = Player->TransformationComp;
	if (TransformationComp && TransformationComp->CurrentTransformation.TransformationName == TargetConditionName) {
		TransformationComp->StopTransformation(false);
	}
}

//특정 Condition을 모두 제거
void UPlayerConditionComponent::RemoveSameNameCondition(FName TargetConditionName, bool bEndEffect) {
	bool bRemovedAny = false;
	FPlayerCondition LastRemovedCondition;

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; --i) {
		if (CurrentConditions[i].ConditionName != TargetConditionName) continue;

		LastRemovedCondition = CurrentConditions[i];
		if (CurrentConditions[i].ConditionEffect) {
			CurrentConditions[i].ConditionEffect->EndFunction(Player, this, CurrentConditions[i], bEndEffect);
		}

		CurrentConditions.RemoveAt(i);
		bRemovedAny = true;
	}

	if (bRemovedAny) {
		Multicast_StopConditionLoopEffect(TargetConditionName);
		Multicast_StopConditionOverlay(TargetConditionName, 0.5f);
		Multicast_StopConditionPersistSound(TargetConditionName, LastRemovedCondition.ConditionSoundFadeOutTime);
		if (bEndEffect) {
			PlayConditionOnceEffect(LastRemovedCondition.ConditionEndEffect, LastRemovedCondition);
			PlayConditionEndSound(LastRemovedCondition);
		}
	}

	UPlayerTransformationComponent* TransformationComp = Player->TransformationComp;
	if (TransformationComp && TransformationComp->CurrentTransformation.TransformationName == TargetConditionName) TransformationComp->StopTransformation(false);
}

//특정 Condition Type을 전부 제거 (우선 순위 확인)
void UPlayerConditionComponent::RemoveSameCategoryCondition(EPlayerConditionType TargetConditionType, bool bEndEffect, int32 priority) {
	TSet<FName> RemovedConditionNames;
	TMap<FName, FPlayerCondition> LastRemovedByName;

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; i--) {
		if (CurrentConditions[i].ConditionType == TargetConditionType && CurrentConditions[i].Priority < priority) {
			if (CurrentConditions[i].ConditionEffect) {
				CurrentConditions[i].ConditionEffect->EndFunction(Player, this, CurrentConditions[i], bEndEffect);
			}
			RemovedConditionNames.Add(CurrentConditions[i].ConditionName);
			LastRemovedByName.Add(CurrentConditions[i].ConditionName, CurrentConditions[i]);
			CurrentConditions.RemoveAt(i);
		}
	}
	UPlayerTransformationComponent* TransformationComp = Player->TransformationComp;
	if (TransformationComp) {
		TryRemoveTransform(5, TargetConditionType, false);
	}

	for (const FName& RemovedName : RemovedConditionNames) {
		if (!HasSameConditionByName(RemovedName)) {
			Multicast_StopConditionLoopEffect(RemovedName);
			Multicast_StopConditionOverlay(RemovedName, 0.5f);
			
			if (FPlayerCondition* RemovedCondition = LastRemovedByName.Find(RemovedName)) {
				Multicast_StopConditionPersistSound(RemovedName, RemovedCondition->ConditionSoundFadeOutTime);
			}

			if (bEndEffect) {
				if (FPlayerCondition* RemovedCondition = LastRemovedByName.Find(RemovedName)) {
					PlayConditionOnceEffect(RemovedCondition->ConditionEndEffect, *RemovedCondition);
					PlayConditionEndSound(*RemovedCondition);
				}
			}
		}
	}
}

//우선순위보다 낮은 Condition을 전부 제거 (Priority가 5면 모두 제거)
void UPlayerConditionComponent::RemoveLowPriorityCondition(int32 Priority, bool bEndEffect) {
	TSet<FName> RemovedConditionNames;
	TMap<FName, FPlayerCondition> LastRemovedByName;

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; i--) {
		if (CurrentConditions[i].Priority < Priority) {
			if (CurrentConditions[i].ConditionEffect) {
				CurrentConditions[i].ConditionEffect->EndFunction(Player, this, CurrentConditions[i], bEndEffect);
			}
			RemovedConditionNames.Add(CurrentConditions[i].ConditionName);
			LastRemovedByName.Add(CurrentConditions[i].ConditionName, CurrentConditions[i]);
			CurrentConditions.RemoveAt(i);
		}
	}
	UPlayerTransformationComponent* TransformationComp = Player->TransformationComp;
	if (TransformationComp) {
		TryRemoveTransform(Priority, EPlayerConditionType::None, false);
	}

	for (const FName& RemovedName : RemovedConditionNames) {
		if (!HasSameConditionByName(RemovedName)) {
			Multicast_StopConditionLoopEffect(RemovedName);
			Multicast_StopConditionOverlay(RemovedName, 0.5f);
			if (FPlayerCondition* RemovedCondition = LastRemovedByName.Find(RemovedName)) {
				Multicast_StopConditionPersistSound(RemovedName, RemovedCondition->ConditionSoundFadeOutTime);
				if (bEndEffect) {
					PlayConditionOnceEffect(RemovedCondition->ConditionEndEffect, *RemovedCondition);
					PlayConditionEndSound(*RemovedCondition);
				}
			}
			
		}
	}
}

//특정 Condition이 있는지 체크
bool UPlayerConditionComponent::CheckCondition(FName TargetConditionName) {
	if (TargetConditionName.IsNone()) return false;
	if (!IsValid(Player)) Player = Cast<APlayer_Character>(GetOwner());
	if (!IsValid(Player)) return false;

	for (FPlayerCondition& Conditions : CurrentConditions) {
		if (Conditions.ConditionName == TargetConditionName) {
			return true;
		}
	}
	UPlayerTransformationComponent* TransformationComp = Player->TransformationComp;
	if (TransformationComp && TransformationComp->CurrentTransformation.TransformationName == TargetConditionName) {
		return true;
	}
	return false;
}

//디버프 무시 상태를 판별
bool UPlayerConditionComponent::IgnoreDebuff(UPlayerConditionDataAsset* ConditionData) const
{
	if (ConditionData->ConditionType != EPlayerConditionType::DeBuff) return false;

	static const FName InvincibilityName(TEXT("Invincible"));
	static const FName DebuffImmunityName(TEXT("NoDebuff"));

	for (const FPlayerCondition& ActiveCondition : CurrentConditions) {
		bool bIsImmuneBuff = (ActiveCondition.ConditionName == InvincibilityName || ActiveCondition.ConditionName == DebuffImmunityName);

		if (bIsImmuneBuff) {
			if (ActiveCondition.Priority >= ConditionData->Priority) return true;
		}
	}

	return false;
}

void UPlayerConditionComponent::HandleConditionEvent(EPlayerConditionEvent Event, bool bUseEndEffect)
{
	if (!Player) return;
	if (!Player->HasAuthority()) return;

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; --i) {
		FPlayerCondition& Condition = CurrentConditions[i];
		EConditionEventRule Rule = Condition.GetRuleByEvent(Event);

		if (Rule == EConditionEventRule::Keep) continue;
		if (Rule == EConditionEventRule::Remove) {
			if (!Condition.bCanRemove) continue;
			FName RemovedConditionName = Condition.ConditionName;
			bool bHasSameConditionAfterRemove = HasSameConditionExceptIndex(RemovedConditionName, i);
			if (!bHasSameConditionAfterRemove) {
				Multicast_StopConditionLoopEffect(RemovedConditionName);
				Multicast_StopConditionOverlay(RemovedConditionName, 0.5f);
				Multicast_StopConditionPersistSound(RemovedConditionName, Condition.ConditionSoundFadeOutTime);
			}
			
			if (Condition.ConditionEffect) {
				Condition.ConditionEffect->EndFunction(Player, this, Condition, bUseEndEffect);
			}
			if (!bHasSameConditionAfterRemove && bUseEndEffect) {
				PlayConditionOnceEffect(Condition.ConditionEndEffect, Condition);
				PlayConditionEndSound(Condition);
			}

			CurrentConditions.RemoveAt(i);
		}
	}
}

void UPlayerConditionComponent::StartConditionLoopEffect_Local(FName ConditionName, const FGameEffectData& EffectData, const FGameEffectRuntimeParams& RuntimeParams)
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());

	if (!Player) return;
	if (ConditionName.IsNone()) return;
	if (!EffectData.NiagaraEffect) return;

	//중첩 Condition도 이펙트를 중첩하지 않음
	if (ActiveConditionPersistEffects.Contains(ConditionName)) return;
	
	bool bShouldShow = Player->ShouldShowGameEffectForThisClient(EffectData);
	FActiveConditionPersistEffect& ActiveEffect = ActiveConditionPersistEffects.Add(ConditionName);

	ActiveEffect.EffectData = EffectData;
	ActiveEffect.NiagaraComp = NewObject<UNiagaraComponent>(Player, UNiagaraComponent::StaticClass());
	if (!ActiveEffect.NiagaraComp) {
		ActiveConditionPersistEffects.Remove(ConditionName);
		return;
	}

	ActiveEffect.NiagaraComp->RegisterComponent();
	ActiveEffect.NiagaraComp->AttachToComponent(Player->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);

	ActiveEffect.NiagaraComp->SetAsset(EffectData.NiagaraEffect);
	ActiveEffect.NiagaraComp->SetRelativeLocation(EffectData.LocationOffset);
	ActiveEffect.NiagaraComp->SetRelativeRotation(EffectData.RotationOffset);
	ActiveEffect.NiagaraComp->SetRelativeScale3D(EffectData.Scale);
	UNiagaraFunctionLibrary::OverrideSystemUserVariableSkeletalMeshComponent(ActiveEffect.NiagaraComp, TEXT("User.PlayerMesh"), Player->GetMesh());

	ApplyRuntimeParams(ActiveEffect.NiagaraComp, RuntimeParams);

	ActiveEffect.NiagaraComp->SetVisibility(bShouldShow, true);
	ActiveEffect.NiagaraComp->SetHiddenInGame(!bShouldShow, true);

	if (bShouldShow) ActiveEffect.NiagaraComp->Activate(true);
	else ActiveEffect.NiagaraComp->Deactivate();
}

void UPlayerConditionComponent::StopConditionLoopEffect_Local(FName ConditionName)
{
	if (ConditionName.IsNone()) return;

	FActiveConditionPersistEffect* ActiveEffect = ActiveConditionPersistEffects.Find(ConditionName);

	if (!ActiveEffect) return;
	if (ActiveEffect->NiagaraComp) {
		UNiagaraComponent* NiagaraComp = ActiveEffect->NiagaraComp;
		NiagaraComp->Deactivate();

		FTimerHandle DestroyHandle;
		if (UWorld* World = GetWorld()) {
			World->GetTimerManager().SetTimer(DestroyHandle, FTimerDelegate::CreateWeakLambda(this, [NiagaraComp]() {
				if (IsValid(NiagaraComp)) NiagaraComp->DestroyComponent();
			}),1.f, false);
		}
		else {
			ActiveEffect->NiagaraComp->DestroyComponent();
		}
	}

	ActiveConditionPersistEffects.Remove(ConditionName);
}

void UPlayerConditionComponent::ApplyRuntimeParams(UNiagaraComponent* NiagaraComp, const FGameEffectRuntimeParams& RuntimeParams)
{
	if (!NiagaraComp) return;

	for (const FGameEffectFloatParam& Param : RuntimeParams.FloatParams) {
		if (!Param.ParamName.IsNone()) NiagaraComp->SetVariableFloat(Param.ParamName, Param.Value);
	}
	for (const FGameEffectVectorParam& Param : RuntimeParams.VectorParams) {
		if (!Param.ParamName.IsNone()) NiagaraComp->SetVariableVec3(Param.ParamName, Param.Value);
	}
	for (const FGameEffectBoolParam& Param : RuntimeParams.BoolParams) {
		if (!Param.ParamName.IsNone()) NiagaraComp->SetVariableBool(Param.ParamName, Param.Value);
	}
}

bool UPlayerConditionComponent::HasSameConditionExceptIndex(FName ConditionName, int32 ExceptIndex)
{
	if (ConditionName.IsNone()) return false;
	for (int32 i = 0; i < CurrentConditions.Num(); ++i) {
		if (i == ExceptIndex) continue;
		if (CurrentConditions[i].ConditionName == ConditionName) return true;
	}

	return false;
}

bool UPlayerConditionComponent::HasSameConditionByName(FName ConditionName)
{
	if (ConditionName.IsNone()) return false;
	for (int32 i = 0; i < CurrentConditions.Num(); ++i) {
		if (CurrentConditions[i].ConditionName == ConditionName) return true;
	}

	return false;
}

void UPlayerConditionComponent::RefreshConditionLoopEffectVisibility()
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player) return;

	for (auto& Pair : ActiveConditionPersistEffects) {
		FActiveConditionPersistEffect& ActiveEffect = Pair.Value;

		if (!ActiveEffect.NiagaraComp) continue;

		bool bShouldShow = Player->ShouldShowGameEffectForThisClient(ActiveEffect.EffectData);

		ActiveEffect.NiagaraComp->SetVisibility(bShouldShow, true);
		ActiveEffect.NiagaraComp->SetHiddenInGame(!bShouldShow, true);

		if (bShouldShow) {
			if (!ActiveEffect.NiagaraComp->IsActive()) ActiveEffect.NiagaraComp->Activate(true);
		}
		else ActiveEffect.NiagaraComp->Deactivate();
	}
}

void UPlayerConditionComponent::StartConditionOverlay_Local(FName ConditionName, UMaterialInterface* OverlayMaterial, FName OpacityParamName, int32 Priority)
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player || !Player->GetMesh()) return;
	if (ConditionName.IsNone() || !OverlayMaterial) return;

	UWorld* World = GetWorld();
	if (World) World->GetTimerManager().ClearTimer(OverlayFadeTimerHandle);

	USkeletalMeshComponent* MeshComp = Player->GetMesh();

	if (ActiveConditionOverlays.Num() == 0 && CurrentOverlayConditionName.IsNone()) {
		DefaultOverlayMaterial = MeshComp->GetOverlayMaterial();
	}

	UMaterialInstanceDynamic* OverlayMID = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
	if (!OverlayMID) return;

	OverlayMID->SetScalarParameterValue(OpacityParamName, 1.f);

	FActiveConditionOverlayEffect& ActiveOverlay = ActiveConditionOverlays.FindOrAdd(ConditionName);

	ActiveOverlay.OverlayMID = OverlayMID;
	ActiveOverlay.SourceMaterial = OverlayMaterial;
	ActiveOverlay.Priority = Priority;

	ApplyHighestConditionOverlay_Local();
}

void UPlayerConditionComponent::StopConditionOverlay_Local(FName ConditionName, float FadeOutTime)
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player || !Player->GetMesh()) return;
	if (ConditionName.IsNone()) return;

	FActiveConditionOverlayEffect* ActiveOverlay = ActiveConditionOverlays.Find(ConditionName);
	if (!ActiveOverlay) return;

	if (CurrentOverlayConditionName != ConditionName) {
		ActiveConditionOverlays.Remove(ConditionName);
		return;
	}

	UWorld* World = GetWorld();
	if (!World) {
		ActiveConditionOverlays.Remove(ConditionName);
		ApplyHighestConditionOverlay_Local();
		return;
	}

	World->GetTimerManager().ClearTimer(OverlayFadeTimerHandle);

	TWeakObjectPtr<UMaterialInstanceDynamic> FadingMID = ActiveOverlay->OverlayMID;
	FName OpacityParamName = TEXT("Opacity");
	float SafeFadeTime = FMath::Max(0.1f, FadeOutTime);

	ActiveConditionOverlays.Remove(ConditionName);

	TSharedRef<float, ESPMode::ThreadSafe> Elapsed = MakeShared<float, ESPMode::ThreadSafe>(0.f);

	World->GetTimerManager().SetTimer(OverlayFadeTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, ConditionName, FadingMID, OpacityParamName, SafeFadeTime, Elapsed]() {
		UWorld* World = GetWorld();
		if (!World) return;

		*Elapsed += World->GetDeltaSeconds();

		float Alpha = FMath::Clamp(*Elapsed / SafeFadeTime, 0.f, 1.f);
		float Opacity = 1.f - Alpha;

		if (FadingMID.IsValid()) {
			FadingMID->SetScalarParameterValue(OpacityParamName, Opacity);
		}
		if (Alpha >= 1.f) {
			World->GetTimerManager().ClearTimer(OverlayFadeTimerHandle);
			ApplyHighestConditionOverlay_Local();
		}
	}), 0.015f, true);
}

void UPlayerConditionComponent::RefreshConditionOverlay_Local()
{
	ApplyHighestConditionOverlay_Local();
}

void UPlayerConditionComponent::ApplyHighestConditionOverlay_Local()
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player || !Player->GetMesh()) return;

	USkeletalMeshComponent* MeshComp = Player->GetMesh();

	if (ShouldNoShowingConditionOverlayByTransformation_Local()) {
		if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(OverlayFadeTimerHandle);
		CurrentOverlayConditionName = NAME_None;

		MeshComp->SetOverlayMaterial(DefaultOverlayMaterial);
		MeshComp->MarkRenderStateDirty();

		NotifyTransformationOverlayChanged_Local();
		return;
	}

	FName BestName;
	FActiveConditionOverlayEffect* BestOverlay = nullptr;

	for (auto& Pair : ActiveConditionOverlays) {
		FActiveConditionOverlayEffect& Overlay = Pair.Value;
		if (!Overlay.OverlayMID) continue;

		if (!BestOverlay || Overlay.Priority > BestOverlay->Priority) {
			BestName = Pair.Key;
			BestOverlay = &Overlay;
		}
	}

	if (BestOverlay && BestOverlay->OverlayMID) {
		CurrentOverlayConditionName = BestName;
		BestOverlay->OverlayMID->SetScalarParameterValue(TEXT("Opacity"), 1.f);

		MeshComp->SetOverlayMaterial(BestOverlay->OverlayMID);
		MeshComp->MarkRenderStateDirty();
		NotifyTransformationOverlayChanged_Local();
	}
	else {
		CurrentOverlayConditionName = NAME_None;
		MeshComp->SetOverlayMaterial(DefaultOverlayMaterial);
		MeshComp->MarkRenderStateDirty();
		NotifyTransformationOverlayChanged_Local();
	}
}

void UPlayerConditionComponent::NotifyTransformationOverlayChanged_Local()
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player || !Player->TransformationComp) return;

	Player->TransformationComp->NotifyPlayerOverlayMaterialChanged();
}

bool UPlayerConditionComponent::ShouldNoShowingConditionOverlayByTransformation_Local()
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player || !Player->TransformationComp) return false;

	auto& CurrentTransformation = Player->TransformationComp->CurrentTransformation;

	if (!CurrentTransformation.bActive) return false;
	if (!CurrentTransformation.bHidePlayerEffectsFromOthers) return false;

	return !Player->IsLocallyControlled();
}

void UPlayerConditionComponent::PlayConditionStartSound(const FPlayerCondition& Condition)
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player) return;
	if (!Player->HasAuthority()) return;
	if (!Condition.ConditionStartSound) return;

	Multicast_PlayConditionOnceSound(Condition.ConditionStartSound);
}
void UPlayerConditionComponent::PlayConditionEndSound(const FPlayerCondition& Condition)
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player) return;
	if (!Player->HasAuthority()) return;
	if (!Condition.ConditionEndSound) return;

	Multicast_PlayConditionOnceSound(Condition.ConditionEndSound);
}

void UPlayerConditionComponent::StartConditionPersistSound_Local(FName ConditionName, USoundBase* Sound)
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player) return;
	if (ConditionName.IsNone()) return;
	if (!Sound) return;

	if (ActiveConditionPersistSounds.Contains(ConditionName)) return;

	USceneComponent* AttachComp = Player->GetRootComponent();
	if (!AttachComp) return;

	UAudioComponent* AudioComp = UGameplayStatics::SpawnSoundAttached(Sound, AttachComp, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);

	if (!AudioComp) return;

	ActiveConditionPersistSounds.Add(ConditionName, AudioComp);
}

void UPlayerConditionComponent::StopConditionPersistSound_Local(FName ConditionName, float FadeOutTime)
{
	if (ConditionName.IsNone()) return;

	TObjectPtr<UAudioComponent>* FoundAudioComp = ActiveConditionPersistSounds.Find(ConditionName);
	if (!FoundAudioComp || !FoundAudioComp->Get()) return;

	UAudioComponent* AudioComp = FoundAudioComp->Get();
	
	if (FadeOutTime > 0.f) {
		AudioComp->FadeOut(FadeOutTime, 0.f);

		FTimerHandle DestroyHandle; 
		if (UWorld* World = GetWorld()) {
			World->GetTimerManager().SetTimer(DestroyHandle, FTimerDelegate::CreateWeakLambda(this, [AudioComp]() {
				if (IsValid(AudioComp)) AudioComp->DestroyComponent();
			}), FadeOutTime + 0.05f, false);
		}
	}
	else {
		AudioComp->Stop();
		AudioComp->DestroyComponent();
	}

	ActiveConditionPersistSounds.Remove(ConditionName);
}

void UPlayerConditionComponent::PlayConditionOnceEffect(const FGameEffectData& EffectData, const FPlayerCondition& Condition)
{
	if (!Player) return;
	if (!Player->HasAuthority()) return;
	if (!Player->EffectManagerComp) return;

	if (!EffectData.NiagaraEffect && !EffectData.Sound) return;

	FGameEffectContext Context;
	Context.SourceActor = Player;
	Context.SourceComponent = Player->GetMesh();
	Context.WorldLocation = Player->GetActorLocation();
	Context.WorldRotation = Player->GetActorRotation();
	
	Player->EffectManagerComp->PlayGameEffect_Multicast(EffectData, Context);
}

void UPlayerConditionComponent::TryRemoveTransform(int32 Priority, EPlayerConditionType Type, bool bEndEffect) {
	if (!Player) return;

	if (UPlayerTransformationComponent* TransformationComp = Player->TransformationComp) {
		if (Type == EPlayerConditionType::Buff) {
			TransformationComp->RemoveTransformationByRule(false, Priority, EPlayerTransformationType::Buff);
		}
		else if (Type == EPlayerConditionType::DeBuff) {
			TransformationComp->RemoveTransformationByRule(false, Priority, EPlayerTransformationType::Debuff);
		}
		else if (Type == EPlayerConditionType::Complex) {
			TransformationComp->RemoveTransformationByRule(false, Priority, EPlayerTransformationType::Complex);
		}
		else {
			TransformationComp->RemoveTransformationByRule(false, Priority, EPlayerTransformationType::None);
		}
	}
}

bool UPlayerConditionComponent::TryGetVisualSlotForCondition(const FPlayerCondition& NewCondition, bool bEndEffect)
{
	if (!Player) return false;
	if (!NewCondition.HasConditionAnimation()) {
		return true;
	}

	//현재 적용된 Transformation의 Priority가 높다면 적용 X
	if (UPlayerTransformationComponent* TransformationComp = Player->TransformationComp) {
		if (TransformationComp->CurrentTransformation.bActive) {
			const int32 TransformPriority = TransformationComp->CurrentTransformation.Priority;

			if (TransformPriority > NewCondition.Priority) return false;
		}
	}

	//지금 적용중인 Condition에 Animation이 있고 그 Condition이 Priority가 높다면 적용 X 
	for (const FPlayerCondition& condition : CurrentConditions) {
		if (!condition.HasConditionAnimation()) continue;

		if (condition.Priority > NewCondition.Priority) {
			return false;
		}
	}

	if (UPlayerTransformationComponent* TransformationComp = Player->TransformationComp) {
		if (TransformationComp->CurrentTransformation.bActive) {
			TransformationComp->StopTransformation(false);
		}
	}

	TSet<FName> RemovedConditionNames;
	TMap<FName, FPlayerCondition> LastRemovedConditions;

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; --i) {
		FPlayerCondition& Condition = CurrentConditions[i];
		if (!Condition.HasConditionAnimation()) continue;
		if (Condition.Priority <= NewCondition.Priority){
			if (Condition.ConditionEffect){
				Condition.ConditionEffect->EndFunction(Player, this, Condition, bEndEffect);
			}
			RemovedConditionNames.Add(Condition.ConditionName);
			LastRemovedConditions.Add(Condition.ConditionName, Condition);

			CurrentConditions.RemoveAt(i);
		}
	}
	for (const FName& RemovedName : RemovedConditionNames) {
		if (!HasSameConditionByName(RemovedName)) {
			Multicast_StopConditionLoopEffect(RemovedName);
			Multicast_StopConditionOverlay(RemovedName, 0.5f);
			if (FPlayerCondition* RemovedCondition = LastRemovedConditions.Find(RemovedName)) {
				Multicast_StopConditionPersistSound(RemovedName, RemovedCondition->ConditionSoundFadeOutTime);
				if (bEndEffect) {
					PlayConditionOnceEffect(RemovedCondition->ConditionEndEffect, *RemovedCondition);
					PlayConditionEndSound(*RemovedCondition);
				}
			}
		}
	}

	return true;
}

bool UPlayerConditionComponent::CanTransformationGetVisualSlot(int32 NewPriority) const
{
	for (const FPlayerCondition& condition : CurrentConditions) {
		if (!condition.HasConditionAnimation()) continue;
		if (condition.Priority > NewPriority) return false;
	}

	return true;
}

void UPlayerConditionComponent::RemoveAnimationConditionsForTransformation(int32 NewPriority, bool bEndEffect)
{
	if (!Player) return;

	TSet<FName> RemovedConditionNames;
	TMap<FName, FPlayerCondition> LastRemovedConditions;

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; --i) {
		FPlayerCondition& condition = CurrentConditions[i];
		if (!condition.HasConditionAnimation()) continue;

		if (condition.Priority <= NewPriority) {
			if (condition.ConditionEffect) {
				condition.ConditionEffect->EndFunction(Player, this, condition, bEndEffect);
			}
			RemovedConditionNames.Add(condition.ConditionName);
			LastRemovedConditions.Add(condition.ConditionName, condition);
			CurrentConditions.RemoveAt(i);
		}
	}

	for (const FName& RemovedName : RemovedConditionNames) {
		if (!HasSameConditionByName(RemovedName)) {
			Multicast_StopConditionLoopEffect(RemovedName);
			Multicast_StopConditionOverlay(RemovedName, 0.5f);
			if (bEndEffect) {
				if (FPlayerCondition* RemovedCondition = LastRemovedConditions.Find(RemovedName)) {
					PlayConditionOnceEffect(RemovedCondition->ConditionEndEffect, *RemovedCondition);
				}
			}
		}
	}
}

void UPlayerConditionComponent::ResumeCurrentConditionAnimation()
{
	if (!Player) return;
	if (!Player->HasAuthority()) return;

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; --i) {
		FPlayerCondition& Condition = CurrentConditions[i];

		if (!Condition.HasConditionAnimation()) continue;
		if (!Condition.ConditionEffect) continue;

		Condition.ConditionEffect->ResumeEffectVisual(Player, this, Condition);

		return;
	}
}

//플레이어 탈락 시 플레이어의 모든 Condition 제거 및 정리
void UPlayerConditionComponent::ClearAllConditionWhenPlayerOut()
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player || !Player->HasAuthority()) return;

	for (FPlayerCondition& Condition : CurrentConditions) {
		if (Condition.ConditionEffect) Condition.ConditionEffect->EndFunction(Player, this, Condition, false);
	}

	CurrentConditions.Empty();
	MultiApplyIntervalMap.Empty();

	Multicast_ClearAllConditionVisualWhenPlayerOut();
}

void UPlayerConditionComponent::ClearAllConditionVisualsWhenPlayerOut_Local()
{
	if(!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player) return;

	UWorld* World = GetWorld();
	if (World) World->GetTimerManager().ClearTimer(OverlayFadeTimerHandle);

	for (auto& Pair : ActiveConditionPersistEffects) {
		FActiveConditionPersistEffect& ActiveEffect = Pair.Value;

		if (ActiveEffect.NiagaraComp) {
			ActiveEffect.NiagaraComp->DeactivateImmediate();
			ActiveEffect.NiagaraComp->DestroyComponent();
			ActiveEffect.NiagaraComp = nullptr;
		}
	}

	ActiveConditionPersistEffects.Empty();
	
	for (auto& Pair : ActiveConditionPersistSounds) {
		if (Pair.Value) {
			Pair.Value->Stop();
			Pair.Value->DestroyComponent();
		}
	}

	ActiveConditionPersistSounds.Empty();

	for (auto& Pair : ActiveConditionOverlays) {
		FActiveConditionOverlayEffect& ActiveOverlay = Pair.Value;

		if (ActiveOverlay.OverlayMID) {
			ActiveOverlay.OverlayMID->SetScalarParameterValue(TEXT("Opacity"), 0.f);
		}
	}

	ActiveConditionOverlays.Empty();
	CurrentOverlayConditionName = NAME_None;

	if (USkeletalMeshComponent* MeshComp = Player->GetMesh()) {
		MeshComp->SetOverlayMaterial(DefaultOverlayMaterial);
		MeshComp->MarkRenderStateDirty();
	}

	DefaultOverlayMaterial = nullptr;
	NotifyTransformationOverlayChanged_Local();
}


void UPlayerConditionComponent::Multicast_StopConditionOverlay_Implementation(FName ConditionName, float FadeOutTime)
{
	StopConditionOverlay_Local(ConditionName, FadeOutTime);
}

void UPlayerConditionComponent::Multicast_StartConditionOverlay_Implementation(FName ConditionName, UMaterialInterface* OverlayMaterial, FName OpacityParamName, int32 Priority)
{
	StartConditionOverlay_Local(ConditionName, OverlayMaterial, OpacityParamName, Priority);
}

void UPlayerConditionComponent::Multicast_StartConditionLoopEffect_Implementation(FName ConditionName, FGameEffectData EffectData, FGameEffectRuntimeParams RuntimeParams)
{
	StartConditionLoopEffect_Local(ConditionName, EffectData, RuntimeParams);
}

void UPlayerConditionComponent::Multicast_StopConditionLoopEffect_Implementation(FName ConditionName)
{
	StopConditionLoopEffect_Local(ConditionName);
}

void UPlayerConditionComponent::Multicast_ClearAllConditionVisualWhenPlayerOut_Implementation()
{
	ClearAllConditionVisualsWhenPlayerOut_Local();
}

void UPlayerConditionComponent::Multicast_RefreshConditionOverlayVisual_Implementation()
{
	RefreshConditionOverlay_Local();
}

void UPlayerConditionComponent::Multicast_PlayConditionOnceSound_Implementation(USoundBase* Sound)
{
	if (!Player) Player = Cast<APlayer_Character>(GetOwner());
	if (!Player || !Player->GetMesh()) return;
	if (!Sound) return;

	UGameplayStatics::SpawnSoundAttached(Sound, Player->GetMesh(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
}

void UPlayerConditionComponent::Multicast_StartConditionPersistSound_Implementation(FName ConditionName, USoundBase* Sound)
{
	StartConditionPersistSound_Local(ConditionName, Sound);
}

void UPlayerConditionComponent::Multicast_StopConditionPersistSound_Implementation(FName ConditionName, float FadeOutTime)
{
	StopConditionPersistSound_Local(ConditionName, FadeOutTime);
}
