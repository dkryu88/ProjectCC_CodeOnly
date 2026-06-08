// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapon_SniperRifle.generated.h"

/**
 * 
 */
class ACameraActor;
class UUserWidget;

UCLASS()
class PROJECTCC_API AWeapon_SniperRifle : public AWeapon
{
	GENERATED_BODY()
	
public:
	AWeapon_SniperRifle();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual bool InteractionWeaponFunction(EFunctionInterActionReason Reason) override;
	virtual bool BeforeAttackWeaponFunction() override;
	virtual void ReleaseAttackWeaponFunction() override {}

	virtual float OnPreHit(APlayer_Character* TPlayer, bool& bSkipRotation) override;
	virtual bool BuildAimPreviewData(APlayer_Character* Player, FAimPreviewVisualData& PreviewData) override;
protected:
	virtual void EquipEffect_Implementation(APlayer_Character* Player) override;
	virtual void UnEquipEffect_Implementation(APlayer_Character* Player) override;
	virtual void AdditionalUnEquipWeaponFunction() override;
public:
	void ToggleScopeMode();
	void ApplyScopeModeEffects(bool bEnabled);
	void UpdateScopeLogic();
	void Local_RequestFire();

	UFUNCTION()
	void HandleOwnerTakenDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(Client, Reliable)
	void Client_InitializeScopeUI(APlayer_Character* Player);
	UFUNCTION(Client, Reliable)
	void Client_ForceExitScope();
	UFUNCTION(Server, Reliable)
	void Server_SyncScopeState(bool bNewZoom);
	UFUNCTION(Server, Reliable)
	void Server_ExecuteFireTrace(FVector TraceStart, FVector TraceEnd);

public:
	UPROPERTY(Replicated)
	bool bIsScopeModeActive = false;

	UPROPERTY(Replicated)
	TObjectPtr<ACameraActor> ScopeCameraActor = nullptr;
	// 서브클래스 - BP에서 드롭다운으로 WBP를 선택입력하는 칸의 이름 = Ui
	UPROPERTY(EditDefaultsOnly, Category = "Sniper|UI")
	TSubclassOf<UUserWidget> ScopeWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> ScopeWidget = nullptr;
	// 카메라의 머리 위 높이
	UPROPERTY(EditDefaultsOnly, Category="Sniper|Camera")
	float ScopeCameraHeight = 700.f;
	// 카메라 최대 이동 거리
	UPROPERTY(EditDefaultsOnly, Category = "Sniper|Camera")
	float MaxScrollDistance = 2500.f;
	// 카메라 움직임 민감도
	UPROPERTY(EditDefaultsOnly, Category = "Sniper|Camera")
	float MoveSensitivity = 6.f;
	// 마우스 커서 움직인 거리에 곱해지는 배수
	UPROPERTY(EditDefaultsOnly, Category = "Sniper|Camera")
	float ScrollSpeedMultiplier = 0.05f;
	// 로직함수 타이머가 실행되는 간격(60프레임 간격= 약 0.016초)
	UPROPERTY(EditDefaultsOnly, Category = "Sniper|Camera")
	float LogicUpdateInterval = 0.02f;
	// 화면의 조준점 추적 속도 (낮을수록 느림)
	UPROPERTY(EditDefaultsOnly, Category = "Sniper|UI")
	float CrosshairInterpSpeed = 25.f;

	// 스코프카메라로 전환 소요시간
	UPROPERTY(EditAnywhere, Category = "Sniper|Camera|Setting")
	float ToScopeCameraBlendTime = 0.3f;

	// 기본카메라로 전환 소요시간
	UPROPERTY(EditAnywhere, Category = "Sniper|Camera|Setting")
	float ToOriginCameraBlendTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Sniper|Fire")
	float FireMaxRange = 2000.f;
	// 가상의 발사체의 두께 반지름
	UPROPERTY(EditDefaultsOnly, Category = "Sniper|Fire")
	float BulletSweepRadius = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Sniper|Option")
	bool bAutoExitScopeAfterFire = false;

	UPROPERTY(EditDefaultsOnly, Category = "Sniper|Option")
	bool bAutoExitScopeOnDamage = true;

private:
	FTimerHandle ScopeLogicTimerHandle;

	FRotator ScopeEntryRotation = FRotator::ZeroRotator;
	FVector2D VirtualCursorOffset = FVector2D::ZeroVector;
	FVector2D CurrentCrosshairPos = FVector2D::ZeroVector;
};
