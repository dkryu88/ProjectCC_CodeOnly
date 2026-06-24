// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_SqueakyHammer.h"
#include "Player_Character.h"
#include "PlayerConditionComponent.h"

void AWeapon_SqueakyHammer::HitEffect_Implementation(APlayer_Character* Player, AActor* Target)
{
	if (!HasAuthority() || !Player || !Target) return;

	APlayer_Character* CurrentTarget = Cast<APlayer_Character>(Target);
	if (!CurrentTarget) return;

	//콤보 시간제한(기능필요시 주석풀고 사용)
	//float CurrentTime = GetWorld()->GetTimeSeconds();
	//if (CurrentTime - LastHitTime > ComboResetTime) {
	//	ComboStack = 0;
	//}
	//LastHitTime = CurrentTime; // 타격 시간 갱신

	// 이전공격 대상과 같다면
	if (LastHitPlayer == CurrentTarget) {
		ComboStack++;

		if (ComboStack >= 3) {	//3연타 공격시
			ComboStack = 0;		//스택 초기화

			// 스턴 데이터에셋 적용
			if (Player && StunDataAsset && CurrentTarget && !CurrentTarget->IsOut() && CurrentTarget->ConditionComp) {
				CurrentTarget->ConditionComp->ApplyCondition(StunDataAsset, Player, StunDuration);
			}
		}
	}
	// 최초공격 or 이전 대상과 다른 대상이면 1스택으로 변경
	else {
		ComboStack = 1;
		LastHitPlayer = CurrentTarget;
	}
}
