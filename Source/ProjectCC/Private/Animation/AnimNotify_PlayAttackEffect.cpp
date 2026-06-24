// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotify_PlayAttackEffect.h"
#include "Player_Character.h"

void UAnimNotify_PlayAttackEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;
	
	APlayer_Character* Player = Cast<APlayer_Character>(MeshComp->GetOwner());
	if (!Player) return;

	Player->PlayAttackEffectByNotify();
}
