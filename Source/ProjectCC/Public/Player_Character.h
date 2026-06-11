// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerStats.h"
#include "Player_State.h"
#include "WeaponStats.h"
#include "PlayerConditions.h"
#include "Player_FunctionInterActionReason.h"
#include "Components/WidgetComponent.h"
#include "Components/LineBatchComponent.h"
#include "Player_Character.generated.h"


DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float, float);
DECLARE_MULTICAST_DELEGATE(FOnWeaponChanged);

struct FAimPreviewVisualData;
class AAttackPreviewGuide;
class UPlayerConditionComponent;
class UPlayerTransformationComponent;
class UPlayerVisualManagerComponent;
class UEffectManagerComponent;
class UPlayer_CharacterWidget;
class UNiagaraSystem;
class UNiagaraComponent;
class AMapConstructor;
class UBoxComponent;
class UPlayer_AdditionalWidget;
class APlayer_State;
class UWidgetComponent;
class UAnimMontage;
class UAnimSequenceBase;
class AObjects;
class AEquipment;
class AWeapon;
class AItem;
class AActor;

USTRUCT(BlueprintType)
struct FSpeedController {
	GENERATED_BODY()

public:
	UPROPERTY()
	FName SpeedControllerName = FName("Default");

	UPROPERTY()
	float SpeedMagnification = 1.f;

	UPROPERTY()
	float SpeedOffset = 0.f;

	UPROPERTY()
	int32 SpeedPriority = 0;

	UPROPERTY()
	bool bConstant = false;
};

USTRUCT(BlueprintType)
struct FInputBlockController {
	GENERATED_BODY()

public:
	UPROPERTY()
	FName BlockControllerName = FName("Default");

	UPROPERTY()
	bool bBlockMove = false;

	UPROPERTY()
	bool bBlockCamera = false;
};

UCLASS()
class PROJECTCC_API APlayer_Character : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayer_Character();
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	//서버 Replicate Property
	UPROPERTY(ReplicatedUsing = OnRep_HP, VisibleAnywhere, BlueprintReadOnly, Category = "HP")
	float HP;
	UPROPERTY(ReplicatedUsing = OnRep_bIsOut, VisibleAnywhere, BlueprintReadOnly, Category = "HP")
	bool bIsOut = false;
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsDodging = false;
	UPROPERTY(Replicated)
	bool bIsAiming = false;

