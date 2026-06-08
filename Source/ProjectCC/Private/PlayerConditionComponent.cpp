// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerConditionComponent.h"
#include "Player_Character.h"
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

				if (Condition.EffectCount >= 0) {
					Condition.RemainingTickCount--;

					if (Condition.RemainingTickCount <= 0) break;
				}
			}
		}

		bool bTickEffectEnd = (Condition.EffectInterval > 0.f && Condition.EffectCount >= 0 && Condition.RemainingTickCount <= 0);

		if (bTimeOver || bTickEffectEnd) {
			if (CurrentConditions[i].ConditionEffect) {
				CurrentConditions[i].ConditionEffect->EndFunction(Player, this, CurrentConditions[i], false);
			}
			CurrentConditions.RemoveAt(i);
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
	NewCondition.ConditionMontage = ConditionData->ConditionMontage;
	
	NewCondition.JumpRule = ConditionData->JumpRule;
	NewCondition.AttackRule = ConditionData->AttackRule;
	NewCondition.DodgeRule = ConditionData->DodgeRule;
	NewCondition.HitRule = ConditionData->HitRule;
	NewCondition.BigHitRule = ConditionData->BigHitRule;
	
	NewCondition.NextEffectTimer = ConditionData->EffectInterval;
	NewCondition.RemainingTickCount = ConditionData->EffectCount;

	if (ConditionData->ConditionEffect) {
		NewCondition.ConditionEffect = NewObject<UPlayerConditionEffect>(this, ConditionData->ConditionEffect);
	}

	if (!TryGetVisualSlotForCondition(NewCondition, false)) return;

	if (NewCondition.ConditionEffect) {
		NewCondition.ConditionEffect->StartEffect(Player, this, NewCondition, CausePlayer);
	}
	CurrentConditions.Add(NewCondition);

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
	for (int32 i = 0; i < CurrentConditions.Num(); i++) {
		if (CurrentConditions[i].ConditionName == TargetConditionName) {
			if (CurrentConditions[i].ConditionEffect) {
				CurrentConditions[i].ConditionEffect->EndFunction(Player, this, CurrentConditions[i], false);
			}
			CurrentConditions.RemoveAt(i);
			i--;
		}
	}
	UPlayerTransformationComponent* TransformationComp = Player->TransformationComp;
	if (TransformationComp && TransformationComp->CurrentTransformation.TransformationName == TargetConditionName) {
		TransformationComp->StopTransformation(false);
	}
}

//특정 Condition Type을 전부 제거 (우선 순위 확인)
void UPlayerConditionComponent::RemoveSameCategoryCondition(EPlayerConditionType TargetConditionType, bool bEndEffect, int32 priority) {
	for (int32 i = CurrentConditions.Num() - 1; i >= 0; i--) {
		if (CurrentConditions[i].ConditionType == TargetConditionType && CurrentConditions[i].Priority < priority) {
			if (CurrentConditions[i].ConditionEffect) {
				CurrentConditions[i].ConditionEffect->EndFunction(Player, this, CurrentConditions[i], false);
			}
			CurrentConditions.RemoveAt(i);
		}
	}
	UPlayerTransformationComponent* TransformationComp = Player->TransformationComp;
	if (TransformationComp) {
		TryRemoveTransform(5, TargetConditionType, false);
	}
}

//우선순위보다 낮은 Condition을 전부 제거 (Priority가 5면 모두 제거)
void UPlayerConditionComponent::RemoveLowPriorityCondition(int32 Priority, bool bEndEffect) {
	for (int32 i = CurrentConditions.Num() - 1; i >= 0; i--) {
		if (CurrentConditions[i].Priority < Priority) {
			if (CurrentConditions[i].ConditionEffect) {
				CurrentConditions[i].ConditionEffect->EndFunction(Player, this, CurrentConditions[i], false);
			}
			CurrentConditions.RemoveAt(i);
		}
	}
	UPlayerTransformationComponent* TransformationComp = Player->TransformationComp;
	if (TransformationComp) {
		TryRemoveTransform(Priority, EPlayerConditionType::None, false);
	}
}

//특정 Condition이 있는지 체크
bool UPlayerConditionComponent::CheckCondition(FName TargetConditionName) {
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
			if (Condition.ConditionEffect) {
				Condition.ConditionEffect->EndFunction(Player, this, Condition, bUseEndEffect);
			}
		}

		CurrentConditions.RemoveAt(i);
	}
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

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; --i) {
		FPlayerCondition& Condition = CurrentConditions[i];
		if (!Condition.HasConditionAnimation()) continue;
		if (Condition.Priority <= NewCondition.Priority){
			if (Condition.ConditionEffect){
				Condition.ConditionEffect->EndFunction(Player, this, Condition, bEndEffect);
			}
			CurrentConditions.RemoveAt(i);
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

	for (int32 i = CurrentConditions.Num() - 1; i >= 0; --i) {
		FPlayerCondition& condition = CurrentConditions[i];
		if (!condition.HasConditionAnimation()) continue;

		if (condition.Priority <= NewPriority) {
			if (condition.ConditionEffect) {
				condition.ConditionEffect->EndFunction(Player, this, condition, bEndEffect);
			}
			CurrentConditions.RemoveAt(i);
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


