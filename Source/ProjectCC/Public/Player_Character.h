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
class UGameEffectManagerComponent;
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
class UPlayerSoundDataAsset;

UENUM(BlueprintType)
enum class EPlayerImmunityType : uint8 {
	None				UMETA(DisplayName = "Fallback"),
	DamageImmunity		UMETA(DisplayName = "Damage Immunity"),
	DebuffImmunity		UMETA(DisplayName = "Debuff Immunity"),
	Invincible			UMETA(DisplayName = "Invincible")
};

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
struct FDamageImmunityController {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ImmunityControllerName = FName("Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPlayerImmunityType  Type = EPlayerImmunityType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanEraseForce = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 0;

	FTimerHandle ControllerTimerHandle;
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
	//?œë²„ Replicate Property
	UPROPERTY(ReplicatedUsing = OnRep_HP, VisibleAnywhere, BlueprintReadOnly, Category = "HP")
	float HP;
	UPROPERTY(ReplicatedUsing = OnRep_bIsOut, VisibleAnywhere, BlueprintReadOnly, Category = "HP")
	bool bIsOut = false;
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsDodging = false;
	UPROPERTY(Replicated)
	bool bIsAiming = false;
	UPROPERTY(Replicated)
	bool bIsAttacking = false;

public:
	//UI Delegate ë°”ì¸??
	FOnHPChanged OnHPChanged;
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> WidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayer_CharacterWidget> PlayerHeadWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
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
	virtual void OnJumped_Implementation() override;
public:
	//?Œë ˆ?´ì–´ ?¤íƒ¯-------------------------------------------------------
	UPROPERTY(ReplicatedUsing = OnRep_MoveSpeed, BlueprintReadOnly)
	float move_Speed = 400.f;
	UPROPERTY(Replicated)
	int32 Aim_TurnSpeed = 0;
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 Weight = 1;
	UFUNCTION()
	void OnRep_MoveSpeed();
	//?Œë ˆ?´ì–´ ?íƒœ-------------------------------------------------------
	//?Œë ˆ?´ì–´ ì¡°ì‘ ê°€???íƒœ
	UPROPERTY(ReplicatedUsing = OnRep_CanControl)
	bool bCanControl = true;
	UPROPERTY(Replicated)
	bool bCanCamControl = true;
	UPROPERTY(BlueprintReadWrite, Category = "Control")
	bool bIsDodgeLocked = false;
	UPROPERTY(Replicated)
	bool bEndMatchState = false;
	UPROPERTY(Replicated)
	bool bInteractionLock = false;
	//?œë ë¶ˆê? ?íƒœ ?¬ë? (?„ì´???¬í•¨)
	UPROPERTY(Replicated)
	bool bDropLock = false;
	//?¥ì°©/?¥ì°© ?´ì œ ë¶ˆê? ?íƒœ ?¬ë?
	UPROPERTY(Replicated)
	bool bEquipLock = false;
	UFUNCTION()
	void OnRep_CanControl();
	UFUNCTION()
	void OnRep_HP();
	UFUNCTION()
	void OnRep_bIsOut();
	UFUNCTION()
	void HandlePortraitIdChanged(int32 NewPortraitId);
	//?Œë ˆ?´ì–´ ìºë¦­??ê¸°ë³¸ ?¤íƒ¯
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	FPlayerStats BaseStats;
	//?Œë ˆ?´ì–´ Condition ì»´í¬?ŒíŠ¸
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	UPlayerConditionComponent* ConditionComp;
	//?Œë ˆ?´ì–´ Transformation ì»´í¬?ŒíŠ¸
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	UPlayerTransformationComponent* TransformationComp;
	//?Œë ˆ?´ì–´ VisualManager ì»´í¬?ŒíŠ¸
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Visual Manager")
	UPlayerVisualManagerComponent* VisualManagerComp;
	//?Œë ˆ?´ì–´ SpeedController (?ë„ ì¡°ì •??
	UPROPERTY()
	TArray<FSpeedController> SpeedControllers;
	//?Œë ˆ?´ì–´ BlockController (ì¡°ì‘ ?œì–´)
	UPROPERTY()
	TArray<FInputBlockController> BlockControllers;
	//?Œë ˆ?´ì–´ ë©´ì—­ ?œì–´
	UPROPERTY()
	TArray<FDamageImmunityController> ImmunityControllers;
	//?Œë ˆ?´ì–´ ì¹´ë©”??----------------------------------------------------
	//?Œë ˆ?´ì–´ ì¹´ë©”?¼ê? ?„ì¹˜??springArm
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* springArmComp;
	//?Œë ˆ?´ì–´ ì¹´ë©”??
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* playerCamComp;
	//?Œë ˆ?´ì–´ ?´ë™-----------------------------------------------------
	//?Œë ˆ?´ì–´ ì»¨íŠ¸ë¡¤ëŸ¬
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* imc_Player;
	//?Œë ˆ?´ì–´ ?´ë™
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Move;
	//ì¹´ë©”???Œì „
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_CamTurn;
	//?Œë ˆ?´ì–´ ?í”„
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Jump;
	//?Œë ˆ?´ì–´ ì¹´ë©”??ê¸°ì? ?Œì „(?œë²„)
	UPROPERTY(EditAnywhere)
	float ServerControlYaw = 0.f;
	//?Œë ˆ?´ì–´ ë¹„ì¡°ì¤€ ?Œì „ê°?
	UPROPERTY(EditAnywhere)
	float ServerMoveFacingYaw = 0.f;
	//?Œë ˆ?´ì–´ ë¹„ì¡°ì¤€ ?Œì „ ?íƒœ
	UPROPERTY(EditAnywhere)
	bool bHavingServerMoveFacingYaw = false;
	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	float YawSendtoServerInterval = 0.06f;
	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	float YawSendtoServerMinChange = 2.0f;
	//?Œë ˆ?´ì–´ ?Œì „?ë„
	UPROPERTY(EditAnywhere)
	float turn_Speed = 10;
	//ì¹´ë©”???Œì „ ?ë„
	UPROPERTY(EditAnywhere)
	float Camturn_Speed = 0.25;
	//?Œë ˆ?´ì–´ê°€ ê°€?¼ì•‰???ë„(Liquid ?œì •)
	UPROPERTY(EditAnywhere)
	float SinkSpeed = 10.f;
	//?´ë™ ?…ë ¥ ?†ì´ ?´ë™ ê°€???¬ë?
	UPROPERTY(Replicated)
	bool bMaintainMoveOnNotInput = false;
	//?…ë ¥???†ì„ ??? ì??˜ëŠ” ?´ë™ ë¹„ìœ¨
	UPROPERTY(Replicated)
	float NotInputMoveScale = 0.5f;
	//?Œë ˆ?´ì–´ ?í˜¸?‘ìš©---------------------------------------------------
	//?Œë ˆ?´ì–´ ?í˜¸?‘ìš© / Equipment ì¤ê¸°
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Interaction;
	//?Œë ˆ?´ì–´ Equipment ë²„ë¦¬ê¸?
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_EquipmentDrop;
	//?Œë ˆ?´ì–´ ?¥ì°© ?„ì´???¬ìš©
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_ItemUse;
	//?Œë ˆ?´ì–´ ?í˜¸?‘ìš© ë²”ìœ„
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TObjectPtr<UBoxComponent> PickupDetectRange;
	//?¥ì°© ë¬´ê¸°
	UPROPERTY(ReplicatedUsing = OnRep_NowWeapon, BlueprintReadOnly)
	TObjectPtr<AWeapon> NowWeapon;
	//?¥ì°©ë¬?ê³ ì • ?íƒœ ?¬ë? (ë¬´ê¸°/ë¬¼ì²´ ?¥ì°©/ ?¥ì°©?´ì œ ë¶ˆê?)
	UPROPERTY(Replicated)
	bool bFixEquipmentMode = false;
	//?¤ì œ ê³ ì •??ë¬´ê¸° Object
	UPROPERTY()
	TObjectPtr<AActor> LockedEquipment;
	//?¥ì°© ë¬´ê¸°ê°€ ê°•ì œ ?¥ì°© ë¬´ê¸°?¸ì? ?•ì¸
	UPROPERTY()
	bool bLockedEquipmentSpawnedByForce = false;
	//?¥ì°© ë¬´ê¸°ê°€ UnEquip????Destroy ? ì? ?•ì¸
	UPROPERTY()
	bool bLockedEquipmentDestroyOnClear = false;
	//?¥ì°© ë¬´ê¸° ë³€ê²????œë²„/ë¡œì»¬ ?Œë¦¼
	UFUNCTION()
	void OnRep_NowWeapon();
	//?¥ì°© ?„ì´??
	UPROPERTY(Replicated)
	TObjectPtr<AItem> NowItem;
	//?¥ì°© ë¬¼ì²´
	UPROPERTY(ReplicatedUsing = OnRep_NowObjects, BlueprintReadOnly)
	TObjectPtr<AObjects> NowObjects;
	//?¥ì°© ë¬¼ì²´ ë³€ê²????œë²„/ë¡œì»¬ ?Œë¦¼
	UFUNCTION()
	void OnRep_NowObjects();
	//?ìš© ì¤‘ì¸ ?œí¬??
	UPROPERTY(Replicated)
	TObjectPtr<AObjects> NowSupport;
	//?„ì´???¬ë¡¯
	UPROPERTY(EditAnywhere, Category = "EquipmentSlot")
	TObjectPtr<USceneComponent> ItemSlot;
	//?œí¬???¬ë¡¯
	UPROPERTY(EditAnywhere, Category = "EquipmentSlot")
	TObjectPtr<USceneComponent> SupportSlot;
	//ì½”ì¸ BPë¥??¤ì •
	UPROPERTY(EditDefaultsOnly, Category = "CoinSlot")
	TSubclassOf<class ACoin> CoinSlot;
	//Middle ì½”ì¸ BPë¥??¤ì •
	UPROPERTY(EditDefaultsOnly, Category = "CoinSlot")
	TSubclassOf<class ACoin> MidCoinSlot;
	//Bigì½”ì¸ BPë¥??¤ì •
	UPROPERTY(EditDefaultsOnly, Category = "CoinSlot")
	TSubclassOf<class ACoin> BigCoinSlot;
	//?Œë ˆ?´ì–´ ?Œí”¼?™ì‘---------------------------------------------------
	//?Œë ˆ?´ì–´ ?Œí”¼
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_dodge;
	//?Œë ˆ?´ì–´ ?Œí”¼ ì¿¨í???
	UPROPERTY(EditAnywhere)
	float DodgeCoolTime = 0.7f;
	//?Œë ˆ?´ì–´ ì§€???Œí”¼ ì§€?ì‹œê°?
	UPROPERTY(EditAnywhere)
	float DodgeDuration = 0.3f;
	//?Œë ˆ?´ì–´ ê³µì¤‘ ?Œí”¼ ì§€?ì‹œê°?
	UPROPERTY(EditAnywhere)
	float AirDodgeDuration = 0.4f;
	//?Œë ˆ?´ì–´ ?Œí”¼ ì§€??ì¶œë ¥
	UPROPERTY(EditAnywhere)
	float DodgeStrength = 900.f;
	//?Œë ˆ?´ì–´ ?Œí”¼ ê³µì¤‘ ì¶œë ¥
	UPROPERTY(EditAnywhere)
	float AirDodgeDistance = 350.f;
	//?Œí”¼ ì§€ë©?ë§ˆì°°
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeGroundFriction = 0.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeBrakingFrictionFactor = 0.f;
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeBrakingDecel = 0.f;
	//?Œí”¼ ?€?´ë¨¸
	FTimerHandle DodgeTimerHandle;;
	//?Œë ˆ?´ì–´ ê³µê²©---------------------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Attack;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Targeting;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Aim;
	//?Œë ˆ?´ì–´ ? ë”œ?ˆì´ ?€?´ë¨¸
	FTimerHandle AttackEarlierDelayTimerHanlde;
	//?Œë ˆ?´ì–´ ?‰ë°± (friction)----------------------------------------
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
	//?Œë ˆ?´ì–´ ì¡°ì?----------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Aim")
	TEnumAsByte<ECollisionChannel> AimTraceChannel = ECC_GameTraceChannel2;
	UPROPERTY(Replicated)
	FVector ServerAimPoint = FVector::ZeroVector;
	//ê³µê²© ë°©í–¥ (ì¡°ì?/ë¹„ì¡°ì¤€ ê³µí†µ, ?œë²„?ì„œ ê³„ì‚°)
	UPROPERTY()
	FVector ServerAttackDirection = FVector::ForwardVector;
	//ì¡°ì? ?„ë¦¬ë·?
	UPROPERTY(EditAnywhere, Category = "Aim")
	TSubclassOf<AAttackPreviewGuide> AttackPreviewGuide;
	UPROPERTY()
	TObjectPtr<AAttackPreviewGuide> AimPreview;
	//ì¡°ì? ê°±ì‹  ê°„ê²©/ìµœì†Œ ì°¨ì´
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float AimSendtoServerInterval = 0.06f;
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float AimSendtoServerMinDistance = 15.f;
	//?Œë ˆ?´ì–´ ?¼ê²© ?íƒœ
	UPROPERTY(Replicated)
	bool bIsHitted = false;
	//?Œë ˆ?´ì–´ ì¡°ì???-------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Cursor")
	TSubclassOf<UUserWidget> Player_AimPointWidget;
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> AimPoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Cursor")
	FVector2D AimPointWidgetSize = FVector2D(50.f, 50.f);
	UPROPERTY(Transient)
	TEnumAsByte<EMouseCursor::Type> SavedMouseCursor = EMouseCursor::Default;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Cursor")
	bool bHideSystemCursorWhileAiming = true;
	UPROPERTY(Transient)
	bool bShowMouseCursor = false;
	UPROPERTY(Transient)
	FVector CurrentAimTargetPoint = FVector::ZeroVector;
	UPROPERTY(Transient)
	bool bHavingCurrentAimTargetPoint = false;
	//?Œë ˆ?´ì–´ ?ˆë½------------------------------------------------
	//?ˆë½?œí‚¨ ?Œë ˆ?´ì–´
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<APlayer_Character> WinnerPlayer = nullptr;
	//?ˆë½?´í›„ PlayerCharacter ?œê±° ?€?´ë¨¸
	FTimerHandle OutPlayerDestroyTimerHandle;
	//?ˆë½ ??Out???Œë ˆ?´ì–´ ìºë¦­???´ë™ ë³´ê°„
	UPROPERTY(EditAnywhere, Category = "Out")
	float OutVisualSmoothSpeed = 2.f;
	//?Œë ˆ?´ì–´ ì½”ì¸ ?ë“/?ì‹¤------------------------------------------
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
	//?œë²„?ì„œ ?Œë ˆ?´ì–´ ê´€??ì²˜ë¦¬(RPC)----------------------------------------
	//?Œë ˆ?´ì–´ ì½”ì¸ ?ë“
	UFUNCTION(Server, Reliable)
	void Server_AddCoin(int32 CoinValue);
	//?Œë ˆ?´ì–´ ê³µê²©
	UFUNCTION(Server, Reliable)
	void Server_Attack(bool bHolding, FVector ClientAimPoint, FVector ClientAttackDirection);
	UFUNCTION(Server, Reliable)
	void Server_AttackRelease();
	//?Œë ˆ?´ì–´ ?°ë?ì§€ ?ìš©
	UFUNCTION(Server, Reliable)
	void Server_ApplyDamage(float Damage, APlayer_Character* AttackPlayer);
	UFUNCTION(Server, Reliable)
	//?Œë ˆ?´ì–´ ?í˜¸?‘ìš©
	void Server_Interaction();
	UFUNCTION(Server, Reliable)
	//?Œë ˆ?´ì–´ Equipment Drop
	void Server_Drop();
	UFUNCTION(Server, Reliable)
	//?Œë ˆ?´ì–´ Item ?¬ìš©
	void Server_UseItem();
	//?Œë ˆ?´ì–´ ì¡°ì?
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
	//?œë²„???ƒì? ì½”ì¸ ?ì„± ?”ì²­
	UFUNCTION(Server, Reliable)
	void Server_SpawnLostCoin(int32 Amount);
	//?œë²„??ë¬´ê¸° ?°ì† ê³µê²© ?”ì²­ (?ê±°ë¦??ê±°ë¦¬HS)
	UFUNCTION(Server, Reliable)
	void Server_HoldAttack();
	//?ˆë½ ??ë¡œì»¬?ì„œ Out???Œë ˆ?´ì–´ ìºë¦­???´ë™
	UFUNCTION(Client, Reliable)
	void Client_Out();
	//?Œë ˆ?´ì–´ ?”ë©´??ì¶”ê? ?´ë?ì§€ë¥??„ìš°ê¸??œì‘
	UFUNCTION(Client, Reliable)
	void Client_StartAdditionalImage(int32 ImageID);
	//?Œë ˆ?´ì–´ ?”ë©´??? ìˆ??ì¶”ê? ?´ë?ì§€ ?œê±°
	UFUNCTION(Client, Reliable)
	void Client_EndAdditionalImage();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopPhysicsOnKillPlane();
	//? ë‹ˆë©”ì´??---------------------------------------------------
	//?Œë ˆ?´ì–´ ?´ë™ ?ë„ (Animation?ì„œ ?¬ìš©)
	UPROPERTY(Replicated, BlueprintReadOnly)
	float AnimMoveSpeed = 0.0f;
	//?Œë ˆ?´ì–´ ?´ë™ ë°©í–¥ Forward (Animation?ì„œ ?¬ìš©)
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AnimMoveForward = 0.f;
	//?Œë ˆ?´ì–´ ?´ë™ ë°©í–¥ Side (Animation?ì„œ ?¬ìš©)
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AnimMoveSide = 0.f;
	//?Œë ˆ?´ì–´ ?ˆë½ ? ë‹ˆë©”ì´??ì§€?ì‹œê°?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	float OutAnimDuration = 2.f;
	//?¼ë°˜ ?¼ê²© ëª½í?ì£??¬ìƒ
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitReaction(float Damage, bool _bIsOut, bool bApplyRotation, FVector AttackDir, bool bUseNoArmsHitReaction, bool bBigHit = false);
	//? ë‹ˆë©”ì´???¬ìƒ ?????ë„ ?ìš©? ì? ?¬ë? ?¤ì •
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetReactionUseNoArms(bool bUseNoArms);
	//Recover ëª½í?ì£??¬ìƒ
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayRecoverReaction();
	//ë¬´ê¸°/ë¬¼ì²´ë³??¼ë°˜ ? ë‹ˆë©”ì´???¬ìƒ
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAnimationDynamic(UAnimSequence* Sequence, FName SlotName, float BlendInTime, float BlendOutTime, float PlayRate, int32 LoopCount, int32 StartTime);
	//ë¬´ê¸°/ë¬¼ì²´ë³??¹ìˆ˜ ? ë‹ˆë©”ì´???¬ìƒ
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayOverrideMontage(UAnimMontage* Montage, FName StartSection = NAME_None, bool bRestart = true, bool bPauseAfter = false);
	//?¬ë¡¯ ? ë‹ˆë©”ì´???œí€€?? ?•ì?
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StopSlotAnimation(FName SlotName, float BlendOutTime);
	//ëª½í?ì£?? ë‹ˆë©”ì´???•ì?
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StopMontage(UAnimMontage* Montage, float BlendOutTime);
	//BigHit ëª½í?ì£?? ë‹ˆë©”ì´???€??(BigHit ì¤??ˆë½ ?¹ì? BigHit???´ë‹¹?˜ëŠ” ?‰ë°±??ê°€ì§?ê³µê²©?¼ë¡œ ?ˆë½ ??
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HoldBigHitDeathPose();
	//?¼ê²© ?????ê¹Œì§€ ?¬ì¦ˆ???ìš© ?œí‚¬ì§€ ?¬ë? (????Grip, ?¹ìˆ˜ ?¬ì¦ˆ??falseë¡??´ì„œ ???ì? ?œì™¸)
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bHitReactionUseNoArms = true;
	//ê°•í•œ ?¼ê²© ëª½í?ì£?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> BigHittedMontage;
	//ê°•í•œ ?¼ê²©?¼ë¡œ ? ì•„ê°€??ì¤‘ì¸ì§€ ?¬ë?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	bool bHoldBigHittingPose = false;
	//ê°•í•œ ?¼ê²© ?¬ìƒ ?¬ë?
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "BigHit")
	bool bIsBigHitReaction = false;
	//ê°•í•œ ?¼ê²© ê¸°ì?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BigHit")
	float BigHitKnockBackRule = 2000.f;
	//Recover ?¬ìƒ ê¸°ì? ?ë„
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BigHit")
	float RecoverVelocityRule = 50.f;
	//ê°•í•œ?¼ê²©-Recover ?„í™˜ ? ì? ?œê°„
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BigHit")
	float BigHitRecoverStopHoldTime = 0.5f;
	//BigHit ì§í›„ LaunchCharacterê°€ ?ìš©?˜ê¸° ??ë°”ë¡œ Recover ë°©ì? ?œê°„
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BigHIt")
	float BigHitRecoverMinCheckDelay = 0.2f;
	//Recover ëª½í?ì£?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> RecoverMontage;
	//ê°•í•œ ?¼ê²© ??Recover ?¬ìƒ ?¬ë?
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "BigHit")
	bool bIsRecoverReaction = false;
	//?¼ë°˜ ?¼ê²© ëª½í?ì£?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> HittedMontage;
	//?ˆë½ ëª½í?ì£?(bIsOut || Hp <= 0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> OutMontage;
	//?¼ë°˜ ê³µê²© ? ë‹ˆë©”ì´??
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> FirstNormalAttack;
	//?¼ë°˜ ê³µê²© ? ë‹ˆë©”ì´??
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> SecondNormalAttack;
	//?¼ë°˜ ê³µê²© ? ë‹ˆë©”ì´???„í™˜ ê°€???œê°„
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Animation")
	float ChangeNormalAttackAnimationTime = 1.f;
	//?„ì¬ ?¼ë°˜ ê³µê²© ? ë‹ˆë©”ì´???¸ë±??
	UPROPERTY()
	int32 NormalAttackAnimIndex = 0;
	//ìµœê·¼ ?¼ë°˜ ê³µê²© ?œê°„
	UPROPERTY()
	float LastNormalAttackTime = -1.f;
	//?¼ê²© ??ìºë¦­??ë°©í–¥ ?€??
	UPROPERTY()
	FRotator TargetHitRotation;
	//?Œë ˆ?´ì–´ ?´í™??-------------------
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RefreshPersistEffectVisibility();
	UPROPERTY()
	UGameEffectManagerComponent* EffectManagerComp;
	//?¼ë°˜ ê³µê²© ?´í™??
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	FGameEffectData NormalAttackUseEffect;
	//?¼ë°˜ ê³µê²© ?€ê²??´í™??
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	FGameEffectData NormalAttackHitEffect;
	//?Œë ˆ?´ì–´ ?ˆë½ ?´í™??
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TMap<FName, FGameEffectData> OutEffects;
	//?Œë ˆ?´ì–´ ?¤ë²„?ˆì´ ë¨¸í‹°ë¦¬ì–¼------------------------
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PlayerDefaultOverlayMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PlayerDefaultOverlayMID;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetPlayerOverlayMaterialNoShowing();
	/*--------------------------?Œë ˆ?´ì–´ ?¬ìš´??ê´€??---------------------------*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sound")
	TObjectPtr<UPlayerSoundDataAsset> PlayerSoundData;

public:
	//?„ì¬ ë§¤ì¹˜?ì„œ ?Œë ˆ??ì¤‘ì¸ ë§?
	AMapConstructor* NowMap;
	//?Œë ˆ?´ì–´ ì§ì „ ?´ë™ ë°©í–¥
	FVector LastPlayerdir;
	FVector Playerdir;
	//?Œë ˆ?´ì–´ ?„ì ¯ ì´ˆê¸°??
	void InitPlayerWidget();
	//?Œë ˆ?´ì–´ ?„ì ¯ ?¨ê? (?ˆë½ ??
	void SetPlayerWidgetVisibility(bool bVisible);
	//ë§¤ì¹˜ ì¢…ë£Œ ???Œë ˆ?´ì–´??ì¡°ì‘ ?œì–´
	void SetPlayerEndMatchState();
	//?¥ì°© ì¤‘ì¸ Objects/Weapon ?„ì´ì½??ë“
	UTexture2D* GetWidgetEquippmentSlotIcon();
	//?¥ì°© ì¤‘ì¸ Objects/Weapon ?¨ì? ?¬ìš© ?Ÿìˆ˜ ?ë“
	float GetWidgetWeaponSlotPercent();
	//?Œë ˆ?´ì–´ ?´ë™
	void Move(const struct FInputActionValue& inputValue);
	void MoveStop(const FInputActionValue& inputValue);
	void TrySendToServerControlYaw();
	void UpdateMoveFacingFromVelocity(float DeltaTime);
	void ApplyRotation(FVector2D& InputValue, float DeltaTime);
	//?Œë ˆ?´ì–´ ì¹´ë©”???Œì „
	void CamTurn(const struct FInputActionValue& inputValue);
	//?Œë ˆ?´ì–´ ?í”„
	void Player_Jump(const struct FInputActionValue& inputValue);
	//Equipment ?¥ì°© / ë¬¼ì²´ ?í˜¸?‘ìš©
	void Interaction(const FInputActionValue& inputValue);
	void InteractionInternal();
	//?ˆì•½??ë¬´ê¸° ?¥ì°© (?ì )
	void ApplyResevedWeapon();
	//ë¬´ê¸° ?ë™ ?¥ì°©
	void EquipWeaponAuto(TSubclassOf<AWeapon> weapon);
	//?„ì´???¬ìš©
	void UseItem(const struct FInputActionValue& inputValue);
	void UseItemInternal();
	//Equipment ?´ì œ
	void Drop(const FInputActionValue& Value);
	void DropInternal();
	float GetThrowDamageWithWeight(float weight);
	//?Œë ˆ?´ì–´ ê³µê²©
	void Attack(const struct FInputActionValue& inputValue);
	void AttackInternal(bool bPlayAnimation = true);
	void HoldAttack(const struct FInputActionValue& inputValue);
	void AttackRelease(const struct FInputActionValue& inputValue);
	bool AttackLineOfSight(AActor* TargetActor);
	//?Œë ˆ?´ì–´ ?¼ê²© ?‰ë°± ?ìš© ??ê³µì¤‘ Dampen ?ìš©
	void UpdateKnockBackAirDamping(float DeltaTime);
	//?Œë ˆ?´ì–´ ì¡°ì???ì¶”ê?/?œê±°
	void UpdateAimTargetPoint();
	void UpdateAimPoint();
	void HideAimPoint();
	//?Œë ˆ?´ì–´ ì¡°ì? ?„ë¦¬ë·?
	void UpdateAimPreview(float DeltaTime);
	void HideAimPreview();
	bool BuildCurrentAttackPreviewData(FAimPreviewVisualData& OutData);
	//?Œë ˆ?´ì–´ ?Œí”¼
	void Dodge(const struct FInputActionValue& inputValue);
	void DodgeInternal(FVector DodgeDir);
	//?Œë ˆ?´ì–´ ì¡°ì?
	void Aim(const struct FInputActionValue& inputValue);
	void AimStop(const struct FInputActionValue& inputValue);
	//ì¡°ì? ?íƒœ ?´ì œ (?„ë¦¬ë·?ë§ˆìš°???¬ì¸???±ì„ ì¡°ì? ?´ì œ ?íƒœë¡??¤ì •) - AimStop ?´ì˜ ê¸°ëŠ¥
	void CancelAimState();
	void SetAimInternal(bool bAiming);
	void TrySendtoServerAimPoint();
	//ê°€ê¹Œìš´ Equipments ì¤?ê°€??ê°€ê¹Œìš´ ê²ƒì„ ë°˜í™˜
	AEquipment* ClosestEquipment();
	//ê°€ê¹Œìš´ ë¬¼ì²´??ì¤?ê°€??ê°€ê¹Œìš´ ê²ƒì„ ë°˜í™˜
	AObjects* ClosestObjects();
	//ê°?Equipments ?´ì œ
	void DropWeapon(float Strength, bool bIsThrowing);
	void DropItem(float Strength);
	void DropObjects(float Strength, bool bIsThrowing);
	//Equipments ?¥ì°©
	bool PickWeapon(TObjectPtr<AWeapon>Weapon);
	bool PickItem(TObjectPtr<AItem> Item);
	bool PickObjects(TObjectPtr<AObjects> Object);
	//Drop??Equipment???„ì¹˜/?Œì „ ê³„ì‚°
	FTransform DropTransform();
	//?´ë™?˜ë©° Drop???˜ì?ê¸??ìš©
	void ApplyThrow(AEquipment* Equipment, float BaseStrength, float UpStrength, float IgnorePawnSeconds);
	void ApplyThrowOb(AObjects* object, float BaseStrength, float UpStrength, float IgnorePawnSeconds);
	//?Œë ˆ?´ì–´??ë¬´ê¸° ?¤íƒ¯ ?ìš©
	FWeaponStats GetWeaponStat();
	//ë§ˆìš°???¬ì¸???„ì¹˜ ?ë“
	bool GetMousePoint(FVector& MousePoint);
	//ì¡°ì? ??ìºë¦­???Œì „
	void ApplyAimRotation(float DeltaTime);
	//?Œë ˆ?´ì–´ ?Œì „ ê³„ì‚°
	void ApplyPlayerRotation(float TargetYaw, float DeltaTime);
	//?Œë ˆ?´ì–´ ?„ì¬ HP ?ë“
	float GetCurrentHP() const { return HP; }
	//?Œë ˆ?´ì–´ ?°ë?ì§€ ?ìš© ??ì²˜ë¦¬
	float TakeDamage(float damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	//?´ë¼?´ì–¸?¸ê? ?°ë?ì§€ ?ìš©
	float ApplyDamageInternal(float Damage, APlayer_Character* AttackPlayer, AActor* DamageCauser, bool bApplyKnockBack = true, bool bApplyRotation = true, bool bUsingHitAction = true, bool bForceDamage = false, float OverrideKnockBackStrength = -1.f);
	//?Œë ˆ?´ì–´ ?‰ë°±
	void ApplyKnockBack(FVector& AttackDir, float Strength, float UpStrength);
	//ì½”ì¸ ?ì‹¤--------------------------------------------------------------
	void SpawnLostCoins(int32 Amount);
	//ì½”ì¸???´ë™??ëª©ì ì§€ê°€ ??ë¸”ë¡???ìƒ‰ ë°??„ë³´ ? ì •
	bool CollectNearbySafeBlocksFromMap(TArray<FVector>& SafeBlockLocations, int32 instanceSearchRadius, int32 instanceSearchHeight);
	void BuildCoinTargetLocations(int32 RequiredTargetCount, AMapConstructor* CurrentMap, TArray<FVector>& TargetLocations);
	//?ìƒ‰???„ë³´ ì¤?ìµœì¢… ëª©ì ì§€ë¥?? ì •
	FVector GetCoinTargetLocation(TArray<FVector>& SafeBlockLocations, TMap<int32, int32>& UsageCount);
	//?Œí™˜?´ì•¼??ì½”ì¸?¤ì˜ Listë¥??•ë¦¬
	void SpawnCoinList(TArray<TSubclassOf<ACoin>>& CoinList, int32 Amount);
	//------------------------------------------------------------------------
	//?Œë ˆ?´ì–´ ?ˆë½
	void Out(APlayer_Character* WinnerPlayer);
	//?ˆë½???Œë ˆ?´ì–´ ?œê±° ???œë²„??ë¦¬ìŠ¤???”ì²­
	void DestroyPlayer();
	//?Œë ˆ?´ì–´ ?¼ê²© ??ë°©í–¥ ?Œì „
	void TurnToAttackPlayer(const FVector& AttackDir);
	//?Œë ˆ?´ì–´ ?ˆë½ ???¥ì°© ì¤‘ì¸ ?„ì´???•ë³´ ?€??
	void SaveNowItem();
	//?Œë ˆ?´ì–´ ë¦¬ìŠ¤?????€?¥ëœ ?„ì´???•ë³´ ?ë“
	void LoadNowItem();
	//-------------------------------------------------------------------------
	//?Œë ˆ?´ì–´ ?´ë™ ë°©í–¥/?„ì¬ ë°©í–¥ ì°¨ì´ ê³„ì‚° (Animation ?¬ìš©)
	void UpdateAnimationMoveDirectionValues(float DeltaTime);
	//? ë‹ˆë©”ì´???¬ìƒ ???????í–¥ ?¬ë?
	bool ShouldUseNoArmsReaction();
	//?Œë ˆ?´ì–´ ?¼ê²© ? ë‹ˆë©”ì´???¬ìƒ
	void PlayDamageAnimation(float Damage, bool bBigHit);
	//?Œë ˆ?´ì–´ ê¸°ë³¸ ë¬´ê¸°/ë¬¼ì²´ ê³µê²© ? ë‹ˆë©”ì´???¬ìƒ
	void PlayAnimationDynamic(UAnimSequence* Sequence, FName SlotName, float BlendInTime, float BlendOutTime, float PlayRate, int32 LoopCount, int32 StartTime);
	//?„ì‹ /?ì²´ ì¡°ì? ? ë‹ˆë©”ì´???„í™˜/ê°±ì‹ 
	void UpdateAimAnimationSlot();
	//?¬ìƒ??? ë‹ˆë©”ì´?˜ì´ ?´ë–¤ê²ƒì¸ì§€ ?•ì¸
	bool NeedToPlayAllBodyAnimation();
	//ê°??¥ì°©ë¬¼ë“¤???€???¬ìƒ??? ë‹ˆë©”ì´???•ì¸ ë°??¬ìƒ
	bool PlayEquipmentAnimation(EFunctionInterActionReason Reason);
	//? ë‹ˆë©”ì´??Slot ?´ë¦„ ?ë“
	FName GetActionSlotName(bool bUseFullBodyAnim);
	//ë¬´ê¸°ë³?ê³µê²© ëª½í?ì£??ë“
	UAnimMontage* GetCurrentAttackMontague(bool bUseAllBodyAnim);
	//ë¬´ê¸°ë³?ê¸°ë³¸ ? ë‹ˆë©”ì´???ë“
	UAnimSequenceBase* GetCurrentGripSequence();
	//ê¸°ë³¸ ê³µê²© ? ë‹ˆë©”ì´???ë“
	UAnimSequence* GetNormalAttackSequence();
	//ê°??‰ë™ë³?? ë‹ˆë©”ì´??Intercept ?¬ë? ?•ì¸ ë°??ìš©
	bool ApplyAnimationIntercept(EFunctionInterActionReason InterceptorReason, EFunctionInterActionReason TargetReason, const FEquipmentActionAnimation& TargetAnimation, FName TargetSlotName, int32& InOutStartFrame);
	//ê°•í•œ ?¼ê²© ? ë‹ˆë©”ì´???œì‘
	void StartBigHitReaction();
	//ê°•í•œ ?¼ê²© ? ë‹ˆë©”ì´??ê°±ì‹  -> Reaction ? ë‹ˆë©”ì´???œì‘ ê²€??
	void UpdateBigHitReaction(float DeltaTime);
	//Recover ? ë‹ˆë©”ì´???œì‘
	void StartRecoverReaction();
	//Recover ë°?ê°•í•œ ?¼ê²© ? ë‹ˆë©”ì´??ì¢…ë£Œ
	void EndBigHitReaction();
	//?„ì¬ ?¬ìƒì¤‘ì¸ ? ë‹ˆë©”ì´???ë“
	bool GetCurrentEquipmentActionAnimation(EFunctionInterActionReason Reason, FEquipmentActionAnimation& Animation);
	//?¬ìƒ??? ë‹ˆë©”ì´???¬ë¡¯ ? íƒ (?????¬ìš© ?¬ë?)
	FName GetAnimationSlot(APlayer_Character* Player);
	//ê°•í•œ ?¼ê²© ?œì‘ ?œê°„
	float BigHitStartTime = 0.f;
	//ê°•í•œ ?¼ê²© ì¢…ë£Œ ?œê°„
	float BigHitStopTime = 0.f;
	//?„ì‹  ì¡°ì? ? ë‹ˆë©”ì´???¬ìš© ?¬ë?
	bool bUsingFullBodyAimAnimation = false;
	//ê°•í•œ ?¼ê²© ??Recover ?œì‘ ?€?´ë¨¸
	FTimerHandle BigHitRecoverTimerHandle;
	//ê³µê²© ? ë‹ˆë©”ì´???„ë£Œ ??ì¡°ì? ì¤‘ì´ë©?ì¡°ì? ? ë‹ˆë©”ì´???¬ìƒ ?€?´ë¨¸
	FTimerHandle ResumeAimAnimationTimerHandle;
	//ê³µê²© ?íƒœ ì¢…ë£Œ ?€?´ë¨¸ (? ë‹ˆë©”ì´??ê¸°ì?)
	FTimerHandle EndAttackStateTimerHandle;
	//BigHit ì¢…ë£Œ ???¬ì¦ˆ ?•ì? ?€?´ë¨¸ (BigHit ?ˆë½ ???¬ìš©)
	FTimerHandle BigHitDeathPauseTimerHandle;
	/*---------------------------?Œë ˆ?´ì–´ ?´í™??ì²˜ë¦¬----------------------------*/
	void PlayNormalAttackHitEffect(AActor* Target, const FHitResult& AttackHit);
	void PlayAttackEffectByNotify();
	bool ShouldHideEffectsFromOtherPlayer();
	bool ShouldShowGameEffectForThisClient(const FGameEffectData& EffectData);
	/*---------------------------?Œë ˆ?´ì–´ ?¤ë²„?ˆì´ ë¨¸í‹°ë¦¬ì–¼ ê´€??----------------*/
	void SetPlayerOverlayOpacityZero_Local();
	void SearchPlayerDefaultOverlayMaterial();