public:
	//UI Delegate 바인드
	FOnHPChanged OnHPChanged;
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> WidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UPlayer_CharacterWidget> PlayerHeadWidget;
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UPlayer_AdditionalWidget> AdditionalImageWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UPlayer_AdditionalWidget> AdditionalImageWidget;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void PawnClientRestart() override;
public:
	//플레이어 스탯-------------------------------------------------------
	UPROPERTY(ReplicatedUsing = OnRep_MoveSpeed, BlueprintReadOnly)
	float move_Speed = 400.f;
	UPROPERTY(Replicated)
	int32 Aim_TurnSpeed = 0;
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 Weight = 1;
	UFUNCTION()
	void OnRep_MoveSpeed();
	//플레이어 상태-------------------------------------------------------
	//플레이어 조작 가능 상태
	UPROPERTY(ReplicatedUsing = OnRep_CanControl)
	bool bCanControl = true;
	UPROPERTY(Replicated)
	bool bCanCamControl = true;
	UPROPERTY(BlueprintReadWrite, Category = "Control")
	bool bIsDodgeLocked = false;
	UPROPERTY(BlueprintReadWrite, Category = "Control")
	bool bIsInteractionLocked = false;
	UPROPERTY(Replicated)
	bool bEndMatchState = false;
	UFUNCTION()
	void OnRep_CanControl();
	UFUNCTION()
	void OnRep_HP();
	UFUNCTION()
	void OnRep_bIsOut();
	UFUNCTION()
	void HandlePortraitIdChanged(int32 NewPortraitId);
	//플레이어 캐릭터 기본 스탯
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	FPlayerStats BaseStats;
	//플레이어 Condition 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	UPlayerConditionComponent* ConditionComp;
	//플레이어 Transformation 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	UPlayerTransformationComponent* TransformationComp;
	//플레이어 VisualManager 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Visual Manager")
	UPlayerVisualManagerComponent* VisualManagerComp;
	//플레이어 SpeedController (속도 조정자)
	UPROPERTY()
	TArray<FSpeedController> SpeedControllers;
	//플레이어 BlockController (조작 제어)
	UPROPERTY()
	TArray<FInputBlockController> BlockControllers;
	//플레이어 카메라-----------------------------------------------------
	//플레이어 카메라가 위치할 springArm
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* springArmComp;
	//플레이어 카메라
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* playerCamComp;
	//플레이어 이동-----------------------------------------------------
	//플레이어 컨트롤러
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* imc_Player;
	//플레이어 이동
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Move;
	//카메라 회전
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_CamTurn;
	//플레이어 점프
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Jump;
	//플레이어 카메라 기준 회전(서버)
	UPROPERTY(EditAnywhere)
	float ServerControlYaw = 0.f;
	//플레이어 비조준 회전값
	UPROPERTY(EditAnywhere)
	float ServerMoveFacingYaw = 0.f;
	//플레이어 비조준 회전 상태
	UPROPERTY(EditAnywhere)
	bool bHavingServerMoveFacingYaw = false;
	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	float YawSendtoServerInterval = 0.06f;
	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	float YawSendtoServerMinChange = 2.0f;
	//플레이어 회전속도
	UPROPERTY(EditAnywhere)
	float turn_Speed = 10;
	//카메라 회전 속도
	UPROPERTY(EditAnywhere)
	float Camturn_Speed = 0.25;
	//플레이어가 가라앉는 속도(Liquid 한정)
	UPROPERTY(EditAnywhere)
	float SinkSpeed = 10.f;
	//이동 입력 없이 이동 가능 여부
	UPROPERTY(Replicated)
	bool bMaintainMoveOnNotInput = false;
	//입력이 없을 때 유지되는 이동 비율
	UPROPERTY(Replicated)
	float NotInputMoveScale = 0.5f;
	//플레이어 상호작용---------------------------------------------------
	//플레이어 상호작용 / Equipment 줍기
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Interaction;
	//플레이어 Equipment 버리기
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_EquipmentDrop;
	//플레이어 장착 아이템 사용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_ItemUse;
	//플레이어 상호작용 범위
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TObjectPtr<UBoxComponent> PickupDetectRange;
	//장착 무기
	UPROPERTY(ReplicatedUsing = OnRep_NowWeapon)
	TObjectPtr<AWeapon> NowWeapon;
	//장착 무기 변경 시 서버/로컬 알림
	UFUNCTION()
	void OnRep_NowWeapon();
	//장착 아이템
	UPROPERTY(Replicated)
	TObjectPtr<AItem> NowItem;
	//장착 물체
	UPROPERTY(ReplicatedUsing = OnRep_NowObjects)
	TObjectPtr<AObjects> NowObjects;
	//장착 물체 변경 시 서버/로컬 알림
	UFUNCTION()
	void OnRep_NowObjects();
	//적용 중인 서포트
	UPROPERTY(Replicated)
	TObjectPtr<AObjects> NowSupport;
	//아이템 슬롯
	UPROPERTY(EditAnywhere, Category = "EquipmentSlot")
	TObjectPtr<USceneComponent> ItemSlot;
	//물체 슬롯
	UPROPERTY(EditAnywhere, Category = "EquipmentSlot")
	TObjectPtr<USceneComponent> ObjectsSlot;
	//서포트 슬롯
	UPROPERTY(EditAnywhere, Category = "EquipmentSlot")
	TObjectPtr<USceneComponent> SupportSlot;
	//코인 BP를 설정
	UPROPERTY(EditDefaultsOnly, Category = "CoinSlot")
	TSubclassOf<class ACoin> CoinSlot;
	//Middle 코인 BP를 설정
	UPROPERTY(EditDefaultsOnly, Category = "CoinSlot")
	TSubclassOf<class ACoin> MidCoinSlot;
	//Big코인 BP를 설정
	UPROPERTY(EditDefaultsOnly, Category = "CoinSlot")
	TSubclassOf<class ACoin> BigCoinSlot;
	//플레이어 회피동작---------------------------------------------------
	//플레이어 회피
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_dodge;
	//플레이어 회피 쿨타임
	UPROPERTY(EditAnywhere)
	float DodgeCoolTime = 0.7f;
	//플레이어 지상 회피 지속시간
	UPROPERTY(EditAnywhere)
	float DodgeDuration = 0.3f;
	//플레이어 공중 회피 지속시간
	UPROPERTY(EditAnywhere)
	float AirDodgeDuration = 0.4f;
	//플레이어 회피 지상 출력
	UPROPERTY(EditAnywhere)
	float DodgeStrength = 900.f;
	//플레이어 회피 공중 출력
	UPROPERTY(EditAnywhere)
	float AirDodgeDistance = 350.f;
	//회피 지면 마찰
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeGroundFriction = 0.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeBrakingFrictionFactor = 0.f;
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeBrakingDecel = 0.f;
	//회피 타이머
	FTimerHandle DodgeTimerHandle;;
	//플레이어 공격---------------------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Attack;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Targeting;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Aim;
	//플레이어 선딜레이 타이머
	FTimerHandle AttackEarlierDelayTimerHanlde;
	//플레이어 넉백 (friction)----------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KnockBack")
	bool bUseKnockBackAirDamping = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KnockBack")
	float KnockBackAirDamping = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KnockBack")
	float KnockBackAirMinSpeed = 80.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KnockBack")
	float KnockBackAirDampingDuration = 0.75f;
	UPROPERTY(Transient)
	bool bKnockBackAirDampingActive = false;
	UPROPERTY(Transient)
	float KnockBackAirDampingElapsed = 0.f;
	//플레이어 조준----------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Aim")
	TEnumAsByte<ECollisionChannel> AimTraceChannel = ECC_GameTraceChannel2;
	UPROPERTY(Replicated)
	FVector ServerAimPoint = FVector::ZeroVector;
	//조준 프리뷰
	UPROPERTY(EditAnywhere, Category = "Aim")
	TSubclassOf<AAttackPreviewGuide> AttackPreviewGuide;
	UPROPERTY()
	TObjectPtr<AAttackPreviewGuide> AimPreview;
	//조준 갱신 간격/최소 차이
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float AimSendtoServerInterval = 0.06f;
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float AimSendtoServerMinDistance = 15.f;
	//플레이어 피격 상태
	UPROPERTY(Replicated)
	bool bIsHitted = false;
	//플레이어 조준점--------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Cursor")
	TSubclassOf<UUserWidget> Player_AimPointWidget;
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> AimPoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Cursor")
	FVector2D AimPointWidgetSize = FVector2D(50.f, 50.f);
	UPROPERTY(Transient)
	TEnumAsByte<EMouseCursor::Type> SavedMouseCursor = EMouseCursor::Default;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Cursor")
	bool bHideSystemCursorWhileAiming = true;
	UPROPERTY(Transient)
	bool bShowMouseCursor = false;
	UPROPERTY(Transient)
	FVector CurrentAimTargetPoint = FVector::ZeroVector;
	UPROPERTY(Transient)
	bool bHavingCurrentAimTargetPoint = false;
	//플레이어 탈락------------------------------------------------
	//탈락시킨 플레이어
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<APlayer_Character> WinnerPlayer = nullptr;
	//탈락이후 PlayerCharacter 제거 타이머
	FTimerHandle OutPlayerDestroyTimerHandle;
	//탈락 시 Out된 플레이어 캐릭터 이동 보간
	UPROPERTY(EditAnywhere, Category = "Out")
	float OutVisualSmoothSpeed = 2.f;
	//플레이어 코인 획득/손실------------------------------------------
	UPROPERTY(EditAnywhere)
	float CoinLoseHpInterval = 30;
	UPROPERTY(EditAnywhere)
	int32 CoinLoseAmount = 1;
	UPROPERTY(EditAnywhere)
	int32 OutCoinLoseAmount = 10;
	UPROPERTY(EditAnywhere)
	int32 SearchRadius = 4;
	UPROPERTY(EditAnywhere)
	int32 SearchHeight = 1;
	UPROPERTY(EditAnywhere)
	float CoinPathCheckRadius = 20.f;
	//서버에서 플레이어 관련 처리(RPC)----------------------------------------
	//플레이어 코인 획득
	UFUNCTION(Server, Reliable)
	void Server_AddCoin(int32 CoinValue);
	//플레이어 공격
	UFUNCTION(Server, Reliable)
	void Server_Attack(bool bHolding, FVector ClientAimPoint);
	UFUNCTION(Server, Reliable)
	void Server_AttackRelease();
	//플레이어 데미지 적용
	UFUNCTION(Server, Reliable)
	void Server_ApplyDamage(float Damage, APlayer_Character* AttackPlayer);
	UFUNCTION(Server, Reliable)
	//플레이어 상호작용
	void Server_Interaction();
	UFUNCTION(Server, Reliable)
	//플레이어 Equipment Drop
	void Server_Drop();
	UFUNCTION(Server, Reliable)
	//플레이어 Item 사용
	void Server_UseItem();
	//플레이어 조준
	UFUNCTION(Server, Unreliable)
	void Server_Aim(bool bNewAiming);
	UFUNCTION(Server, Unreliable)
	void Server_SetAim(FVector NewAimPoint);
	UFUNCTION(Server, Unreliable)
	void Server_SetMoveFacingYaw(float Yaw);
	UFUNCTION(Server, Unreliable)
	void Server_ClearMoveFacingYaw();
	UFUNCTION(Server, Reliable)
	void Server_Dodge(FVector DodgeDir);
	UFUNCTION(Server, Unreliable)
	void Server_SetControlYaw(float Yaw);
	//서버에 잃은 코인 생성 요청
	UFUNCTION(Server, Reliable)
	void Server_SpawnLostCoin(int32 Amount);
	//서버에 무기 연속 공격 요청 (원거리/원거리HS)
	UFUNCTION(Server, Reliable)
	void Server_HoldAttack();
	//탈락 시 로컬에서 Out된 플레이어 캐릭터 이동
	UFUNCTION(Client, Reliable)
	void Client_Out();
	//플레이어 화면에 추가 이미지를 띄우기 시작
	UFUNCTION(Client, Reliable)
	void Client_StartAdditionalImage(int32 ImageID);
	//플레이어 화면에 떠있던 추가 이미지 제거
	UFUNCTION(Client, Reliable)
	void Client_EndAdditionalImage();
	//애니메이션----------------------------------------------------
	//플레이어 이동 속도 (Animation에서 사용)
	UPROPERTY(Replicated, BlueprintReadOnly)
	float AnimMoveSpeed = 0.0f;
	//플레이어 이동 방향 Forward (Animation에서 사용)
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AnimMoveForward = 0.f;
	//플레이어 이동 방향 Side (Animation에서 사용)
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AnimMoveSide = 0.f;
	//플레이어 탈락 애니메이션 지속시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	float OutAnimDuration = 2.f;
	//일반 피격 몽타주 재생
	UFUNCTION(NetMulticast,Unreliable)
	void Multicast_PlayHitReaction(float Damage, bool _bIsOut, bool bApplyRotation, FVector AttackDir, bool bBigHit = false);
	//Recover 몽타주 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayRecoverReaction();
	//무기/물체별 일반 애니메이션 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAnimationDynamic(UAnimSequence* Sequence, FName SlotName, float BlendInTime, float BlendOutTime, float PlayRate, int32 LoopCount, int32 StartTime);
	//무기/물체별 특수 애니메이션 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayOverrideMontage(UAnimMontage* Montage, FName StartSection = NAME_None, bool bRestart = true, bool bPauseAfter = false);
	//슬롯 애니메이션(시퀀스) 정지
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StopSlotAnimation(FName SlotName, float BlendOutTime);
	//몽차주 애니메이션 정지
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StopMontage(UAnimMontage* Montage, float BlendOutTime);
	//강한 피격 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> BigHittedMontage;
	//강한 피격으로 날아가는 중인지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	bool bHoldBigHittingPose = false;
	//강한 피격 재생 여부
	UPROPERTY(Replicated, BlueprintReadOnly, Category="BigHit")
	bool bIsBigHitReaction = false;
	//강한 피격 기준
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BigHit")
	float BigHitKnockBackRule = 2000.f;
	//Recover 재생 기준 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BigHit")
	float RecoverVelocityRule = 50.f;
	//강한피격-Recover 전환 유지 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BigHit")
	float BigHitRecoverStopHoldTime = 0.5f;
	//BigHit 직후 LaunchCharacter가 적용되기 전 바로 Recover 방지 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BigHIt")
	float BigHitRecoverMinCheckDelay = 0.2f;
	//Recover 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> RecoverMontage;
	//강한 피격 후 Recover 재생 여부
	UPROPERTY(Replicated, BlueprintReadOnly, Category="BigHit")
	bool bIsRecoverReaction = false;
	//일반 피격 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> HittedMontage;
	//탈락 몽타주 (bIsOut || Hp <= 0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> OutMontage;
	//일반 공격 애니메이션1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> FirstNormalAttack;
	//일반 공격 애니메이션2
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> SecondNormalAttack;
	//일반 공격 애니메이션 전환 가능 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation")
	float ChangeNormalAttackAnimationTime = 1.f;
	//현재 일반 공격 애니메이션 인덱스
	UPROPERTY()
	int32 NormalAttackAnimIndex = 0;
	//최근 일반 공격 시간
	UPROPERTY()
	float LastNormalAttackTime = -1.f;
	//피격 시 캐릭터 방향 저장
	UPROPERTY()
	FRotator TargetHitRotation;	
	//플레이어 이펙트--------------------
	UPROPERTY()
	UEffectManagerComponent* EffectManagerComp;
