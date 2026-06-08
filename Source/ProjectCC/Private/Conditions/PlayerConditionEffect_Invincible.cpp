// Fill out your copyright notice in the Description page of Project Settings.


#include "Conditions/PlayerConditionEffect_Invincible.h"
#include "PlayerConditionComponent.h"
#include "Player_Character.h"

//플레이어 캐릭터 코드에서 NoDamage Condition을 가지고있는지의 여부에 따라 데미지 면역 효과 처리를 하므로 여기서는 관련 내용이 없음

void UPlayerConditionEffect_Invincible::StartEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData, APlayer_Character* usedPlayer) {
	if (ConditionComp) {
		ConditionComp->RemoveSameCategoryCondition(EPlayerConditionType::DeBuff, true, ConditionData.Priority);	// 동일한 우선순위까지 싹 지우고 싶다면 ConditionData.Priority + 1로 변경
	}
}

void UPlayerConditionEffect_Invincible::EndEffect(APlayer_Character* Player, UPlayerConditionComponent* ConditionComp, FPlayerCondition& ConditionData) {

}