public:
	//?Œë ˆ?´ì–´ ?œì–´ ê´€??/
	//*----------------------------------
	void EquippmentLockActivateForEvent(TSubclassOf<AActor> TargetClass, bool bEnable);
	void EquipLockedEquipment(TSubclassOf<AActor> EquipmentClass, bool bApplyFixUseCount, bool bDestroyOnClear);
	void ClearLockedEquipment(bool bForceDestroyFixEquipment);

	bool CheckHavingLockedEquipment(AActor* Actor);
	//*--------------------------------
	//?„ì¬ ?Œë ˆ?´ì–´??Player_State
	TObjectPtr<APlayer_State> NowPlayer_State;
	//PlayerState ë°”ì¸??
	void BindPlayer_State();
	//?‰ë™ë³?Condition ?œì–´
	void NotifyConditionEvent(EPlayerConditionEvent Event, bool bUseEndEffect = true);
	//?´ë™???…ë ¥ ?¬ë?
	bool bMoveInputHolding = false;
	void SetMaintainMoveOnNotInput(bool bEnable, float InNoInputMoveScale = 0.5f);
	void UpdateMaintainMoveOnNotInput(float DeltaTime);
	//?Œí”¼ ?íƒœ
	float LastDodgeTime = -5.f;
	float SavedGroundFriction = 0.f;
	float SavedBrakingFrictionFactor = 0.f;
	float SavedBrakingDecel = 0.f;
	float SavedBrakingDecelFalling = 0.f;
	float SavedFallingLateralFriction = 0.f;
	//?Œë ˆ?´ì–´ ?í”„ ì¿¨í???
	float JumpCoolTime = 0.2f;
	float LastJumpTime = -1.f;
	//?Œë ˆ?´ì–´ ê³µê²© ?íƒœ
	float LastAttackTime = -5.f;
	//(?€??ê³µê²© ?? ?€??ê³µê²© ?¬ì‹œ?‘ì´ ?„ìš”?œì? ?•ì¸
	bool bNowHoldingAttack = false;
	//?Œë ˆ?´ì–´ Equipment ?¥ì°© ì¿¨í???
	float EquipCoolTime = 0.5f;
	float LastEquipTime = -5.f;
	//?Œë ˆ?´ì–´ Equipment ?´ì œ ì¿¨í???
	float UnEquipCoolTime = 0.5f;
	float LastUnEquipTime = -5.f;
	//?Œë ˆ?´ì–´???˜ì?????
	float PutStrength = 30.f;
	float MoveStrength = 100.f;
	float ThrowStrength = 1000.f;
	//?Œë ˆ?´ì–´ ?„ì¬ Stat
	FWeaponStats AStat;
	float WeightPenalty();
	float CalculateSpeed(float Default_Speed = -1.f);
	void UpdateMoveSpeed();
	//?ë„ ì¡°ì •??ë°˜ì˜
	void AddSpeedController(FName ControllerName, float Magnification, float offset, bool bConstantSpeed = false, int32 Priority = 0);
	//?ë„ ì¡°ì •???œê±°
	void RemoveSpeedControllerByName(FName ControllerName);
	void RemoveSpeedControllerByPriority(int32 Priority);
	//ë©´ì—­ ì¡°ì •??ë°˜ì˜
	void AddImmunityController(FName ControllerName, EPlayerImmunityType type, int32 Priority = 0, bool bCanEraseForce = false, float Duration = 0.f);
	void RemoveImmunityControllerByName(FName ControllerName, bool EraseForce = false);
	void RemoveImmunityControllerByPriority(int32 priority);
	void RemoveImmunityControllerByType(EPlayerImmunityType type);
	void RefreshImmunityConditionEffects(EPlayerImmunityType type);
	//ë©´ì—­ ì¡°ì •???•ì¸
	bool HavingImmunity(EPlayerImmunityType type);
	bool HavingDamageImmunity();
	bool HavingDebuffImmunity();
	FName GetConditionNameByImmunityType(EPlayerImmunityType type);
	//?Œë ˆ?´ì–´???„ì¬ ?íƒœ ?°ì´?°ë? ?œë²„?ì„œ ?ë“
	APlayer_State* GetThePlayerState();
	//?Œë ˆ?´ì–´???„ì¬ ?íƒœ ?°ì´?°ë? ë°˜ì˜
	virtual void OnRep_PlayerState() override;
	FTimerHandle InitPlayerWidgetRetryTimerHandle;
	//?Œë ˆ?´ì–´ ì²´ë ¥ ë³€ê²?
	void HPChange(float HPAmount);
	//?Œë ˆ?´ì–´ ?…ë ¥ ê°€???íƒœ ë³€ê²?(BlockController ?¬ìš©)
	//BlockController ì¶”ê?
	void AddInputBlockController(FName ControllerName, bool bBlockMove, bool bBlockCamera, bool bStopMovementOnAdd = true, bool bIsOnLiquid = false);
	//BlockController ?œê±°
	void RemoveInputBlockController(FName ControllerName);
	//?„ì¬ ì¡´ì¬?˜ëŠ” BlockController?¤ì— ë§ê²Œ ?´ë™, ì¹´ë©”??ì¡°ì‘ ?œí•œ ?•ì¸
	void RefreshInputBlockState(bool bStopMovementOnBlock = true, bool bIsOnLiquid = false);
	//?¤ì œ ì¡°ì‘ ?œí•œ ê¸°ëŠ¥
	void ApplyInputBlockInternal(bool bIsOnLiquid);
	//?Œë ˆ?´ì–´ ?¬ë§ ?íƒœ ?•ì¸
	bool IsOut() { return bIsOut; }
	//?Œë ˆ?´ì–´ ì¡°ì? ??
	FVector LastAimPoint = FVector::ZeroVector;
	//?Œë ˆ?´ì–´ ë¹„ì¡°ì¤€ ê³µê²© ë°©í–¥
	FVector BuildAttackAimPointForCurrentState();
	//?Œë ˆ?´ì–´ ì¡°ì? ?œê°„
	float LastAimTime = -1.f;
	//?Œë ˆ?´ì–´ ?Œì „ ?œê°„
	float LastTurnTime = 0.f;
	float LastSenttoServerYaw = 0.f;
	FTimerHandle HittedResetTimerHandle;
	//?Œë ˆ?´ì–´ ì½”ì¸ ?ì‹¤ ?œê°„
	float LastLoseCoinHP = -1.f;
	//ì§ì „ ê³µê²© ?Œë ˆ?´ì–´ ?¤ì •
	APlayer_Character* LastAttackPlayer;
	FTimerHandle HoldLastAttackPlayer;
	void ClearLastAttackPlayer() { LastAttackPlayer = nullptr; }
	//?Œë ˆ?´ì–´ ?ˆë½ ???¡ì²´???ˆëŠ”ì§€ ?•ì¸
	bool bIsOnLiquidWhenOut = false;
	//?Œë ˆ?´ì–´ ?ˆë½ ???¡ì²´???ˆë‹¤ë©??„ì¹˜ ë³´ì™„ (?ˆí•˜ë©??Šê²¨??ë³´ì„)
	bool bOutVisualSmoothing = false;
	//?Œë ˆ?´ì–´ ?ˆë½ ??SaveEquipmentëª¨ë“œ?¼ë©´ ?„ì¬ ?¥ì°©ì¤‘ì¸ Equipment ?´ë˜???€??
	void SaveCurrentEquipmentClass();
	//?Œë ˆ?´ì–´ ë¶€????SaveEquipmentê°€ ?ˆë‹¤ë©?ë°”ë¡œ ?¥ì°©
	void EquipSavedEquipmentAfterRespawn();

	bool CheckWeaponInteraction(EFunctionInterActionReason Reason);
	bool IsCurrentWeaponInputType(EWeaponAttackInputType InputType);
	bool IsCurrentWeaponHoldLikeAttack();

	FVector DefaultMeshLocation = FVector::ZeroVector;
	FVector LastActorLocation = FVector::ZeroVector;
	FVector VisualMeshLocation = FVector::ZeroVector;
	FVector DefaultCamLocation = FVector::ZeroVector;
	FVector VisualCamLocation = FVector::ZeroVector;
};