public:
	//현재 매치에서 플레이 중인 맵
	AMapConstructor* NowMap;
	//플레이어 직전 이동 방향
	FVector LastPlayerdir;
	FVector Playerdir;
	//플레이어 위젯 초기화
	void InitPlayerWidget();
	//플레이어 위젯 숨김 (탈락 시)
	void SetPlayerWidgetVisibility(bool bVisible);
	//매치 종료 시 플레이어의 조작 제어
	void SetPlayerEndMatchState();
	//장착 중인 Objects/Weapon 아이콘 획득
	UTexture2D* GetWidgetEquippmentSlotIcon();
	//장착 중인 Objects/Weapon 남은 사용 횟수 획득
	float GetWidgetWeaponSlotPercent();
	//플레이어 이동
	void Move(const struct FInputActionValue& inputValue);
	void MoveStop(const FInputActionValue& inputValue);
	void TrySendToServerControlYaw();
	void UpdateMoveFacingFromVelocity(float DeltaTime);
	void ApplyRotation(FVector2D& InputValue, float DeltaTime);
	//플레이어 카메라 회전
	void CamTurn(const struct FInputActionValue& inputValue);
	//플레이어 점프
	void Player_Jump(const struct FInputActionValue& inputValue);
	//Equipment 장착 / 물체 상호작용
	void Interaction(const FInputActionValue& inputValue);
	void InteractionInternal();
	//예약된 무기 장착 (상점)
	void ApplyResevedWeapon();
	//무기 자동 장착
	void EquipWeaponAuto(TSubclassOf<AWeapon> weapon);
	//아이템 사용
	void UseItem(const struct FInputActionValue& inputValue);
	void UseItemInternal();
	//Equipment 해제
	void Drop(const FInputActionValue& Value);
	void DropInternal();
	float GetThrowDamageWithWeight(float weight);
	//플레이어 공격
	void Attack(const struct FInputActionValue& inputValue);
	void AttackInternal(bool bPlayAnimation = true);
	void HoldAttack(const struct FInputActionValue& inputValue);
	void AttackRelease(const struct FInputActionValue& inputValue);
	bool AttackLineOfSight(AActor* TargetActor);
	//플레이어 피격 넉백 적용 시 공중 Dampen 적용
	void UpdateKnockBackAirDamping(float DeltaTime);
	//플레이어 조준점 추가/제거
	void UpdateAimTargetPoint();
	void UpdateAimPoint();
	void HideAimPoint();
	//플레이어 조준 프리뷰
	void UpdateAimPreview(float DeltaTime);
	void HideAimPreview();
	bool BuildCurrentAttackPreviewData(FAimPreviewVisualData& OutData);
	//플레이어 회피
	void Dodge(const struct FInputActionValue& inputValue);
	void DodgeInternal(FVector DodgeDir);
	//플레이어 조준
	void Aim(const struct FInputActionValue& inputValue);
	void AimStop(const struct FInputActionValue& inputValue);
	void SetAimInternal(bool bAiming);
	void TrySendtoServerAimPoint();
	//가까운 Equipments 중 가장 가까운 것을 반환
	AEquipment* ClosestEquipment();
	//가까운 물체들 중 가장 가까운 것을 반환
	AObjects* ClosestObjects();
	//각 Equipments 해제
	void DropWeapon(float Strength, bool bIsThrowing);
	void DropItem(float Strength);
	void DropObjects(float Strength, bool bIsThrowing);
	//Equipments 장착
	bool PickWeapon(TObjectPtr<AWeapon>Weapon);
	bool PickItem(TObjectPtr<AItem> Item);
	bool PickObjects(TObjectPtr<AObjects> Object);
	//Drop한 Equipment의 위치/회전 계산
	FTransform DropTransform();
	//이동하며 Drop시 던지기 적용
	void ApplyThrow(AEquipment* Equipment, float BaseStrength, float UpStrength, float IgnorePawnSeconds);
	void ApplyThrowOb(AObjects* object, float BaseStrength, float UpStrength, float IgnorePawnSeconds);
	//플레이어의 무기 스탯 적용
	FWeaponStats GetWeaponStat();
	//마우스 포인트 위치 획득
	bool GetMousePoint(FVector& MousePoint);
	//조준 시 캐릭터 회전
	void ApplyAimRotation(float DeltaTime);
	//플레이어 회전 계산
	void ApplyPlayerRotation(float TargetYaw, float DeltaTime);
	//플레이어 현재 HP 획득
	float GetCurrentHP() const { return HP; }
	//플레이어 데미지 적용 전 처리
	float TakeDamage(float damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	//클라이언트가 데미지 적용
	float ApplyDamageInternal(float Damage, APlayer_Character* AttackPlayer, AActor* DamageCauser, bool bApplyKnockBack = true, bool bApplyRotation = true, bool bForceDamage = false);
	//플레이어 넉백
	void ApplyKnockBack(FVector& AttackDir, float Strength, float UpStrength);
	//코인 손실--------------------------------------------------------------
	void SpawnLostCoins(int32 Amount);
	//코인이 이동할 목적지가 될 블록을 탐색 및 후보 선정
	bool CollectNearbySafeBlocksFromMap(TArray<FVector>& SafeBlockLocations, int32 instanceSearchRadius, int32 instanceSearchHeight);
	void BuildCoinTargetLocations(int32 RequiredTargetCount, AMapConstructor* CurrentMap, TArray<FVector>& TargetLocations);
	//탐색된 후보 중 최종 목적지를 선정
	FVector GetCoinTargetLocation(TArray<FVector>& SafeBlockLocations, TMap<int32, int32>& UsageCount);
	//소환해야할 코인들의 List를 정리
	void SpawnCoinList(TArray<TSubclassOf<ACoin>>& CoinList, int32 Amount);
	//------------------------------------------------------------------------
	//플레이어 탈락
	void Out(APlayer_Character* WinnerPlayer);
	//탈락한 플레이어 제거 후 서버에 리스폰 요청
	void DestroyPlayer();
	//플레이어 피격 시 방향 회전
	void TurnToAttackPlayer(const FVector& AttackDir);
	//플레이어 탈락 시 장착 중인 아이템 정보 저장
	void SaveNowItem();
	//플레이어 리스폰 시 저장된 아이템 정보 획득
	void LoadNowItem();
	//-------------------------------------------------------------------------
	//플레이어 이동 방향/현재 방향 차이 계산 (Animation 사용)
	void UpdateAnimationMoveDirectionValues(float DeltaTime);
	//플레이어 피격 애니메이션 재생
	void PlayDamageAnimation(float Damage, bool bBigHit);
	//플레이어 기본 무기/물체 공격 애니메이션 재생
	void PlayAnimationDynamic(UAnimSequence* Sequence, FName SlotName, float BlendInTime, float BlendOutTime, float PlayRate, int32 LoopCount, int32 StartTime);
	//재생할 애니메이션이 어떤것인지 확인
	bool NeedToPlayAllBodyAnimation();
	//각 장착물들에 대해 재생할 애니메이션 확인 및 재생
	bool PlayEquipmentAnimation(EFunctionInterActionReason Reason);
	//애니메이션 Slot 이름 획득
	FName GetActionSlotName(bool bUseFullBodyAnim);
	//무기별 공격 몽타주 획득
	UAnimMontage* GetCurrentAttackMontague(bool bUseAllBodyAnim);
	//무기별 기본 애니메이션 획득
	UAnimSequenceBase* GetCurrentGripSequence();
	//기본 공격 애니메이션 획득
	UAnimSequence* GetNormalAttackSequence();
	//강한 피격 애니메이션 시작
	void StartBigHitReaction();
	//강한 피격 애니메이션 갱신 -> Reaction 애니메이션 시작 검사
	void UpdateBigHitReaction(float DeltaTime);
	//Recover 애니메이션 시작
	void StartRecoverReaction();
	//Recover 및 강한 피격 애니메이션 종료
	void EndBigHitReaction();
	//강한 피격 시작 시간
	float BigHitStartTime = 0.f;
	//강한 피격 종료 시간
	float BigHitStopTime = 0.f;
	//강한 피격 후 Recover 시작 타이머
	FTimerHandle BigHitRecoverTimerHandle;
	//공격 애니메이션 완료 후 조준 중이면 조준 애니메이션 재생 타이머
	FTimerHandle ResumeAimAnimationTimerHandle;
public:
	//현재 플레이어의 Player_State
	TObjectPtr<APlayer_State> NowPlayer_State;
	//PlayerState 바인딩
	void BindPlayer_State();
	//행동별 Condition 제어
	void NotifyConditionEvent(EPlayerConditionEvent Event, bool bUseEndEffect = true);
	//이동키 입력 여부
	bool bMoveInputHolding = false;
	void SetMaintainMoveOnNotInput(bool bEnable, float InNoInputMoveScale = 0.5f);
	void UpdateMaintainMoveOnNotInput(float DeltaTime);
	//회피 상태
	float LastDodgeTime = -5.f;
	float SavedGroundFriction = 0.f;
	float SavedBrakingFrictionFactor = 0.f;
	float SavedBrakingDecel = 0.f;
	float SavedBrakingDecelFalling = 0.f;
	float SavedFallingLateralFriction = 0.f;
	//플레이어 점프 쿨타임
	float JumpCoolTime = 0.2f;
	float LastJumpTime = -1.f;
	//플레이어 공격 상태
	float LastAttackTime = -5.f;
	//(홀딩 공격 시) 홀딩 공격 재시작이 필요한지 확인
	bool bNowHoldingAttack = false;
	//플레이어 Equipment 장착 쿨타임
	float EquipCoolTime = 0.5f;
	float LastEquipTime = -5.f;
	//플레이어 Equipment 해제 쿨타임
	float UnEquipCoolTime = 0.5f;
	float LastUnEquipTime = -5.f;
	//플레이어의 던지는 힘
	float PutStrength = 30.f;
	float MoveStrength = 100.f;
	float ThrowStrength = 1000.f;
	//플레이어 현재 Stat
	FWeaponStats AStat;
	float WeightPenalty();
	float CalculateSpeed(float Default_Speed = -1.f);
	void UpdateMoveSpeed();
	//속도 조정자 반영
	void AddSpeedController(FName ControllerName, float Magnification, float offset, bool bConstantSpeed = false, int32 Priority = 0);
	//속도 조정자 제거
	void RemoveSpeedControllerByName(FName ControllerName);
	void RemoveSpeedControllerByPriority(int32 Priority);
 	//플레이어의 현재 상태 데이터를 서버에서 획득
	APlayer_State* GetThePlayerState();
	//플레이어의 현재 상태 데이터를 반영
	virtual void OnRep_PlayerState() override;
	//플레이어 체력 변경
	void HPChange(float HPAmount);
	//플레이어 입력 가능 상태 변경 (BlockController 사용)
	//BlockController 추가
	void AddInputBlockController(FName ControllerName, bool bBlockMove, bool bBlockCamera, bool bStopMovementOnAdd = true, bool bIsOnLiquid = false);
	//BlockController 제거
	void RemoveInputBlockController(FName ControllerName);
	//현재 존재하는 BlockController들에 맞게 이동, 카메라 조작 제한 확인
	void RefreshInputBlockState(bool bStopMovementOnBlock = true, bool bIsOnLiquid = false);
	//실제 조작 제한 기능
	void ApplyInputBlockInternal(bool bIsOnLiquid);
	//플레이어 사망 상태 확인
	bool IsOut() { return bIsOut; }
	//플레이어 조준 점
	FVector LastAimPoint = FVector::ZeroVector;
	//플레이어 조준 시간
	float LastAimTime = -1.f;
	//플레이어 회전 시간
	float LastTurnTime = 0.f;
	float LastSenttoServerYaw = 0.f;
	FTimerHandle HittedResetTimerHandle;
	//플레이어 코인 손실 시간
	float LastLoseCoinHP = -1.f;
	//직전 공격 플레이어 설정
	APlayer_Character* LastAttackPlayer;
	FTimerHandle HoldLastAttackPlayer;
	void ClearLastAttackPlayer() { LastAttackPlayer = nullptr; }
	//플레이어 탈락 시 액체에 있는지 확인
	bool bIsOnLiquidWhenOut = false;
	//플레이어 탈락 시 액체에 있다면 위치 보완 (안하면 끊겨서 보임)
	bool bOutVisualSmoothing = false;

	bool CheckWeaponInteraction(EFunctionInterActionReason Reason);
	bool IsCurrentWeaponInputType(EWeaponAttackInputType InputType);
	bool IsCurrentWeaponHoldLikeAttack();

	FVector DefaultMeshLocation = FVector::ZeroVector;
	FVector LastActorLocation = FVector::ZeroVector;
	FVector VisualMeshLocation = FVector::ZeroVector;
	FVector DefaultCamLocation = FVector::ZeroVector;
	FVector VisualCamLocation = FVector::ZeroVector;

	// [사운드]=======================================================
	// 점프 효과음 지정 변수
	UPROPERTY(EditDefaultsOnly, Category="Sound")
	TObjectPtr<USoundBase> JumpSound;

	UFUNCTION(Server,Reliable)
	void Server_PlayJumpSound();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayJumpSound();

	//

};
