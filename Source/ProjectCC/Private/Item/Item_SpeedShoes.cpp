// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item_SpeedShoes.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"

bool AItem_SpeedShoes::UseEffect_Implementation(APlayer_Character* Player)
{
    if (!HasAuthority() || !Player || !FastConditionData) return false;

    if (Player->ConditionComp) {
        Player->ConditionComp->ApplyCondition(FastConditionData, Player, SpeedDuration);
        return true;
    }

    return false;
}
