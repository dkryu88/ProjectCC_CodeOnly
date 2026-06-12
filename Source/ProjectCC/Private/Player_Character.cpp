// Fill out your copyright notice in the Description page of Project Settings.


#include "Player_Character.h"
#include <GameFramework/SpringArmComponent.h>
#include <Camera/CameraComponent.h>
#include <GameFramework/CharacterMovementComponent.h>
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Algo/RandomShuffle.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "ETC/AttackPreviewGuide.h"
#include "InputActionValue.h"
#include "AllPlayMode_GameInstance.h"
#include "PlayerConditionDataAsset.h"
#include "PlayerConditionComponent.h"
#include "PlayerTransformationComponent.h"
#include "PlayerVisualManagerComponent.h"
#include "PlayerTransformations.h"
#include "Match_PlayerController.h"
#include "Player_CharacterWidget.h"
#include "Player_AdditionalWidget.h"
#include "PlayMode_Match.h"
#include "EffectManagerComponent.h"
#include "MapConstructor.h"
#include "BlockType.h"
#include "PlayerStats.h"
#include "Equipment.h"
#include "Weapon.h"
#include "ObjectsDataAsset.h"
#include "Item.h"
#include "Coin.h"
#include "KillPlane.h"
#include "Objects.h"
//디버그 헤더
#include "DrawDebugHelpers.h"

// Sets default values
APlayer_Character::APlayer_Character()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	//카메라가 위치할 springArm 컴포넌트
	springArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	springArmComp->SetupAttachment(RootComponent);
	springArmComp->SetRelativeLocation(FVector(0, 0, 350));
	springArmComp->TargetArmLength = 350;
	springArmComp->bEnableCameraLag = true;
	springArmComp->CameraLagSpeed = 15.f;
	springArmComp->CameraLagMaxDistance = 100.f;
	springArmComp->bEnableCameraRotationLag = true;
	springArmComp->CameraRotationLagSpeed = 15.f;
	//간헐적으로 카메라가 플레이어와 부딪혀 튀는 것을 방지
	springArmComp->bDoCollisionTest = false;
	//컨트롤러(마우스) 기반 카메라(SpringArm) 회전
	springArmComp->bUsePawnControlRotation = true;
	//SpringArm에 카메라를 부착
	playerCamComp = CreateDefaultSubobject<UCameraComponent>(TEXT("TpsCamComp"));
	playerCamComp->SetupAttachment(springArmComp);
	playerCamComp->SetRelativeRotation(FRotator(-50, 0, 0));
	playerCamComp->AspectRatio = 1920.f / 1080.f;
	playerCamComp->bConstrainAspectRatio = true;
	//컨트롤러(마우스) 기반 플레이어 회전 권한 제거
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	//플레이어 이동 컴포넌트 설정
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp) {
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->BrakingDecelerationWalking = 2048.f;
		MoveComp->MaxWalkSpeed = BaseStats.Default_Speed;
		MoveComp->RotationRate = FRotator(0.f, 720.f, 0.f);
		MoveComp->AirControl = 0.8f;
		MoveComp->GravityScale = 2.f;
		MoveComp->BrakingDecelerationFalling = 3000.f;
		MoveComp->FallingLateralFriction = 5.f;
		MoveComp->JumpZVelocity = 700.f;
		//플레이어 캐릭터가 바닥에 걸치는 것을 방지
		MoveComp->bCanWalkOffLedges = true;
		MoveComp->PerchRadiusThreshold = 20.f;
		MoveComp->PerchAdditionalHeight = 0.0f;
		//플레이어의 미는 힘 조절 (initialpush 최초 접촉/push 지속)
		MoveComp->bEnablePhysicsInteraction = true;
		MoveComp->InitialPushForceFactor = 1500.0f;
		MoveComp->PushForceFactor = 1800.0f;
		MoveComp->TouchForceFactor = 500.0f;
	}
	//상호작용 감지 Collision을 부착
	PickupDetectRange = CreateDefaultSubobject<UBoxComponent>(TEXT("Interaction Detection"));
	PickupDetectRange->SetupAttachment(GetRootComponent());
	//상호작용 감지 Collison 크기 조정
	PickupDetectRange->InitBoxExtent(FVector(50, 50, 90));
	//상호작용 감지 Collision 설정
	PickupDetectRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupDetectRange->SetCollisionObjectType(ECC_GameTraceChannel1);
	PickupDetectRange->SetCollisionResponseToAllChannels(ECR_Ignore);
	//ECC_GameTraceChannel1 <- 콜리전 프리셋 1번 (Interaction)
	PickupDetectRange->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	PickupDetectRange->SetGenerateOverlapEvents(true);
	//아이템 슬롯 컴포넌트 부착
	ItemSlot = CreateDefaultSubobject<USceneComponent>(TEXT("ItemSlot"));
	ItemSlot->SetupAttachment(GetRootComponent());
	//물체 슬롯 컴포넌트 부착
	ObjectsSlot = CreateDefaultSubobject<USceneComponent>(TEXT("ObjectsSlot"));
	ObjectsSlot->SetupAttachment(GetRootComponent());
	//서포트 슬롯 컴포넌트 부착
	SupportSlot = CreateDefaultSubobject<USceneComponent>(TEXT("SupportSlot"));
	SupportSlot->SetupAttachment(GetRootComponent());
	//플레이어 Condition 컴포넌트 부착
	ConditionComp = CreateDefaultSubobject<UPlayerConditionComponent>(TEXT("Player Condition"));
	//플레이어 변신 컴포넌트 부착
	TransformationComp = CreateDefaultSubobject<UPlayerTransformationComponent>(TEXT("Player Transformation"));
	//플레이어 비주얼 컴포넌트 부착
	VisualManagerComp = CreateDefaultSubobject<UPlayerVisualManagerComponent>(TEXT("Player VisualManager"));
	//플레이어 위젯
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("Player Widget"));
	WidgetComponent->SetupAttachment(RootComponent);
	WidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetDrawAtDesiredSize(false);
	WidgetComponent->SetDrawSize(FVector2D(180.f, 60.f));
	WidgetComponent->SetManuallyRedraw(false);
	WidgetComponent->SetRedrawTime(0.03f);
	//플레이어 이펙트 컴포넌트 부착
	EffectManagerComp = CreateDefaultSubobject<UEffectManagerComponent>(TEXT("EffectManager"));

	//플레이어 스탯 초기값 설정
	AStat = GetWeaponStat();
	HP = BaseStats.Max_HP;
	move_Speed = BaseStats.Default_Speed;
	Weight = BaseStats.Default_Weight;
	LastLoseCoinHP = HP;
}

void APlayer_Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayer_Character, HP);
	DOREPLIFETIME(APlayer_Character, bIsOut);
	DOREPLIFETIME(APlayer_Character, bIsAiming);
	DOREPLIFETIME(APlayer_Character, bIsDodging);
	DOREPLIFETIME(APlayer_Character, bIsHitted);
	DOREPLIFETIME(APlayer_Character, bEndMatchState);
	DOREPLIFETIME(APlayer_Character, bCanControl);
	DOREPLIFETIME(APlayer_Character, bCanCamControl);
	DOREPLIFETIME(APlayer_Character, AnimMoveSpeed);
	DOREPLIFETIME(APlayer_Character, move_Speed);
	DOREPLIFETIME(APlayer_Character, Aim_TurnSpeed);
	DOREPLIFETIME(APlayer_Character, Weight);

	DOREPLIFETIME(APlayer_Character, NowWeapon);
	DOREPLIFETIME(APlayer_Character, NowItem);
	DOREPLIFETIME(APlayer_Character, NowObjects);
	DOREPLIFETIME(APlayer_Character, NowSupport);
	DOREPLIFETIME(APlayer_Character, ServerAimPoint);

	DOREPLIFETIME(APlayer_Character, bIsBigHitReaction);
	DOREPLIFETIME(APlayer_Character, bIsRecoverReaction);
}

void APlayer_Character::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitPlayerWidget();
	BindPlayer_State();

	if (VisualManagerComp) {
		VisualManagerComp->RefreshPortraitMaterials();
	}
}

void APlayer_Character::PawnClientRestart() {
	Super::PawnClientRestart();
	InitPlayerWidget();

	// [사운드] 카메라에 붙어있던 귀(이어폰)를 플레이어 위치로 옮김(사운드 보간의 거리를 알맞게 설정하기 위한 기준)
	if (APlayerController* PC = Cast<APlayerController>(GetController())) {
		PC->SetAudioListenerOverride(GetCapsuleComponent(), FVector::ZeroVector, FRotator::ZeroRotator);
	}
}

void APlayer_Character::OnRep_MoveSpeed()
{
	if (GetCharacterMovement()) {
		GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.f, move_Speed);
	}
}

// Called when the game starts or when spawned
void APlayer_Character::BeginPlay()
{
	Super::BeginPlay();
	
	InitPlayerWidget();
	BindPlayer_State();

	if (VisualManagerComp) {
		VisualManagerComp->RefreshPortraitMaterials();
	}
	if (GetMesh()) {
		DefaultMeshLocation = GetMesh()->GetRelativeLocation();
		//잔상 제거
		GetMesh()->SetBoundsScale(1.5f);
	}
	if (springArmComp)
	{
		DefaultCamLocation = springArmComp->GetRelativeLocation();
	}

	if (HasAuthority()) {
		if (APlayMode_Match* GM = GetWorld()->GetAuthGameMode<APlayMode_Match>()) {
			NowMap = GM->GetCurrentMap();
		}
	}	
}

// Called every frame
void APlayer_Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Out된 로컬 플레이어 캐릭터 Mesh 위치를 서버와의 차이만큼 보정
	if (bOutVisualSmoothing && IsLocallyControlled()) {
		FVector CurrentActorLocation = GetActorLocation();
		FVector ActorLocationChange = CurrentActorLocation - LastActorLocation;
		if (GetMesh()) {
			VisualMeshLocation -= ActorLocationChange * 1.f;
			VisualMeshLocation = FMath::VInterpTo(VisualMeshLocation, FVector::ZeroVector, DeltaTime, OutVisualSmoothSpeed);
			
		}
		GetMesh()->SetRelativeLocation(DefaultMeshLocation + VisualMeshLocation);
		if (springArmComp) {
			VisualCamLocation -= ActorLocationChange * 1.f;
			VisualCamLocation = FMath::VInterpTo(VisualCamLocation, FVector::ZeroVector, DeltaTime, OutVisualSmoothSpeed);
			
		}
		springArmComp->SetRelativeLocation(DefaultCamLocation + VisualCamLocation);
		LastActorLocation = CurrentActorLocation;
	}
	if (IsLocallyControlled()) {
		TrySendToServerControlYaw();
	}
	if (IsLocallyControlled() && bIsAiming) {
		UpdateAimTargetPoint();
		TrySendtoServerAimPoint();
		ApplyAimRotation(DeltaTime);
		UpdateAimPoint();
		UpdateAimPreview(DeltaTime);
	}
	if (HasAuthority()) {

		if (!bIsAiming && !bIsDodging) {
			//로컬 플레이어는 이미 Move에서 회전했으므로 중복 방지
			if(!IsLocallyControlled()){
				if (bHavingServerMoveFacingYaw) {
					ApplyPlayerRotation(ServerMoveFacingYaw, DeltaTime);
				}
				else {
					UpdateMoveFacingFromVelocity(DeltaTime);
				}
			}
		}
		if (bIsAiming && !IsLocallyControlled()) {
			ApplyAimRotation(DeltaTime);
		}
	}
	
	if (HasAuthority()) {
		UpdateBigHitReaction(DeltaTime);
		UpdateKnockBackAirDamping(DeltaTime);
	}

	UpdateMaintainMoveOnNotInput(DeltaTime);

	//애니메이션 이동 방향 계산 및 적용
	UpdateAnimationMoveDirectionValues(DeltaTime);
}

// Called to bind functionality to input
void APlayer_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//키 바인딩
	auto PlayerInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (PlayerInput) {
		PlayerInput->BindAction(ia_Move, ETriggerEvent::Triggered, this, &APlayer_Character::Move);
		PlayerInput->BindAction(ia_Move, ETriggerEvent::Completed, this, &APlayer_Character::MoveStop);
		PlayerInput->BindAction(ia_Move, ETriggerEvent::Canceled, this, &APlayer_Character::MoveStop);
		PlayerInput->BindAction(ia_CamTurn, ETriggerEvent::Triggered, this, &APlayer_Character::CamTurn);
		PlayerInput->BindAction(ia_Jump, ETriggerEvent::Started, this, &APlayer_Character::Player_Jump);
		PlayerInput->BindAction(ia_dodge, ETriggerEvent::Started, this, &APlayer_Character::Dodge);
		PlayerInput->BindAction(ia_Interaction, ETriggerEvent::Started, this, &APlayer_Character::Interaction);
		PlayerInput->BindAction(ia_EquipmentDrop, ETriggerEvent::Started, this, &APlayer_Character::Drop);
		PlayerInput->BindAction(ia_Attack, ETriggerEvent::Started, this, &APlayer_Character::Attack);
		PlayerInput->BindAction(ia_Attack, ETriggerEvent::Triggered, this, &APlayer_Character::HoldAttack);
		PlayerInput->BindAction(ia_Attack, ETriggerEvent::Completed, this, &APlayer_Character::AttackRelease);
		PlayerInput->BindAction(ia_Attack, ETriggerEvent::Canceled, this, &APlayer_Character::AttackRelease);
		PlayerInput->BindAction(ia_ItemUse, ETriggerEvent::Started, this, &APlayer_Character::UseItem);
		PlayerInput->BindAction(ia_Aim, ETriggerEvent::Started, this, &APlayer_Character::Aim);
		PlayerInput->BindAction(ia_Aim, ETriggerEvent::Completed, this, &APlayer_Character::AimStop);
		PlayerInput->BindAction(ia_Aim, ETriggerEvent::Canceled, this, &APlayer_Character::AimStop);
	}

}

void APlayer_Character::AddInputBlockController(FName ControllerName, bool bBlockMove, bool bBlockCamera, bool bStopMovementOnAdd, bool bIsOnLiquid)
{
	if (!HasAuthority()) return;
	if (ControllerName.IsNone()) return;
	
	//같은 이름의 컨트롤러는 갱신
	RemoveInputBlockController(ControllerName);

	FInputBlockController NewController;
	NewController.BlockControllerName = ControllerName;
	NewController.bBlockMove = bBlockMove;
	NewController.bBlockCamera = bBlockCamera;

	BlockControllers.Add(NewController);

	RefreshInputBlockState(bStopMovementOnAdd, bIsOnLiquid);
}

void APlayer_Character::RemoveInputBlockController(FName ControllerName)
{
	if (!HasAuthority()) return;
	if (ControllerName.IsNone()) return;

	for (auto IT = BlockControllers.CreateIterator(); IT; ++IT) {
		if (IT->BlockControllerName == ControllerName) {
			IT.RemoveCurrent();
		}
	}

	RefreshInputBlockState(false, false);
}

void APlayer_Character::RefreshInputBlockState(bool bStopMovementOnBlock, bool bIsOnLiquid)
{
	if (!HasAuthority()) return;
	bool bPrevCanControl = bCanControl;
	bool bShouldBlockMove = false;
	bool bShouldBlockCam = false;

	for (const FInputBlockController& controller : BlockControllers) {
		if (controller.bBlockMove) {
			bShouldBlockMove = true;
		}
		if (controller.bBlockCamera) {
			bShouldBlockCam = true;
		}
		if (bShouldBlockCam && bShouldBlockMove) {
			break;
		}
	}

	bCanControl = !bShouldBlockMove;
	bCanCamControl = !bShouldBlockCam;

	if (bPrevCanControl && !bCanControl && bStopMovementOnBlock) {
		ApplyInputBlockInternal(bIsOnLiquid);
	}

	ForceNetUpdate();
}

void APlayer_Character::OnRep_CanControl()
{
	if (!bCanControl)
	{
		LastPlayerdir = FVector::ZeroVector;
		Playerdir = FVector::ZeroVector;
		bMoveInputHolding = false;
		AnimMoveSpeed = 0.f;
	}
}

void APlayer_Character::OnRep_HP()
{
	OnHPChanged.Broadcast(HP, BaseStats.Max_HP);
}

void APlayer_Character::OnRep_bIsOut()
{
	SetPlayerWidgetVisibility(false);
}

void APlayer_Character::OnRep_NowWeapon()
{
	AStat = GetWeaponStat();
	OnWeaponChanged.Broadcast();
}

void APlayer_Character::OnRep_NowObjects()
{
	AStat = GetWeaponStat();
	OnWeaponChanged.Broadcast();
}

void APlayer_Character::HandlePortraitIdChanged(int32 NewPortraitId)
{
	if (VisualManagerComp) {
		VisualManagerComp->RefreshPortraitMaterials();
	}
}

void APlayer_Character::ApplyInputBlockInternal(bool bIsOnLiquid)
{
	LastPlayerdir = FVector::ZeroVector;
	Playerdir = FVector::ZeroVector;
	bMoveInputHolding = false;
	AnimMoveSpeed = 0.f;

	if (GetCharacterMovement()) {
		if (bIsOnLiquid) {
			bIsOnLiquidWhenOut = bIsOnLiquid;
			GetCharacterMovement()->GravityScale = 0.f;
			GetCharacterMovement()->Velocity = FVector(0.f, 0.f, bEndMatchState ? -SinkSpeed : 0.f);
		}
		else {
			GetCharacterMovement()->Velocity = FVector(0.f, 0.f, GetCharacterMovement()->Velocity.Z);
		}
	}
}

void APlayer_Character::ApplyPlayerRotation(float TargetYaw, float DeltaTime)
{
	FRotator Current = GetActorRotation();
	FRotator Target(0.f, TargetYaw, 0.f);

	float Speed = turn_Speed / FMath::Max(1.f, Aim_TurnSpeed * 1.5f);

	FRotator NewRotation = FMath::RInterpTo(Current, Target, DeltaTime, Speed);

	if (!NewRotation.Equals(Current, 0.1f)) {
		SetActorRotation(NewRotation);
		if (HasAuthority()) {
			ForceNetUpdate();
		}
	}
	
}

void APlayer_Character::UpdateAnimationMoveDirectionValues(float DeltaTime)
{
	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.f;

	float Speed = HorizontalVelocity.Size();
	AnimMoveSpeed = Speed;

	float AnimTurnSpeed = turn_Speed / FMath::Max(1.f, Aim_TurnSpeed * 1.5f);

	if (Speed < 5.f) {
		AnimMoveForward = FMath::FInterpTo(AnimMoveForward, 0.f, DeltaTime, AnimTurnSpeed /*보간 속도*/);
		AnimMoveSide = FMath::FInterpTo(AnimMoveSide, 0.f, DeltaTime, AnimTurnSpeed);
		return;
	}

	FVector MoveDir = HorizontalVelocity.GetSafeNormal();

	if (IsLocallyControlled() && !LastPlayerdir.IsNearlyZero(0.1f)) {
		MoveDir = LastPlayerdir.GetSafeNormal();
	}

	FRotator AnimRotation = GetActorRotation();
	if (bIsAiming) {
		FVector ToAimPoint = ServerAimPoint - GetActorLocation();
		ToAimPoint.Z = 0.f;

		if (!ToAimPoint.IsNearlyZero()) {
			float TargetYaw = ToAimPoint.Rotation().Yaw;
			FRotator Current = GetActorRotation();
			FRotator Target(0.f, TargetYaw, 0.f);

			AnimRotation = FMath::RInterpTo(Current, Target, DeltaTime, AnimTurnSpeed);
		}
	}
	else {
		if (!MoveDir.IsNearlyZero()) {
			float TargetYaw = MoveDir.Rotation().Yaw;
			FRotator Current = GetActorRotation();
			FRotator Target(0.f, TargetYaw, 0.f);

			AnimRotation = FMath::RInterpTo(Current, Target, DeltaTime, AnimTurnSpeed);
		}
	}

	FVector Forward = FRotationMatrix(AnimRotation).GetUnitAxis(EAxis::X);
	FVector Side = FRotationMatrix(AnimRotation).GetUnitAxis(EAxis::Y);

	Forward.Z = 0.f;
	Side.Z = 0.f;

	//방향 벡터와 앞벡터/옆벡터의 내적(Cos(@)) 계산
	float TargetForward = FVector::DotProduct(Forward, MoveDir);
	float TargetSide = FVector::DotProduct(Side, MoveDir);

	if (!bIsAiming) {
		TargetForward = 1.f;
		TargetSide = 0.f;
	}

	AnimMoveForward = FMath::FInterpTo(AnimMoveForward, TargetForward, DeltaTime, AnimTurnSpeed);
	AnimMoveSide = FMath::FInterpTo(AnimMoveSide, TargetSide, DeltaTime, AnimTurnSpeed);

}

bool APlayer_Character::CheckWeaponInteraction(EFunctionInterActionReason Reason)
{
	if (!NowWeapon) return true;
	if (!NowWeapon->WeaponData) return true;

	return NowWeapon->InteractionWeaponFunction(Reason);
}

bool APlayer_Character::IsCurrentWeaponInputType(EWeaponAttackInputType InputType)
{
	if (!NowWeapon) return false;

	FWeaponStats* Stat = NowWeapon->GetWeaponStats();
	if (!Stat) return false;

	return Stat->AttackInputType == InputType;
}

bool APlayer_Character::IsCurrentWeaponHoldLikeAttack()
{
	if (!NowWeapon) return false;

	FWeaponStats* Stat = NowWeapon->GetWeaponStats();
	if (!Stat) return false;

	return Stat->AttackInputType == EWeaponAttackInputType::Continuous || Stat->AttackInputType == EWeaponAttackInputType::Repeat;
}

//카메라 회전 설정
void APlayer_Character::CamTurn(const struct FInputActionValue& inputValue) {

	float Value = inputValue.Get<float>();
	
	if (!bCanCamControl) return;
	if (!bIsAiming) {
		float value = inputValue.Get<float>();
		AddControllerYawInput(value * Camturn_Speed);
	}
}

void APlayer_Character::InitPlayerWidget()
{
	if (!WidgetComponent || !PlayerHeadWidget) return;

	WidgetComponent->SetWidgetClass(PlayerHeadWidget);
	WidgetComponent->InitWidget();

	if (UPlayer_CharacterWidget* Widget = Cast<UPlayer_CharacterWidget>(WidgetComponent->GetUserWidgetObject())) {
		Widget->InitWidget(this);
	}

	WidgetComponent->SetVisibility(true);

	if (APlayerController* LocalPC = UGameplayStatics::GetPlayerController(this, 0)) {
		if (AMatch_PlayerController* MatchPC = Cast<AMatch_PlayerController>(LocalPC)) {
			if (MatchPC->IsWaitingRespawn() && !IsLocallyControlled()) {
				WidgetComponent->SetVisibility(false);
			}
		}
	}
}

void APlayer_Character::SetPlayerWidgetVisibility(bool bVisible)
{
	if (WidgetComponent) {
		WidgetComponent->SetVisibility(bVisible);
	}
}

void APlayer_Character::SetPlayerEndMatchState()
{
	if (!HasAuthority() || bEndMatchState) return;

	bEndMatchState = true;
	bIsAiming = false;
	bIsHitted = false;
	bIsDodging = false;
	bNowHoldingAttack = false;
	bHavingCurrentAimTargetPoint = false;
	CurrentAimTargetPoint = FVector::ZeroVector;

	if (AimPoint) AimPoint->SetVisibility(ESlateVisibility::Hidden);

	if (ConditionComp) {
		ConditionComp->RemoveLowPriorityCondition(10, true);
	}
	if (TransformationComp && TransformationComp->IsTransformed()) {
		TransformationComp->StopTransformation(false);
	}

	GetWorldTimerManager().ClearTimer(OutPlayerDestroyTimerHandle);
	GetWorldTimerManager().ClearTimer(HoldLastAttackPlayer);

	ForceNetUpdate();
}

//장착 중인 Weapon 아이콘 획득
UTexture2D* APlayer_Character::GetWidgetEquippmentSlotIcon()
{
	if (NowWeapon && NowWeapon->WeaponData) {
		return NowWeapon->WeaponData->WeaponIcon;
	}
	else if (NowObjects && NowObjects->ObjectsData) {
		return NowObjects->ObjectsData->ObjectsIcon;
	}
	return nullptr;
}
//장착 중인 Weapon 사용 횟수 획득
float APlayer_Character::GetWidgetWeaponSlotPercent()
{
	if (NowWeapon && NowWeapon->WeaponData && NowWeapon->WeaponData->MaxUseCount > 0) {
		if (NowWeapon->NowUseCount < 0) return 0.f;
		return (float)NowWeapon->NowUseCount / (float)NowWeapon->WeaponData->MaxUseCount;
	}

	return 0.f;
}

//플레이어 이동
void APlayer_Character::Move(const struct FInputActionValue& inputValue) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;
	if (move_Speed <= 0.f) return;
	if (TransformationComp && !TransformationComp->CanMoveDuringTransfomation()) return;
	
	if (NowWeapon && !NowWeapon->InteractionWeaponFunction(EFunctionInterActionReason::Move)) return;

	FVector2D value = inputValue.Get<FVector2D>();
	value = value.GetSafeNormal();

	if (value.SizeSquared() < 0.05f) {
		AnimMoveSpeed = 0.f;
		return;
	}

	bMoveInputHolding = true;

	//액터 기준 회전 방향 획득
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	FRotator ControlYawRot(0.f, PC->GetControlRotation().Yaw, 0.f);
	FVector Forward = FRotationMatrix(ControlYawRot).GetUnitAxis(EAxis::X);
	FVector Right = FRotationMatrix(ControlYawRot).GetUnitAxis(EAxis::Y);

	Forward.Z = 0.f;
	Right.Z = 0.f;
	Forward = Forward.GetSafeNormal();
	Right = Right.GetSafeNormal();

	//플레이어 위치 이동
	Playerdir = (Forward * value.X) + (Right * value.Y);
	Playerdir = Playerdir.GetSafeNormal();
	if (Playerdir.IsNearlyZero()) {
		AnimMoveSpeed = 0.f;
		return;
	}
	AddMovementInput(Playerdir, 1.f);
	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.f;
	if (HorizontalVelocity.IsNearlyZero()) {
		AnimMoveSpeed = 0.f;
		return;
	}
	AnimMoveSpeed = HorizontalVelocity.Size();

	//플레이어 방향 변경
	if (!bIsAiming && !Playerdir.IsNearlyZero(0.05f)) {
		float TargetYaw = Playerdir.Rotation().Yaw;
		ApplyPlayerRotation(TargetYaw, GetWorld()->GetDeltaSeconds());
		//로컬 캐릭터의 경우 서버에 자신의 회전 값을 전송 및 반영
		if (!HasAuthority()) {
			Server_SetMoveFacingYaw(TargetYaw);
		}
		else {
			ServerMoveFacingYaw = TargetYaw;
			bHavingServerMoveFacingYaw = true;
		}
	}
	//플레이어 직전 방향 저장
	LastPlayerdir = Playerdir;
	Playerdir = FVector::ZeroVector;
}
//플레이어 이동을 멈추도록 벡터 초기화
void APlayer_Character::MoveStop(const FInputActionValue& inputValue) {
	bMoveInputHolding = false;

	if (!bMaintainMoveOnNotInput)
	{
		AnimMoveSpeed = 0.f;
		LastPlayerdir = FVector::ZeroVector;

		//회전이 중단됬음을 서버에 알림
		if (!HasAuthority()) {
			Server_ClearMoveFacingYaw();
		}
		else {
			bHavingServerMoveFacingYaw = false;
		}

		return;
	}

	//입력 없이도 이동 유지 가능 상태일때
	else {
		if (LastPlayerdir.IsNearlyZero(0.05f)) {
			LastPlayerdir = GetActorForwardVector();
			LastPlayerdir.Z = 0.f;
			LastPlayerdir = LastPlayerdir.GetSafeNormal();
		}
	}
}

//서버에 캐릭터 회전값 전송
void APlayer_Character::TrySendToServerControlYaw() {
	if (!IsLocallyControlled()) return;
	
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	float CurrentYaw = PC->GetControlRotation().Yaw;
	float CurrentTime = GetWorld()->GetTimeSeconds();

	if (HasAuthority()) {
		ServerControlYaw = CurrentYaw;
		LastSenttoServerYaw = CurrentYaw;
		LastTurnTime = CurrentTime;
		return;
	}

	bool bMinTimePassed = (CurrentTime - LastTurnTime) >= YawSendtoServerInterval;
	float YawChange = FMath::Abs(FMath::FindDeltaAngleDegrees(LastSenttoServerYaw, CurrentYaw));
	bool bMinYawChanged = YawChange >= YawSendtoServerMinChange;

	if (bMinTimePassed && bMinYawChanged) {
		Server_SetControlYaw(CurrentYaw);
		LastSenttoServerYaw = CurrentYaw;
		LastTurnTime = CurrentTime;
	}
}
//현재 이동 속도를 기반으로 방향 설정 (서버)
void APlayer_Character::UpdateMoveFacingFromVelocity(float DeltaTime)
{
	if (!HasAuthority()) return;
	if (!bCanControl) return;
	if (bIsOut || bIsDodging || bIsAiming || bIsHitted) return;

	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.f;

	AnimMoveSpeed = HorizontalVelocity.Size();

	if (HorizontalVelocity.SizeSquared() < FMath::Square(5.f)) return;

	FVector MoveDir = HorizontalVelocity.GetSafeNormal();
	float Angle = FMath::RadiansToDegrees(FMath::Atan2(MoveDir.Y, MoveDir.X));

	ApplyPlayerRotation(Angle, DeltaTime);
}

//로컬 플레이어의 회전값 적용
void APlayer_Character::ApplyRotation(FVector2D& InputValue, float DeltaTime) {
	if (!IsLocallyControlled()) return;
	if (bIsAiming) return;
	if (InputValue.SizeSquared() < 0.05f) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	FRotator ControlYawRot(0.f, PC->GetControlRotation().Yaw, 0.f);
	FVector Forward = FRotationMatrix(ControlYawRot).GetUnitAxis(EAxis::X);
	FVector Right = FRotationMatrix(ControlYawRot).GetUnitAxis(EAxis::Y);
	Forward.Z = 0.f;
	Right.Z = 0.f;
	Forward = Forward.GetSafeNormal();
	Right = Right.GetSafeNormal();

	FVector MoveDir = (Forward * InputValue.X) + (Right * InputValue.Y);
	MoveDir.Z = 0.f;
	MoveDir = MoveDir.GetSafeNormal();

	if (MoveDir.IsNearlyZero()) return;

	float Angle = MoveDir.Rotation().Yaw;
	FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Angle, 0.f), DeltaTime, turn_Speed);
	SetActorRotation(NewRotation);
}


//플레이어 점프 처리 (MoveComp기반이라 서버에서 자동 처리)
void APlayer_Character::Player_Jump(const struct FInputActionValue& inputValue) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;
	if (TransformationComp && !TransformationComp->CanJumpDuringTransformation()) return;
	if (!CheckWeaponInteraction(EFunctionInterActionReason::Jump)) return;

	UWorld* World = GetWorld();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement) return;

	if (Movement->IsFalling()) return;	// [추가] 점프키 중복입력 방지

	float CurrentTime = World->GetTimeSeconds();
	//쿨타임이 남았다면 종료
	if (CurrentTime - LastJumpTime < JumpCoolTime) return;
	LastJumpTime = CurrentTime;
	Jump();

	// [사운드]
	if (JumpSound) {
		UGameplayStatics::PlaySoundAtLocation(this, JumpSound, GetActorLocation());
	}
	if (!HasAuthority()) {
		Server_PlayJumpSound();
	}
	else {
		Multicast_PlayJumpSound();
	}

	//점프 시 변경되는 Condition 상태 제어
	if (HasAuthority()) {
		NotifyConditionEvent(EPlayerConditionEvent::Jump, true);
	}
}

void APlayer_Character::Server_PlayJumpSound_Implementation(){
	Multicast_PlayJumpSound();
}

void APlayer_Character::Multicast_PlayJumpSound_Implementation(){
	if (IsLocallyControlled())return;
	if (JumpSound) {
		UGameplayStatics::PlaySoundAtLocation(this, JumpSound, GetActorLocation());
	}
}



bool APlayer_Character::BuildCurrentAttackPreviewData(FAimPreviewVisualData& OutData)
{
	OutData.Reset();
	if (!NowMap) return false;
	if (NowWeapon && NowWeapon->BuildAimPreviewData(this, OutData)) return true;
	float BlockSize = NowMap->BlockSize;

	FWeaponStats CurrentStats = GetWeaponStat();

	OutData.bVisible = true;
	OutData.PreviewRange = CurrentStats.AttackRange * BlockSize;
	OutData.PreviewRadius = CurrentStats.AttackRadius;
	OutData.HalfAngleDegree = CurrentStats.AttackDegree * 0.5f;
	OutData.PathRadius = CurrentStats.AttackRadius;
	OutData.AttackDirection = EWeaponAttackDirection::Horizontal;

	OutData.bOnlySameHeight = true;
	OutData.bBlockByWall = true;

	if (NowWeapon && !NowWeapon->CheckUseCounting()) {
		OutData.Reset();
		return false;
	}

	switch (CurrentStats.AttackType) {
	case EAttackType::Melee:
		OutData.bShowAttackSector = true;
		OutData.bOnlySameHeight = true;
		OutData.bBlockByWall = true;
		break;
	case EAttackType::Shoot:
	case EAttackType::Shoot_HS:
		OutData.bShowAttackSector = true;
		OutData.bShowAttackPath = true;
		OutData.bOnlySameHeight = true;
		OutData.bBlockByWall = true;
		break;
	case EAttackType::Throw:
		OutData.bShowAttackRangeCircle = true;
		OutData.bOnlySameHeight = false;
		OutData.bBlockByWall = false;
		break;
	default:
		return false;
	}

	return OutData.CheckUsingAnyVisual();
}

//플레이어 회피 서버 요청
void APlayer_Character::Dodge(const struct FInputActionValue& inputValue) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;
	if (TransformationComp && !TransformationComp->CanDodgeDuringTransformation()) return;
	if (!CheckWeaponInteraction(EFunctionInterActionReason::Dodge)) return;
	if (bIsDodgeLocked) return;

	FVector DodgeDir = LastPlayerdir;
	if (DodgeDir.SizeSquared() < 0.05f) {
		DodgeDir = GetActorForwardVector();
	}
	DodgeDir.Z = 0;
	DodgeDir = DodgeDir.GetSafeNormal();
	DodgeInternal(DodgeDir);
	if (!HasAuthority()) {
		Server_Dodge(DodgeDir);
		return;
	}
}
//플레이어 회피 처리
void APlayer_Character::DodgeInternal(FVector DodgeDir){
	if (!bCanControl) return;
	UWorld* World = GetWorld();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!World || !Move) return;
	float CurrentTime = World->GetTimeSeconds();
	//쿨타임이 남았다면 종료
	if (CurrentTime - LastDodgeTime < DodgeCoolTime) return;

	if (NowWeapon && !NowWeapon->InteractionWeaponFunction(EFunctionInterActionReason::Dodge)) return;
	
	LastDodgeTime = CurrentTime;
	bIsDodging = true;

	//공격 후 Aim애니메이션 복구 타이머가 있다면, 회피 중에는 끄기
	GetWorldTimerManager().ClearTimer(ResumeAimAnimationTimerHandle);
	if (HasAuthority())
	{
		Multicast_StopSlotAnimation(FName(TEXT("UpperBody")), 0.03f);
		Multicast_StopSlotAnimation(FName(TEXT("DefaultSlot")), 0.03f);
	}
	else {
		UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
		if (AnimInstance) {
			AnimInstance->StopSlotAnimation(0.03f, FName(TEXT("UpperBody")));
			AnimInstance->StopSlotAnimation(0.03f, FName(TEXT("DefaultSlot")));
		}
	}
	//장착된 무기/물체의 회피 애니메이션이 따로 있는경우 재생
	if (HasAuthority()) PlayEquipmentAnimation(EFunctionInterActionReason::Dodge);

	//회피 시 변경되는 Condition 상태 제어
	if (HasAuthority()) {
		NotifyConditionEvent(EPlayerConditionEvent::Dodge, true);
	}

	//방향 설정 (입력이 없으면 캐릭터의 앞, 입력이 있다면 그 방향으로 회피)
	DodgeDir.Z = 0.f;
	DodgeDir = DodgeDir.GetSafeNormal();
	SetActorRotation(DodgeDir.Rotation(), ETeleportType::TeleportPhysics);

	FVector SavedVel = Move->Velocity;
	//출력 결정 (현재 무게에 따라 감소)
	float Strength = DodgeStrength - (100 * Weight);
	//지상 기본 마찰 저장
	SavedGroundFriction = Move->GroundFriction;
	SavedBrakingFrictionFactor = Move->BrakingFrictionFactor;
	SavedBrakingDecel = Move->BrakingDecelerationWalking;
	//공중 기본 저항 저장
	SavedBrakingDecelFalling = Move->BrakingDecelerationFalling;
	SavedFallingLateralFriction = Move->FallingLateralFriction;
	//회피 마찰 설정
	Strength = AirDodgeDistance / FMath::Max(AirDodgeDuration, 0.01f);
	Move->BrakingDecelerationFalling = SavedBrakingDecelFalling * 0.5f;
	Move->FallingLateralFriction = SavedFallingLateralFriction * 0.5f;
	Move->GroundFriction = DodgeGroundFriction;
	Move->BrakingFrictionFactor = DodgeBrakingFrictionFactor;
	Move->BrakingDecelerationWalking = DodgeBrakingDecel;
	//회피 실행
	LaunchCharacter(DodgeDir * Strength,/*xy축 속도 덮어쓰기*/ true,/*z축 속도 덮어쓰기*/ false);
	//타이머로 쿨타임 처리
	World->GetTimerManager().ClearTimer(DodgeTimerHandle);
	World->GetTimerManager().SetTimer(
		DodgeTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, SavedVel]() {
			UCharacterMovementComponent* MoveComp = GetCharacterMovement();
			if (MoveComp) {
				//공중 마찰 복구
				MoveComp->BrakingDecelerationFalling = SavedBrakingDecelFalling;
				MoveComp->FallingLateralFriction = SavedFallingLateralFriction;
				FVector V = MoveComp->Velocity;
				V.X = SavedVel.X;
				V.Y = SavedVel.Y;
				MoveComp->Velocity = V;			
				//지상 마찰 복구
				MoveComp->GroundFriction = SavedGroundFriction;
				MoveComp->BrakingFrictionFactor = SavedBrakingFrictionFactor;
				MoveComp->BrakingDecelerationWalking = SavedBrakingDecel;
			}
			//회피 완료 처리
			bIsDodging = false;

			if (HasAuthority()) {
				if (bIsAiming && !bIsOut && !bIsHitted) {
					PlayEquipmentAnimation(EFunctionInterActionReason::Aim);
				}

				ForceNetUpdate();
			}
			}),
		DodgeDuration,
		false
	);
}

//상호작용 탐지 범위 내의 가장 가까운 Equipment 탐색
AEquipment* APlayer_Character::ClosestEquipment() {
	TArray<AActor*> Overlapping;
	PickupDetectRange->GetOverlappingActors(Overlapping, AEquipment::StaticClass());
	AEquipment* target = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	FVector PlayerLocation = GetActorLocation();

	for (AActor* Equipment : Overlapping) {
		AEquipment* E = Cast<AEquipment>(Equipment);
		if (!IsValid(E)) continue;
		UPrimitiveComponent* PickupCollider = E->GetPickupCollider();
		if (!PickupCollider) continue;

		const float DistSq = FVector::DistSquared(PlayerLocation, PickupCollider->GetComponentLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			target = E;
		}
	}
	return target;
}

//상호작용 탐지 범위 내의 가장 가까운 Object 탐색
AObjects* APlayer_Character::ClosestObjects() {
	TArray<AActor*> Overlapping;
	PickupDetectRange->GetOverlappingActors(Overlapping, AObjects::StaticClass());
	AObjects* target = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	FVector PlayerLocation = GetActorLocation();

	for (AActor* Object : Overlapping) {
		AObjects* E = Cast<AObjects>(Object);
		if (!IsValid(E)) continue;
		//장착/상호작용 가능한 물체만 탐색
		if (!E->ObjectsData || (E->ObjectsData->bCanEquip != true && E->ObjectsData->bCanInteraction != true)) continue;
		UPrimitiveComponent* InterActionCollider = E->GetObjectInterActionCollider();
		if (!InterActionCollider) continue;

		const float DistSq = FVector::DistSquared(PlayerLocation, InterActionCollider->GetComponentLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			target = E;
		}
	}
	return target;
}

//Equipment획득 / 물체 상호작용 서버 요청
void APlayer_Character::Interaction(const FInputActionValue& Value) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (TransformationComp && !TransformationComp->CanInteractionDuringTransformation()) return;
	if (!CheckWeaponInteraction(EFunctionInterActionReason::InterAction)) return;
	
	if (!HasAuthority()) {
		Server_Interaction();
		return;
	}
	InteractionInternal();
}

//Equipment획득 / 물체 상호작용 처리
void APlayer_Character::InteractionInternal() {
	//장착 쿨타임 적용
	if (GetWorld()->GetTimeSeconds() - LastEquipTime < EquipCoolTime) {
		return;
	}
	AEquipment* Closest = ClosestEquipment();
	AObjects* ClosestOb = ClosestObjects();
	//가장 가까운 Equipment와 Object의 거리 비교 (없을 경우 -1)
	float ClosestEquipmentDist = Closest ? FVector::Dist(Closest->GetActorLocation(), GetActorLocation()) : -1.f;
	float ClosestObjectDist = ClosestOb ? FVector::Dist(ClosestOb->GetActorLocation(), GetActorLocation()) : -1.f;
	//탐색된 Equipment와 Object 둘 다 없는 경우
	if (ClosestEquipmentDist == -1.f && ClosestObjectDist == -1.f) return;

	//물체/무기 상호작용 특수 애니메이션이 따로 있는경우 그 애니메이션을 재생
	if (HasAuthority()) {
		PlayEquipmentAnimation(EFunctionInterActionReason::InterAction);
	}

	//탐색된 Equipment가 있고 그 Equipment가 가장 가까울 경우
	if ((ClosestEquipmentDist <= ClosestObjectDist || ClosestObjectDist == -1.f) && ClosestEquipmentDist != -1.f) {
		bool bCanPick = Closest->PickedByPlayer(this);
		LastEquipTime = GetWorld()->GetTimeSeconds();
		return;
	}
	//탐색된 Object가 있고 그 Object가 가장 가까울 경우
	else if ((ClosestEquipmentDist > ClosestObjectDist || ClosestEquipmentDist == -1.f) && ClosestObjectDist != -1.f) {
		//탐색된 Object 중 장착이 불가능하지만, 상호작용은 가능한 경우 상호작용 상태로 변경하고 장착x
		if (ClosestOb->ObjectsData->bCanEquip == false) {
			if (ClosestOb->ObjectsData->bCanInteraction == true) {
				ClosestOb->ApplyInteractionState(this);
			}
			return;
		}
		bool bCanPick = ClosestOb->PickedByPlayer(this);
		LastEquipTime = GetWorld()->GetTimeSeconds();
		return;
	}
}

void APlayer_Character::ApplyResevedWeapon()
{
	if (!HasAuthority()) return;
	if (NowWeapon) return;

	APlayer_State* PS = GetThePlayerState();
	if (!PS) return;
	if (!PS->CheckReservedWeapon()) return;

	EquipWeaponAuto(PS->GetReservedWeapon());
	PS->ClearReservedWeapon();
}

void APlayer_Character::EquipWeaponAuto(TSubclassOf<AWeapon> weapon)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeapon* SpawnedWeapon = World->SpawnActor<AWeapon>(weapon, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

	if (!SpawnedWeapon) return;

	PickWeapon(SpawnedWeapon);
}

//무기 획득
bool APlayer_Character::PickWeapon(TObjectPtr<AWeapon> weapon) {
	if (!weapon) return false;
	if (!HasAuthority()) return false;
	if (bIsInteractionLocked) return false;
	else {
		float Strength = 30.f;
		//이미 장착된 무기가 있다면 새로운 무기로 교체
		if (NowWeapon) {
			if (LastPlayerdir.IsNearlyZero(0.05f)) {
				Strength = PutStrength;
			}
			else {
				Strength = MoveStrength;
			}
			DropWeapon(Strength, false);
		}
		else if (NowObjects) {
			if (LastPlayerdir.IsNearlyZero(0.05f)) {
				Strength = PutStrength;
			}
			else {
				Strength = MoveStrength;
			}
			DropObjects(Strength, false);
		}
		LastAttackTime = -1.f;
		NowWeapon = weapon;
		NowWeapon->Equip(this);
		OnWeaponChanged.Broadcast();
		//무기의 Stat 획득
		AStat = GetWeaponStat();
		//현재 플레이어 상태 갱신
		NowWeapon->GetWeaponInfo(this);
		//move_Speed = move_Speed - (50 * (Weight - 1));
		UpdateMoveSpeed();

		if (VisualManagerComp) {
			VisualManagerComp->Multi_RefreshVisuals();
		}
	}
	return true;
}

//아이템 획득
bool APlayer_Character::PickItem(TObjectPtr<AItem> Item) {
	if (!Item) return false;
	if (!HasAuthority()) return false;
	if (bIsInteractionLocked) return false;
	else {
		float Strength = 30.f;
		//이미 장착된 아이템 있다면 새로운 무기로 교체
		if (NowItem) {
			if (LastPlayerdir.IsNearlyZero(0.05f)) {
				Strength = PutStrength;
			}
			else {
				Strength = MoveStrength;
			}
			DropItem(Strength);
		}
		NowItem = Item;
		NowItem->Equip(this);

		//장착한 Item을 PlayerState에도 저장
		SaveNowItem();
	}
	return true;
}

//물체 획득
bool APlayer_Character::PickObjects(TObjectPtr<AObjects> Object) {
	if (!Object) return false;
	if (!HasAuthority()) return false;
	if (bIsInteractionLocked) return false;
	else {
		float Strength = 30.f;
		//이미 장착된 무기가 있다면 새로운 무기로 교체
		if (NowWeapon) {
			if (LastPlayerdir.IsNearlyZero(0.05f)) {
				Strength = PutStrength;
			}
			else {
				Strength = MoveStrength;
			}
			DropWeapon(Strength, false);
		}
		else if (NowObjects) {
			if (LastPlayerdir.IsNearlyZero(0.05f)) {
				Strength = PutStrength;
			}
			else {
				Strength = MoveStrength;
			}
			DropObjects(Strength, false);
		}
		LastAttackTime = -1.f;
		NowObjects = Object;
		NowObjects->Equip(this);
		//현재 플레이어 상태 갱신
		NowObjects->GetObjectInfo(this);
		//move_Speed = move_Speed - (50 * (Weight - 1));
		UpdateMoveSpeed();

		if (VisualManagerComp) {
			VisualManagerComp->Multi_RefreshVisuals();
		}
	}
	return true;
}

//아이템 사용 서버 요청
void APlayer_Character::UseItem(const FInputActionValue& inputValue)
{
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;
	if (TransformationComp && !TransformationComp->CanUseItemDuringTransformation()) return;
	if (!CheckWeaponInteraction(EFunctionInterActionReason::UseItem)) return;

	if (!HasAuthority()) {
		Server_UseItem();
		return;
	}

	UseItemInternal();
}

//아이템 사용 처리
void APlayer_Character::UseItemInternal() {
	if (!bCanControl) return;
	if (!NowItem) return;
	if (!NowMap) {
		UWorld* World = GetWorld();
		if (!World) return;
		
		APlayMode_Match* GM = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
		if (!GM) return;

		NowMap = GM->GetCurrentMap();
		if (!NowMap) return;
	}
	NowItem->UseItem();
	if (NowSupport) {
		if (VisualManagerComp) {
			VisualManagerComp->Multi_RefreshVisuals();
		}
	}
	if (!IsValid(NowItem) || NowItem->NowUseCount <= 0) {
		NowItem = nullptr;
	}
	//PlayerState에 아이템 정보 갱신
	SaveNowItem();
}

//Equipment 드롭 서버 요청
void APlayer_Character::Drop(const FInputActionValue& Value) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;
	if (bIsInteractionLocked) return;
	if (TransformationComp && !TransformationComp->CanInteractionDuringTransformation()) return;
	if (!CheckWeaponInteraction(EFunctionInterActionReason::Drop)) return;
	if (!HasAuthority()) {
		Server_Drop();
	}
	else {
		DropInternal();
	}
}

//Equipment/물체 드롭 처리
void APlayer_Character::DropInternal() {
	float Strength = 30.f;
	if (GetWorld()->GetTimeSeconds() - LastUnEquipTime < UnEquipCoolTime) {
		return;
	}
	FVector PlayerVelocity = GetVelocity();
	PlayerVelocity.Z = 0.f;

	if (PlayerVelocity.SizeSquared() < FMath::Square(0.5f)) {
		Strength = PutStrength;
	}
	else {
		Strength = MoveStrength;
	}
	if (NowWeapon) {
		DropWeapon(Strength, false);
	}
	else if (NowObjects) {
		DropObjects(Strength, false);
	}
	else if (NowItem) {
		DropItem(Strength);
	}

	//*버린 직후 감지가 안되는 현상 방지*
	PickupDetectRange->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupDetectRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//*빠르게 버리고 줍기를 반복하는 것을 방지*
	LastEquipTime = GetWorld()->GetTimeSeconds();
}

float APlayer_Character::GetThrowDamageWithWeight(float weight)
{
	float TotalDamage = 5 + weight * 20;
	return TotalDamage;
}

//Equipment/Object 드롭 위치 설정
FTransform APlayer_Character::DropTransform() {
	FVector Forward = GetActorForwardVector();
	FVector Location = GetActorLocation() + (Forward * (ItemSlot->GetRelativeLocation().X + 10.f)) + FVector(0.f, 0.f, 35.f);
	FRotator Rotation = FRotator(0.f, GetActorRotation().Yaw, 0.f);
	return FTransform(Rotation, Location, FVector(1.f));
}

//Equipment를 이동하며 드롭시 던짐 설정
void APlayer_Character::ApplyThrow(AEquipment* equipment, float BaseStrength, float UpStrength, float IgnorePawnSeconds) {
	if (!equipment) return;
	if (!HasAuthority()) return;
	UPrimitiveComponent* Equipment = equipment->GetPhysicsCollider();
	if (!Equipment) return;

	//던질 방향과 힘의 강도 설정
	FVector Dir = GetActorForwardVector().GetSafeNormal();
	FVector Impulse = Dir * BaseStrength + FVector(0.f, 0.f, UpStrength);
	FVector PlayerVelocity = GetVelocity();
	PlayerVelocity.Z = 0.f;
	//Equipment 물리 설정
	Equipment->SetSimulatePhysics(true);
	Equipment->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Equipment->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Equipment->WakeAllRigidBodies();
	Equipment->SetPhysicsLinearVelocity(PlayerVelocity, false);
	Equipment->AddImpulse(Impulse, NAME_None, true);
	//던진 직후 충돌 무시 설정(필요없으면 IgnorePawnSeconds를 0.0으로)
	if (IgnorePawnSeconds > 0.f) {
		FTimerHandle IgnoreTimerHandle;
		GetWorldTimerManager().SetTimer(IgnoreTimerHandle, [Equipment]() {
			if (Equipment) {
				Equipment->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
			}
		}, IgnorePawnSeconds, false);
	}
}

//물체를 이동하며 드롭시 던짐 설정
void APlayer_Character::ApplyThrowOb(AObjects* object, float BaseStrength, float UpStrength, float IgnorePawnSeconds) {
	if (!object) return;
	if (!HasAuthority()) return;
	UPrimitiveComponent* Object = object->GetObjectPhysicsCollider();
	if (!Object) return;

	//던질 방향과 힘의 강도 설정
	FVector Dir = GetActorForwardVector().GetSafeNormal();
	FVector Impulse = Dir * BaseStrength + FVector(0.f, 0.f, UpStrength);
	FVector PlayerVelocity = GetVelocity();
	PlayerVelocity.Z = 0.f;
	//Equipment 물리 설정
	Object->SetSimulatePhysics(true);
	Object->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Object->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Object->WakeAllRigidBodies();

	Object->SetPhysicsLinearVelocity(PlayerVelocity, false);
	Object->AddImpulse(Impulse, NAME_None, true);
	//던진 직후 충돌 무시 설정(필요없으면 IgnorePawnSeconds를 0.0으로)
	if (IgnorePawnSeconds > 0.f) {
		FTimerHandle IgnoreTimerHandle;
		GetWorldTimerManager().SetTimer(IgnoreTimerHandle, [Object]() {
			if (Object) {
				Object->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
			}
			}, IgnorePawnSeconds, false);
	}
}

//무기 드롭
void APlayer_Character::DropWeapon(float Strength, bool bIsThrowing) {
	if (!NowWeapon) return;
	if(!HasAuthority()) return;

	GetWorldTimerManager().ClearTimer(AttackEarlierDelayTimerHanlde);

	NowWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	NowWeapon->UnEquip(this);
	NowWeapon->SetActorTransform(DropTransform());
	
	if (VisualManagerComp) {
		VisualManagerComp->Multi_RestoreActorVisuals(NowWeapon);
		VisualManagerComp->Multi_RefreshVisuals();
	}

	if (bIsThrowing) {
		float ThrowDamage = GetThrowDamageWithWeight(NowWeapon->GetWeaponWeight());
		NowWeapon->BeginWeaponThrow(this, ThrowDamage);
	}

	ApplyThrow(NowWeapon, Strength, 0.f, 0.1f);

	//현재 플레이어 상태 갱신
	Weight = BaseStats.Default_Weight;
	move_Speed = BaseStats.Default_Speed;
	Aim_TurnSpeed = 0;
	LastAttackTime = -1.f;
	NowWeapon = nullptr;
	AStat = GetWeaponStat();

	UpdateMoveSpeed();
	OnWeaponChanged.Broadcast();
}

//아이템 드롭
void APlayer_Character::DropItem(float Strength) {
	if (!NowItem) return;
	if (!HasAuthority()) return;

	if (APlayer_State* PS = GetThePlayerState()) PS->ClearEquippedItem();

	NowItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	NowItem->UnEquip(this);
	NowItem->SetActorTransform(DropTransform());
	ApplyThrow(NowItem, Strength, 0.f, 0.1f);
	NowItem = nullptr;
}

//물체 드롭
void APlayer_Character::DropObjects(float Strength, bool bIsThrowing) {
	if (!NowObjects) return;
	if (!HasAuthority()) return;

	GetWorldTimerManager().ClearTimer(AttackEarlierDelayTimerHanlde);

	NowObjects->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	NowObjects->UnEquip(this);
	NowObjects->SetActorTransform(DropTransform());

	if (VisualManagerComp) {
		VisualManagerComp->Multi_RestoreActorVisuals(NowObjects);
		VisualManagerComp->Multi_RefreshVisuals();
	}

	if (bIsThrowing && NowObjects->ObjectsData) {
		float ThrowDamage = GetThrowDamageWithWeight(NowObjects->ObjectsData->Weight);
		NowObjects->BeginObjectThrow(this, ThrowDamage);
	}

	ApplyThrowOb(NowObjects, Strength, 0.f, 0.5f);
	//현재 플레이어 상태 갱신
	Weight = BaseStats.Default_Weight;
	move_Speed = BaseStats.Default_Speed;
	LastAttackTime = -1.f;
	NowObjects = nullptr;
	AStat = GetWeaponStat();

	UpdateMoveSpeed();
}

//플레이어의 장착 무기에 따른 스탯 반환
FWeaponStats APlayer_Character::GetWeaponStat() {
	//장착 무기가 있으면 무기 스탯 획득
	if (NowWeapon)
	{
		FWeaponStats* weaponStat = NowWeapon->GetWeaponStats();
		if (weaponStat) return *weaponStat;
	}
	//장착 무기가 없으면 평타 스탯 획득 (플레이어 기본 스탯)
	FWeaponStats normalStat;
	normalStat.AttackName = "Default";
	normalStat.Attack = BaseStats.Attack;
	normalStat.AttackRate = BaseStats.AttackRate;
	normalStat.AttackRange = BaseStats.AttackRange;
	normalStat.AttackType = EAttackType::Melee;
	normalStat.AttackTargetType = EAttackTargetType::SingleTarget;
	normalStat.AttackDegree = BaseStats.AttackDegree;
	normalStat.AttackRadius = BaseStats.AttackRadius;
	normalStat.AttackEarlierDelay = BaseStats.AttackEarlierDelay;
	normalStat.KnockBackStrength = BaseStats.KnockBackStrength;
	normalStat.KnockBackScale = BaseStats.KnockBackScale;
	return normalStat;
}
//마우스 포인터 위치 획득
bool APlayer_Character::GetMousePoint(FVector& MousePoint)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return false;
	FHitResult Hit;
	bool bHit = PlayerController->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(AimTraceChannel), true, Hit);
	if (!bHit) return false;
	MousePoint = Hit.ImpactPoint;
	return true;
}

void APlayer_Character::ApplyAimRotation(float DeltaTime) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;

	//마우스 포인터와 플레이어 캐릭터의 내적 벡터 계산
	FVector ToMouse = ServerAimPoint - GetActorLocation();
	ToMouse.Z = 0.f;
	if (ToMouse.IsNearlyZero()) return;
	float TargetYaw = ToMouse.Rotation().Yaw;

	ApplyPlayerRotation(TargetYaw, DeltaTime);
}

//조준 시작 서버 요청
void APlayer_Character::Aim(const struct FInputActionValue& inputValue) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;
	if (!CheckWeaponInteraction(EFunctionInterActionReason::Aim)) return;

	if (!HasAuthority()) {
		Server_Aim(true);
	}
	SetAimInternal(true);

	LastAimTime = -1.f;
	LastAimPoint = FVector::ZeroVector;

	bHavingCurrentAimTargetPoint = false;
	CurrentAimTargetPoint = FVector::ZeroVector;

	if (IsLocallyControlled()) {
		if (APlayerController* PC = Cast<APlayerController>(GetController())) {
			bShowMouseCursor = PC->bShowMouseCursor;
			SavedMouseCursor = PC->CurrentMouseCursor;

			PC->bShowMouseCursor = true;
			PC->CurrentMouseCursor = EMouseCursor::None;

			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);

			float AimStartForwardDistance = 100.f;
			FVector Forward = GetActorForwardVector();
			Forward.Z = 0.f;
			Forward = Forward.GetSafeNormal();

			FVector StartWorldPoint = GetActorLocation() + Forward * AimStartForwardDistance;
			FVector2D ScreenPosition;

			if (PC->ProjectWorldLocationToScreen(StartWorldPoint, ScreenPosition, false)) {
				PC->SetMouseLocation(FMath::RoundToInt(ScreenPosition.X), FMath::RoundToInt(ScreenPosition.Y));
			}
		}

		UpdateAimTargetPoint();
		TrySendtoServerAimPoint();
		UpdateAimPoint();
		UpdateAimPreview(0.f);
	}
}

//조준 해제 서버 요청
void APlayer_Character::AimStop(const struct FInputActionValue& inputValue) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (!HasAuthority()) {
		Server_Aim(false);
	}
	SetAimInternal(false);

	if (IsLocallyControlled()) {
		HideAimPoint();
		HideAimPreview();

		if (APlayerController* PC = Cast<APlayerController>(GetController())) {
			PC->bShowMouseCursor = bShowMouseCursor;
			PC->CurrentMouseCursor = SavedMouseCursor;

			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
		}
	}

	bHavingCurrentAimTargetPoint = false;
	CurrentAimTargetPoint = FVector::ZeroVector;

}

//조준 시작/해제 처리
void APlayer_Character::SetAimInternal(bool bAiming) {
	bool bWasAiming = bIsAiming;
	bIsAiming = bAiming;

	//무기가 조준 애니메이션이 따로 있는 경우 그 애니메이션을 재생 (중복 재생을 방지하기 위해 비조준->조준일 때만 재생)
	if (HasAuthority() && bIsAiming && !bWasAiming) {
		PlayEquipmentAnimation(EFunctionInterActionReason::Aim);
	}
	else if (!bIsAiming && bWasAiming) {
		GetWorldTimerManager().ClearTimer(ResumeAimAnimationTimerHandle);
		Multicast_StopSlotAnimation(FName(TEXT("UpperBody")), 0.15f);
		Multicast_StopSlotAnimation(FName(TEXT("DefaultSlot")), 0.15f);
	}
}
//조준 위치를 서버에 전송
void APlayer_Character::TrySendtoServerAimPoint()
{
	if (!IsLocallyControlled()) return;
	if (!bIsAiming) return;
	if (!bHavingCurrentAimTargetPoint) {
		UpdateAimTargetPoint();
		if (!bHavingCurrentAimTargetPoint) return;
	}

	FVector mousepoint = CurrentAimTargetPoint;
	float CurrentTime = GetWorld()->GetTimeSeconds();
	bool bMinTimePassed = (CurrentTime - LastAimTime) >= AimSendtoServerInterval;
	bool bMinDistanceMoved = FVector::DistSquared(mousepoint, LastAimPoint) >= FMath::Square(AimSendtoServerMinDistance);

	ServerAimPoint = mousepoint;

	if (HasAuthority()) {
		LastAimPoint = mousepoint;
		LastAimTime = CurrentTime;
		return;
	}

	if (bMinTimePassed && bMinDistanceMoved) {
		Server_SetAim(mousepoint);
		LastAimPoint = mousepoint;
		LastAimTime = CurrentTime;
	}
}

void APlayer_Character::UpdateAimTargetPoint()
{
	if (!IsLocallyControlled()) return;
	if (!bIsAiming) return;

	float BlockSize = NowMap ? NowMap->BlockSize : 100.f;
	float AttackRealRange = AStat.AttackRange * BlockSize;

	FVector FallbackPoint = GetActorLocation() + GetActorForwardVector() * AttackRealRange;
	APlayerController* PC = Cast<APlayerController>(GetController());

	if (!PC) {
		CurrentAimTargetPoint = FallbackPoint;
		bHavingCurrentAimTargetPoint = true;
		return;
	}

	FHitResult AimHit;
	bool bHavingAimHit = PC->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(AimTraceChannel), true, AimHit);

	//CurrentAimTargetPoint = bHavingAimHit ? AimHit.ImpactPoint : FallbackPoint;
	// 조준포인터 용암위 버그 수정
	if (bHavingAimHit) {
			// 블록체널이라면 그 위치 가져옴
		CurrentAimTargetPoint = AimHit.ImpactPoint;
	}
	else {
		// 킬존 용암의 콜리전에서 마우스포인트를 오버랩으로 바꿨음
		// 마우스포인터는 용암위로 올라가나
		// 캐릭터 회전과 공격범위가 따라가지 못해
		// else에서 따라가도록 설정
		FVector WorldLocation, WorldDirection;

		// 마우스 커서 화면 위치에서 3D로 광선 발사
		if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection)) {
			FVector PlaneNormal(0.f, 0.f, 1.f);	//케릭터 발바닥 높이
			FVector PlanePoint = GetActorLocation();

			// 마우스광선과 캐릭터 발바닥 높이의 교차점 찾기
			FVector Intersection = FMath::LinePlaneIntersection(
				WorldLocation,	// 선분 시작점
				WorldLocation + WorldDirection * 1000.f,	// 선분 끝점 (10m로 설정)
				PlanePoint,	//평면 위 한점
				PlaneNormal	//평면 바닥 방향
			);
			// 마우스 방향 위치로 초기화
			FallbackPoint = Intersection;
		}
		CurrentAimTargetPoint = FallbackPoint;
	}

	bHavingCurrentAimTargetPoint = true;
}

void APlayer_Character::UpdateAimPoint()
{
	if (!IsLocallyControlled()) return;
	if (!bIsAiming) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (!AimPoint) {
		if (!Player_AimPointWidget) return;

		AimPoint = CreateWidget<UUserWidget>(PC, Player_AimPointWidget);
		if (!AimPoint) return;
		
		AimPoint->AddToViewport(100);
		AimPoint->SetVisibility(ESlateVisibility::HitTestInvisible);
		AimPoint->SetDesiredSizeInViewport(AimPointWidgetSize);
		AimPoint->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	}

	float MouseX = 0.f;
	float MouseY = 0.f;

	bool bGotMouse = UWidgetLayoutLibrary::GetMousePositionScaledByDPI(PC, MouseX, MouseY);

	if (!bGotMouse) {
		AimPoint->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	AimPoint->SetPositionInViewport(FVector2D(MouseX, MouseY), false);
	AimPoint->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void APlayer_Character::HideAimPoint()
{
	if (AimPoint) {
		AimPoint->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APlayer_Character::UpdateAimPreview(float DeltaTime)
{
	if (!IsLocallyControlled()) return;
	if (!bIsAiming) return;

	if (!NowMap) {
		NowMap = Cast<AMapConstructor>(UGameplayStatics::GetActorOfClass(GetWorld(), AMapConstructor::StaticClass()));
		if (!NowMap) return;
	}

	FAimPreviewVisualData PreviewData;

	if (!BuildCurrentAttackPreviewData(PreviewData)) {
		HideAimPreview();
		return;
	}

	FVector Origin = GetActorLocation();

	if (GetCapsuleComponent()) {
		Origin.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}

	int32 GridX = 0;
	int32 GridY = 0;
	int32 GridZ = 0;

	//플레이어의 바닥 위치와 정확히 겹치면 경계 문제가 생길 수 있으므로 살짝 내림
	float ZOffset = FMath::Max(5.f, NowMap->BlockSize * 0.05f);
	FVector PlayerGridLocation = Origin - FVector(0.f, 0.f, ZOffset);

	if (NowMap->WorldToMapGrid(PlayerGridLocation, GridX, GridY, GridZ)) PreviewData.BaseGridZ = GridZ;
	else PreviewData.BaseGridZ = 0;

	if (!AimPreview) {
		if (!AttackPreviewGuide) return;

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AimPreview = GetWorld()->SpawnActor<AAttackPreviewGuide>(AttackPreviewGuide, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}
	if (!AttackPreviewGuide) return;

	FVector AimDir = CurrentAimTargetPoint - GetActorLocation();
	AimDir.Z = 0.f;

	if (AimDir.IsNearlyZero()) {
		AimDir = GetActorForwardVector();
		AimDir.Z = 0.f;
	}
	
	AimDir = AimDir.GetSafeNormal();
	AimPreview->UpdatePreview(NowMap, Origin, AimDir, PreviewData);
}

void APlayer_Character::HideAimPreview()
{
	if (AimPreview) {
		AimPreview->HidePreview();
	}
}

//플레이어 공격 서버 요청
void APlayer_Character::Attack(const struct FInputActionValue& inputValue) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;
	if (TransformationComp && !TransformationComp->CanAttackDuringTransformation()) return;

	if (IsLocallyControlled()) {
		UpdateAimTargetPoint();
	}

	FVector AimPointToUse = bHavingCurrentAimTargetPoint ? CurrentAimTargetPoint : ServerAimPoint;

	//서버에서 공격 처리
	if (!HasAuthority()) {
		if (NowWeapon) {
			if (!NowWeapon->BeforeAttackWeaponFunction()) {
				return;
			}
		}
		Server_Attack(false, AimPointToUse);
		return;
	}
	
	Server_Attack_Implementation(false, AimPointToUse);
}

//플레이어 공격 (Hold) 
void APlayer_Character::HoldAttack(const FInputActionValue& inputValue)
{
	// 조작불가상태 일때 리턴
	if (!bCanControl || bIsOut || bIsDodging) return;
	//공격 불가 변신 상태일때 리턴
	if (TransformationComp && !TransformationComp->CanAttackDuringTransformation()) return;
	//무지 장착되어있지 않으면 리턴
	if (!NowWeapon) return;

	//무기가 연속 공격이 불가능하면 리턴
	FWeaponStats* Stat = NowWeapon->GetWeaponStats();
	if (!Stat) return;

	//지속공격/연속공격 타입이 아닌 무기의 경우 리턴
	if (Stat->AttackInputType != EWeaponAttackInputType::Continuous && Stat->AttackInputType != EWeaponAttackInputType::Repeat) return;

	if (IsLocallyControlled()) UpdateAimTargetPoint();

	FVector AimPointToUse = bHavingCurrentAimTargetPoint ? CurrentAimTargetPoint : ServerAimPoint;

	//클라이언트라면 서버에 공격 요청 후 리턴하고 서버에서 처리하도록
	if (!HasAuthority()) {
		Server_Attack(true, AimPointToUse);
		return;
	}
	//서버라면 직접 공격 처리
	Server_Attack_Implementation(true, AimPointToUse);
}

void APlayer_Character::AttackRelease(const struct FInputActionValue& inputValue) {
	if (!NowWeapon) return;
	if (!NowWeapon->WeaponData) return;

	FWeaponStats* Stat = NowWeapon->GetWeaponStats();
	if (!Stat) return;
	if (Stat->AttackInputType != EWeaponAttackInputType::Continuous && Stat->AttackInputType != EWeaponAttackInputType::Repeat) return;
	
	if (!HasAuthority()) {
		Server_AttackRelease();
		return;
	}

	Server_AttackRelease_Implementation();
}

//공격 대상자와 공격자 사이에 벽이 있는지 확인
bool APlayer_Character::AttackLineOfSight(AActor* TargetActor)
{
	if (!TargetActor || !GetWorld()) return false;

	FVector Start = GetActorLocation();
	Start.Z += GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f : 50.f;
	FVector End = TargetActor->GetActorLocation();

	if (APlayer_Character* TargetCharacter = Cast<APlayer_Character>(TargetActor)) {
		if (TargetCharacter->GetCapsuleComponent()) {
			End.Z += TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f;
		}
	}
	else {
		End.Z += 50.f;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(TargetActor);

	if (NowWeapon)
	{
		Params.AddIgnoredActor(NowWeapon);
	}

	if (NowObjects)
	{
		Params.AddIgnoredActor(NowObjects);
	}

	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);

	return !bBlocked;
}

void APlayer_Character::UpdateKnockBackAirDamping(float DeltaTime)
{
	if (!HasAuthority()) return;
	if (!bUseKnockBackAirDamping) return;
	if (!bKnockBackAirDampingActive) return;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) {
		bKnockBackAirDampingActive = false;
		return;
	}

	KnockBackAirDampingElapsed += DeltaTime;

	if (KnockBackAirDampingElapsed >= KnockBackAirDampingDuration) {
		bKnockBackAirDampingActive = false;
		return;
	}

	if (bIsDodging) return;

	if (!MoveComp->IsFalling()) {
		bKnockBackAirDampingActive = false;
		return;
	}

	FVector Velocity = MoveComp->Velocity;
	FVector XYVelocity(Velocity.X, Velocity.Y, 0.f);

	if (XYVelocity.SizeSquared() <= FMath::Square(KnockBackAirMinSpeed)) {
		Velocity.X = 0.f;
		Velocity.Y = 0.f;
		MoveComp->Velocity = Velocity;

		bKnockBackAirDampingActive = false;
		return;
	}

	FVector NewXYVelocity = FMath::VInterpTo(XYVelocity, FVector::ZeroVector, DeltaTime, KnockBackAirDamping);
	Velocity.X = NewXYVelocity.X;
	Velocity.Y = NewXYVelocity.Y;

	MoveComp->Velocity = Velocity;
}

//플레이어 공격 (클라이언트 처리)
void APlayer_Character::AttackInternal(bool bPlayAnimation) {
	if (!NowMap) {
		UWorld* World = GetWorld();
		if (!World) return;
		APlayMode_Match* GM = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
		if (!GM) return;
		NowMap = GM->GetCurrentMap();
		if (!NowMap) return;
	}
	float AttackRealRange = AStat.AttackRange * NowMap->BlockSize;
	
	//물체를 장착 중이면 물체를 던짐
	if (NowObjects) {
		if (bPlayAnimation) {
			PlayEquipmentAnimation(EFunctionInterActionReason::Attack);
		}
		//공격 시 변경되는 Condition 상태 제어
		if (HasAuthority()) {
			NotifyConditionEvent(EPlayerConditionEvent::Attack, true);
		}
		DropObjects(ThrowStrength - (300 * Weight), true);
		return;
	}
	//사용 횟수가 소진된 무기를 장착 중이면 무기를 던짐
	if (NowWeapon && !(NowWeapon->CheckUseCounting())) {
		if (bPlayAnimation) {
			PlayEquipmentAnimation(EFunctionInterActionReason::Drop);
		}
		//공격 시 변경되는 Condition 상태 제어
		if (HasAuthority()) {
			NotifyConditionEvent(EPlayerConditionEvent::Attack, true);
		}
		DropWeapon(ThrowStrength - (300 * Weight), true);
		return;
	}

	//무기가 자체 공격 기능을 따로 설정한 경우 그 함수를 실행 (특수 공격 함수)
	if (NowWeapon && NowWeapon->WeaponData) {
		FWeaponStats* Stat = NowWeapon->GetWeaponStats();
		EWeaponAttackInputType InputType = Stat ? Stat->AttackInputType : EWeaponAttackInputType::Single;
		//지속 공격 타입 무기가 지속공격 상태가 아닌경우 즉시 리턴
		if (InputType == EWeaponAttackInputType::Continuous && !bNowHoldingAttack) {
			return;
		}
		//특수 무기공격 함수를 실행 여부 확인 (결과가 True인 경우만 일반공격 함수를 추가 실행) 
		if (!NowWeapon->InteractionWeaponFunction(EFunctionInterActionReason::Attack)) {
			//공격 시 변경되는 Condition 상태 제어
			if (HasAuthority()) {
				NotifyConditionEvent(EPlayerConditionEvent::Attack, true);
			}
			OnWeaponChanged.Broadcast();
			return;
		}
	} 

	//무기별 공격 애니메이션 재생
	if (bPlayAnimation) {
		PlayEquipmentAnimation(EFunctionInterActionReason::Attack);
	}
	//공격 시 변경되는 Condition 상태 제어
	if (HasAuthority()) {
		NotifyConditionEvent(EPlayerConditionEvent::Attack, true);
	}

	//공격 무기 범위 계산
	FVector ARangeStart;
	FVector ARangeEnd;
	FVector Forward = GetActorForwardVector();

	float ARadius = FMath::Max(1.f, AStat.AttackRadius);
	float AHalfAngle = AStat.AttackDegree * 0.5f;

	ARangeStart = GetActorLocation() + (Forward * ARadius);
	float AdjustedRange = FMath::Max(0.f, AttackRealRange - (ARadius * 2));
	ARangeEnd = ARangeStart + Forward * AdjustedRange;
	
	TArray<FHitResult> Hits;

	//디버그 용
	TArray<AActor*>ActorsToIgnore;
	ActorsToIgnore.Add(this);

	//Hit한 Actor들 중 Pawn과 Object만 획득
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel4));

	//각 공격에 사용되는 Trace의 개수를 얻는 람다 함수 
	auto CalculateTraceIntervalCount = [](float AHalfAngle, float Range, float TraceRadius) -> int32 {
		if (Range <= 0 || TraceRadius <= 0) return 1;
		//공격 각도에 따른 현의 길이 획득 공식 2R(반지름) * Sin(Radian(각도(라디안)))
		float AHalfAngleRadius = FMath::DegreesToRadians(AHalfAngle);
		float DegreeLineTotalLength = 2 * Range * FMath::Sin(AHalfAngleRadius);
		//전체 Trace 개수는 현의 길이 / 지름의 길이 (소수점 올림, 최소 1개 보장)
		return FMath::Max(1, FMath::CeilToInt(DegreeLineTotalLength / (TraceRadius * 2)));
	}; 
	/*람다 함수는 실제로 int32값으로 사용할 수 없으므로 auto로 선언 후 리턴 타입을 <- 로 지정*/
	/* 굳이 auto를 안쓰겠다면 TFunction<int32(매개변수들)> 함수 이름 <-- 이렇게도 쓸 수 있음*/

	//근거리 공격 범위 생성
	if (AStat.AttackType == EAttackType::Melee) {
		//공격 범위 계산 후 공격 범위 collsion 생성
		TSet<AActor*> HitActors;
		int32 NumTraceofAttackRange = CalculateTraceIntervalCount(AHalfAngle, AdjustedRange, ARadius);

		if (!NowWeapon || (NowWeapon && NowWeapon->WeaponData->AttackDirection != EWeaponAttackDirection::Round)) {
			for (int32 i = 0; i <= NumTraceofAttackRange; i++) {
				//1개의 Trace의 범위 지정 및 각도 계산
				float Trace = (float)i / (float)NumTraceofAttackRange;
				float TraceAngle = FMath::Lerp(-AHalfAngle, AHalfAngle, Trace);
				//기본 공격을 포함한 기본은 가로공격
				FVector TraceDir = Forward.RotateAngleAxis(TraceAngle, FVector::UpVector).GetSafeNormal();
				//세로 공격 무기이면 세로로 공격하도록 방향 변경
				if (NowWeapon && NowWeapon->WeaponData->AttackDirection == EWeaponAttackDirection::Vertical) {
					//플레이어 캐릭터 기준 세로방향 회전축 획득
					FVector RightAxis = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
					if (RightAxis.IsNearlyZero()) RightAxis = GetActorRightVector();
					TraceDir = Forward.RotateAngleAxis(TraceAngle, RightAxis).GetSafeNormal();
				}
				FVector TraceStart = ARangeStart;
				FVector TraceEnd = TraceStart + TraceDir * AdjustedRange;

				TArray<FHitResult> TraceHits;

				//debug 코드			
				UKismetSystemLibrary::SphereTraceMultiForObjects(
					this,
					TraceStart,
					TraceEnd,
					ARadius,
					ObjectTypes,
					false,
					ActorsToIgnore,
					EDrawDebugTrace::ForDuration,
					TraceHits,
					true
				);
				for (const FHitResult& Hit : TraceHits) {
					AActor* HitActor = Hit.GetActor();
					//Hit Actor 중복 방지
					if (!HitActor || HitActors.Contains(HitActor)) continue;
					HitActors.Add(HitActor);
					Hits.Add(Hit);
				}

			}
		}
		//무기가 전체 범위 공격을 사용할 경우 가로x세로 방향의 Attack Sphere Trace 생성
		else {
			int32 HorizontalTraceCount = CalculateTraceIntervalCount(AHalfAngle, AdjustedRange, ARadius);
			int32 VerticalTraceCount = CalculateTraceIntervalCount(AHalfAngle * 0.5f, AdjustedRange, ARadius);
			FVector ForwardDir = Forward.GetSafeNormal();

			for (int32 VerIndex = 0; VerIndex <= VerticalTraceCount; VerIndex++) {
				float VerAlpha = (float)VerIndex / (float)VerticalTraceCount;
				float VerAngle = FMath::Lerp(-(AHalfAngle / 2), AHalfAngle / 2, VerAlpha);

				for (int32 HorIndex = 0; HorIndex <= HorizontalTraceCount; HorIndex++) {
					float HorAlpha = (float)HorIndex / (float)HorizontalTraceCount;
					float HorAngle = FMath::Lerp(-AHalfAngle, AHalfAngle, HorAlpha);
					//각 방향으로 정규화
					float NormalizedHor = AHalfAngle > 0 ? HorAngle / AHalfAngle : 0.f;
					float NormalizedVer = AHalfAngle * 0.5f > 0 ? VerAngle / (AHalfAngle * 0.5f) : 0.f;
					//Hor방향, Ver방향으로 특정 지점과 중심과의 거리를 재었을 때 1이 넘으면 원(타원) 밖으로 간주하고 continue (* 공식 : X^2 + Y^2 <= 1) 
					if (NormalizedHor * NormalizedHor + NormalizedVer * NormalizedVer > 1.f) continue;

					//좌우 회전
					FVector HorDir = ForwardDir.RotateAngleAxis(HorAngle, FVector::UpVector).GetSafeNormal();
					//위아래 회전 축
					FVector RightAxis = FVector::CrossProduct(FVector::UpVector, HorDir).GetSafeNormal();

					if (RightAxis.IsNearlyZero()) {
						RightAxis = GetActorRightVector();
					}

					//위아래 회전
					FVector VerDir = HorDir.RotateAngleAxis(VerAngle, RightAxis).GetSafeNormal();

					FVector TraceStart = ARangeStart;
					FVector TraceEnd = TraceStart + VerDir * AdjustedRange;

					TArray<FHitResult> TraceHits;

					UKismetSystemLibrary::SphereTraceMultiForObjects(
						this,
						TraceStart,
						TraceEnd,
						ARadius,
						ObjectTypes,
						false,
						ActorsToIgnore,
						EDrawDebugTrace::ForDuration,
						TraceHits,
						true
					);

					for (const FHitResult& Hit : TraceHits) {
						AActor* HitActor = Hit.GetActor();
						if (!HitActor || HitActors.Contains(HitActor)) continue;

						HitActors.Add(HitActor);
						Hits.Add(Hit);
					}
				}
			}
		}
		
	}

	//원거리 히트스캔 공격 범위 생성
	else if (AStat.AttackType == EAttackType::Shoot_HS) {
		float RandomAngle = FMath::RandRange(-AStat.AttackDegree * 0.5f, AStat.AttackDegree * 0.5f);
		//기본은 가로방향으로 AttackDegree만큼 탄퍼짐 영역 생성
		FVector ShotDir = Forward.RotateAngleAxis(RandomAngle, FVector::UpVector).GetSafeNormal();
		//만약 무기가 세로방향 공격을 사용한다면 AttackDegree만큼 세로로 탄퍼짐 영역 생성 <있을지는 모르겠지만...>
		if (NowWeapon && NowWeapon->WeaponData->AttackDirection == EWeaponAttackDirection::Vertical){
			ShotDir = Forward.RotateAngleAxis(RandomAngle, FVector::RightVector).GetSafeNormal();
		}
		//만약 무기가 랜덤방향 공격을 사용한다면 가로범위 * 세로범위/2 만큼의 영역중 한곳을 랜덤으로 공격 범위로 지정
		else if (NowWeapon && NowWeapon->WeaponData->AttackDirection == EWeaponAttackDirection::Round) {
			float HorizontalHalfAngleRadian = FMath::DegreesToRadians(AStat.AttackDegree * 0.5f);
			float VerticalHalfAngleRadian = FMath::DegreesToRadians(AStat.AttackDegree * 0.25f);
			ShotDir = FMath::VRandCone(Forward.GetSafeNormal(), HorizontalHalfAngleRadian, VerticalHalfAngleRadian).GetSafeNormal();
		}
		FVector TraceStart = ARangeStart;
		FVector TraceEnd = TraceStart + ShotDir * AdjustedRange;

		TArray<FHitResult> TraceHits;
		TSet<AActor*> HitActors;

		//debug 코드
		UKismetSystemLibrary::SphereTraceMultiForObjects(
			this,
			TraceStart,
			TraceEnd,
			ARadius,
			ObjectTypes,
			false,
			ActorsToIgnore,
			EDrawDebugTrace::ForDuration,
			TraceHits,
			true
		);

		for (const FHitResult& Hit : TraceHits) {
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActors.Contains(HitActor)) continue;

			HitActors.Add(HitActor);
			Hits.Add(Hit);
		}
	}
	//원거리 투사체 물체 생성
	else if (AStat.AttackType == EAttackType::Shoot) {
		if (!NowWeapon) return;

		FVector AimDir;
		FVector TargetPoint;

		if (bIsAiming) {
			AimDir = ServerAimPoint - GetActorLocation();
		}
		else {
			AimDir = GetActorForwardVector();
		}
		AimDir.Z = 0.f;

		if (AimDir.IsNearlyZero()) {
			AimDir = GetActorForwardVector();
			AimDir.Z = 0.f;
		}

		AimDir = AimDir.GetSafeNormal();

		float RandomAngle = FMath::RandRange(-AStat.AttackDegree * 0.5f, AStat.AttackDegree * 0.5f);
		FVector ShotDir = AimDir.RotateAngleAxis(RandomAngle, FVector::UpVector).GetSafeNormal();
		TargetPoint = GetActorLocation() + ShotDir * AttackRealRange;

		NowWeapon->ShootorThrow(this, TargetPoint);
		return;
	}


	//투척 투사체 물체 생성
	else if (AStat.AttackType == EAttackType::Throw) {
		if (!NowWeapon) return;
		FVector TargetPoint;

		if (bIsAiming) {
			TargetPoint = ServerAimPoint;
			FVector ToTarget = TargetPoint - GetActorLocation();
			ToTarget.Z = 0.f;

			if (ToTarget.IsNearlyZero()) {
				FVector ForwardDir = GetActorForwardVector();
				ForwardDir.Z = 0.f;
				ForwardDir = ForwardDir.GetSafeNormal();

				TargetPoint = GetActorLocation() + GetActorForwardVector() * AttackRealRange;
			}
		}
		else {
			FVector ForwardDir = GetActorForwardVector();
			ForwardDir.Z = 0.f;

			if (ForwardDir.IsNearlyZero()) {
				ForwardDir = FVector::ForwardVector;
			}

			ForwardDir = ForwardDir.GetSafeNormal();
			TargetPoint = GetActorLocation() + ForwardDir * AttackRealRange;
		}

		NowWeapon->ShootorThrow(this, TargetPoint);
		//투척 후 남은 횟수가 없으면 조준점을 숨김
		if (!NowWeapon || !NowWeapon->CheckUseCounting()) {
			HideAimPoint();
			HideAimPreview();

			SetAimInternal(false);
			if (!HasAuthority()) Server_Aim(false);
		}
		return;
	}


	//공격 적중 대상 확인
	TArray<APlayer_Character*> TargetPlayers;
	TArray<AObjects*> TargetObjects;
	TargetPlayers.Reserve(Hits.Num());
	TargetObjects.Reserve(Hits.Num());

	for (FHitResult H : Hits) {
		AActor* Hitted = H.GetActor();
		//타격 대상이 플레이어인 경우
		if (APlayer_Character* HittedPlayers = Cast<APlayer_Character>(Hitted)) {
			//본인 타격 방지
			if (!HittedPlayers || HittedPlayers == this) continue;
			//시체 공격 방지
			if (HittedPlayers->HP <= 0 || HittedPlayers->bIsOut) continue;
			TargetPlayers.AddUnique(HittedPlayers);
		}
		if (AObjects* HittedObjects = Cast<AObjects>(Hitted)) {
			//장착 중인 Object 무시
			if (HittedObjects == NowObjects) continue;
			//체력을 사용하지 않는 Object 무시
			if (!HittedObjects->ObjectsData || HittedObjects->ObjectsData->bUseHP == false) continue;
			//본인이 장착 중인 물체/서포트라면 무시
			if (HittedObjects == NowObjects || HittedObjects == NowSupport) continue;
			//본인이 소유자인 물체는 무시
			if (HittedObjects->OwnPlayerController == this->GetController()) continue;

			TargetObjects.AddUnique(HittedObjects);
		}
	}

	//공격 비적중시 공격 효과 발생
	if (TargetPlayers.Num() == 0 && TargetObjects.Num() == 0) {
		LastAttackTime = GetWorld()->GetTimeSeconds();
		if (NowWeapon) {
			bool bHaveUseEffect = NowWeapon->ApplyUseEffect();
			//비적중시 무기 사용 횟수 차감하지 않는 무기인지 확인
			if (NowWeapon->WeaponData->bNotCountWhenUnHit) return;
			NowWeapon->UseWeapon();
		}
		return;
	}

	//공격 적중시 피해 적용 + 적중 효과 발생
	auto ApplyToPlayer = [&](APlayer_Character* HittedPlayer, float Damage) {
		if (!HittedPlayer || HittedPlayer->IsOut()) return;
		//벽너머 공격 시 return;
		if (!AttackLineOfSight(HittedPlayer)) return;
		float DamageMultiplier = 1.0f;	// 기본값 : 배율 1배
		bool bWeaponSkipRotation = false;	// 기본값 : 회전스킵==false
		bool bApplyKnockBack = true;
		bool bApplyRotation = true;

		//무기 데이터에서 넉백 적용 여부, 회전 적용 여부 확인
		if (NowWeapon && NowWeapon->WeaponData) {
			bApplyKnockBack = NowWeapon->WeaponData->bApplyKnockBack;
			bApplyRotation = NowWeapon->WeaponData->bApplyRotation;

			DamageMultiplier = NowWeapon->OnPreHit(HittedPlayer, bWeaponSkipRotation);
		}
		float FinalDamage = Damage * DamageMultiplier;

		//여기서 데미지 적용 함수 호출
		HittedPlayer->ApplyDamageInternal(FinalDamage, this, this, bApplyKnockBack, bApplyRotation && !bWeaponSkipRotation, false);
		if (NowWeapon) {
			NowWeapon->ApplyHitEffect(HittedPlayer);
		}
	};
	auto ApplyToObject = [&](AObjects* HittedObject, float Damage) {
		if (!HittedObject) return;
		//벽너머 공격 시 Return
		if (!AttackLineOfSight(HittedObject)) return;

		//무기 데이터에서 넉백 적용 여부 확인
		bool bApplyKnockBack = true;
		if (NowWeapon && NowWeapon->WeaponData)
		{
			bApplyKnockBack = NowWeapon->WeaponData->bApplyKnockBack;
		}

		//여기서 데미지 적용 함수 호출
		HittedObject->ApplyDamageInternal(Damage, this, this, bApplyKnockBack, false);
		if (NowWeapon) {
			NowWeapon->ApplyHitEffect(HittedObject);
		}
	};

	switch (AStat.AttackTargetType) {
	case EAttackTargetType::SingleTarget:
	{
		APlayer_Character* ClosestTargetPlayer = nullptr;
		AObjects* ClosestTargetObject = nullptr;
		float ClosestDistance = -1.f;
		float ClosestObDistance = -1.f;
		//가장 가까운 것이 플레이어 캐릭터인지 물체인지 탐색
		//적중 플레이어들과 적중 물체들의 가장 가까운 객체를 탐색
		for (AActor* Target : TargetPlayers) {
			if (!Target) continue;
			float Distance = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
			if (Distance < ClosestDistance || ClosestDistance == -1.f) {
				if (APlayer_Character* TargetPlayer = Cast<APlayer_Character>(Target)) {
					ClosestDistance = Distance;
					ClosestTargetPlayer = TargetPlayer;
				}
			}
		}
		for (AActor* Target : TargetObjects) {
			if (!Target) continue;
			float Distance = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
			if (Distance < ClosestDistance || ClosestDistance == -1.f) {
				if (AObjects* TargetObject = Cast<AObjects>(Target)) {
					ClosestObDistance = Distance;
					ClosestTargetObject = TargetObject;
				}
			}
		}
		//가장 가까운 대상을 선별 후 그 대상에게 공격 적용 (bIsPlayer로 캐릭터인지 물체인지 판단)
		if (!ClosestTargetObject && !ClosestTargetPlayer) return;
		if ((ClosestObDistance < ClosestDistance || ClosestDistance == -1) && ClosestObDistance != -1){
			ApplyToObject(ClosestTargetObject, AStat.Attack);
		}
		else if ((ClosestObDistance >= ClosestDistance || ClosestObDistance == -1) && ClosestDistance != -1) {
			ApplyToPlayer(ClosestTargetPlayer, AStat.Attack);
		}
		break;
	}
	case EAttackTargetType::MultiTarget:
		//적중한 모든 대상에게 공격 적용
		for (APlayer_Character* P : TargetPlayers) {
			ApplyToPlayer(P, AStat.Attack);
		}
		for (AObjects* O : TargetObjects) {
			ApplyToObject(O, AStat.Attack);
		}
		break;
	}
	//장착 무기의 사용 가능 횟수 차감
	if (NowWeapon) NowWeapon->UseWeapon();
}

//플레이어 데미지 적용 전 처리 (플레이어에 의해 데미지를 받으면 플레이어 포인터 획득, 아니면 플레이어 포인터 NULL) <override 상태>
float APlayer_Character::TakeDamage(float damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) {
	if (!HasAuthority()) return 0.0f;
	APlayer_Character* AttackPlayer = nullptr;
	if (EventInstigator) {
		AttackPlayer = Cast<APlayer_Character>(EventInstigator->GetPawn());
	}

	float DamageMultiplier = 1.0f;	// 기본값 : 배율 1배
	bool bWeaponSkipRotation = false;	// 기본값 : 회전스킵==false
	bool bApplyKnockBack = true;
	bool bApplyRotation = true;
	//무기 데이터에서 넉백 적용 여부, 회전 적용 여부 확인
	if (AttackPlayer && AttackPlayer->NowWeapon && AttackPlayer->NowWeapon->WeaponData) {
		bApplyKnockBack = AttackPlayer->NowWeapon->WeaponData->bApplyKnockBack;
		bApplyRotation = AttackPlayer->NowWeapon->WeaponData->bApplyRotation;

		DamageMultiplier = AttackPlayer->NowWeapon->OnPreHit(this, bWeaponSkipRotation);
	}
	float FinalDamage = damage * DamageMultiplier;

	return ApplyDamageInternal(FinalDamage, AttackPlayer, DamageCauser, bApplyKnockBack, bApplyRotation && !bWeaponSkipRotation, false);
}

//플레이어 데미지 적용 처리
float APlayer_Character::ApplyDamageInternal(float Damage, APlayer_Character* AttackPlayer, AActor* DamageCauser, bool bApplyKnockBack, bool bApplyRotation, bool bForceDamage)
{
	if (!HasAuthority()) return 0.f;
	if (bIsOut) return 0.f;
	if (bEndMatchState && !bForceDamage) return 0.f;
	if (bIsBigHitReaction && !bForceDamage) return 0.f;
	if (Damage < 0) return 0.f;

	/*플레이어가 무적 상태이고, bForceDamage가 false일 때 데미지 적용 x 구현부*/
	if (ConditionComp && !bForceDamage) {
		bool bHasDamageImmunity = ConditionComp->CheckCondition(FName("NoDamage"));
		bool bHasInvincibility = ConditionComp->CheckCondition(FName("Invincible"));

		if (bHasDamageImmunity || bHasInvincibility) {
			return 0.f;
		}
	}
	
	FVector AttackDir = FVector::ZeroVector;
	GetWorldTimerManager().ClearTimer(HoldLastAttackPlayer);
	float KnockBackStrength = 0.f;
	float KnockBackScale = 0.f;
	int32 CurrentCoin = GetThePlayerState()->GetPlayerCoin();
	float FinalDamage = FMath::Max(0.f, Damage);

	HP = FMath::Clamp(HP - FinalDamage, 0.0f, BaseStats.Max_HP);
	OnRep_HP();
	ForceNetUpdate();

	//변신 중이라면 피격 효과 발동 및 피격 상황 알림
	if (TransformationComp) {
		TransformationComp->NotifyHittedDuringTransformation(AttackPlayer);
	}
	
	bool bSkipHitReaction = bIsDodging;
	bool bSkipRotation = !bApplyRotation;
	bIsHitted = true;

	//체력이 0이 되면 탈락
	if (HP <= 0.0f) {
		if (CurrentCoin > 0) {
			float dropCoinAmount = FMath::Min(OutCoinLoseAmount, CurrentCoin);
			GetThePlayerState()->AddCoin(-dropCoinAmount);
			Server_SpawnLostCoin(dropCoinAmount);
			LastLoseCoinHP = HP;
		}
		Multicast_PlayHitReaction(FinalDamage, true, false, AttackDir);
		Out(AttackPlayer);
		return FinalDamage;
	}

	//플레이어와 물체에 대한 넉백 계산
	if (AttackPlayer && AttackPlayer != this) {
		AttackDir = GetActorLocation() - AttackPlayer->GetActorLocation();
		KnockBackStrength = AttackPlayer->AStat.KnockBackStrength;
		KnockBackScale = AttackPlayer->AStat.KnockBackScale;
		//직전 공격자 일시 저장
		LastAttackPlayer = AttackPlayer;
		//2초 후 직전 공격자를 nullptr로 설정
		GetWorldTimerManager().SetTimer(HoldLastAttackPlayer, this, &APlayer_Character::ClearLastAttackPlayer, 2.f, false);
	}
	if (AObjects* Object = Cast<AObjects>(DamageCauser)) {
		AttackDir = GetActorLocation() - Object->GetActorLocation();
		KnockBackStrength = Object->ObjectsData->KnockBackStrength;
		KnockBackScale = Object->ObjectsData->KnockBackScale;
	}
	
	AttackDir.Z = 0.f;
	AttackDir = AttackDir.GetSafeNormal();
	float TotalKnockBackStrength = bApplyKnockBack ? KnockBackStrength * KnockBackScale : 0.f;
	bool bBigHit = TotalKnockBackStrength >= BigHitKnockBackRule;

	//피격 시 변경되는 Condition 상태 제어
	if (FinalDamage > 0.f && ConditionComp) {
		if (TotalKnockBackStrength >= BigHitKnockBackRule) {
			ConditionComp->HandleConditionEvent(EPlayerConditionEvent::BigHit, true);
		}
		else {
			ConditionComp->HandleConditionEvent(EPlayerConditionEvent::Hit, true);
		}
	}

	if (!bSkipHitReaction)
	{
		if (bBigHit) {
			StartBigHitReaction();
			Multicast_PlayHitReaction(FinalDamage, false, !bSkipRotation, AttackDir, true);
		}
		else {
			bool bPlayedEquipHittedAnim = PlayEquipmentAnimation(EFunctionInterActionReason::Hitted);

			if (!bPlayedEquipHittedAnim) {
				Multicast_PlayHitReaction(FinalDamage, false, !bSkipRotation, AttackDir);
			}
		}
	}

	//플레이어 넉백 적용
	if(AttackDir != FVector::ZeroVector && bApplyKnockBack) ApplyKnockBack(AttackDir, TotalKnockBackStrength, 0.f);

	//코인 손실 체력 계산
	if (LastLoseCoinHP - HP >= CoinLoseHpInterval && CurrentCoin > 0 && HP > 0) {
		float dropCoinAmount = FMath::Min(CoinLoseAmount, CurrentCoin);
		GetThePlayerState()->AddCoin(-dropCoinAmount);
		Server_SpawnLostCoin(dropCoinAmount);
		LastLoseCoinHP = HP;
	}
	return FinalDamage;
}

//플레이어 넉백 처리
void APlayer_Character::ApplyKnockBack(FVector& AttackDir, float Strength, float UpStrength) {
	if (!HasAuthority()) return;

	FVector LaunchPow = AttackDir * Strength;
	LaunchPow.Z = UpStrength;
	LaunchCharacter(LaunchPow, true, false);

	if (bUseKnockBackAirDamping) {
		bKnockBackAirDampingActive = true;
		KnockBackAirDampingElapsed = 0.f;
	}
}

//플레이어 체력 회복 처리
void APlayer_Character::HPChange(float HPAmount) {
	if (HPAmount > 0.f) {
		HP = FMath::Clamp(HP + HPAmount, 0.f, BaseStats.Max_HP);
		LastLoseCoinHP = FMath::Clamp(LastLoseCoinHP + HPAmount, 0.f, BaseStats.Max_HP);
		OnRep_HP();
		ForceNetUpdate();
	}
}


//플레이어의 코인 드롭
void APlayer_Character::SpawnLostCoins(int32 Amount)
{
	if (!HasAuthority() || !CoinSlot || Amount <= 0) return;
	UWorld* World = GetWorld();
	if (!World) return;
	APlayMode_Match* GM = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
	if (!GM) return;
	AMapConstructor* Map = GM->GetCurrentMap();
	if (!Map) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<TSubclassOf<ACoin>> CoinList;
	SpawnCoinList(CoinList, Amount);
	if (CoinList.Num() <= 0) return;
	TArray<FVector> SafeBlockLocations;
	BuildCoinTargetLocations(CoinList.Num(), Map, SafeBlockLocations);

	FVector PlayerLocation = GetActorLocation();
	FVector CoinStartLocation = PlayerLocation + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.f);

	//선정된 도착 지점을 랜덤하게 셔플
	Algo::RandomShuffle(SafeBlockLocations);

	for (int32 i = 0; i < CoinList.Num(); i++) {
		TSubclassOf<ACoin> Coins = CoinList[i];
		if (!Coins) continue;

		FVector FinalStartLocation = CoinStartLocation + FVector(FMath::FRandRange(-10.f, 10.f), FMath::FRandRange(-10.f, 10.f), 0.f);
		FVector FinalTargetLocation = PlayerLocation + FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f), -50.f);

		//타겟지점이 총 코인 생성 수보다 많을 경우 타겟지점 매치
		if (SafeBlockLocations.IsValidIndex(i)) {
			FinalTargetLocation = SafeBlockLocations[i];
			FVector2D Offset2D = FMath::RandPointInCircle(2.5f);
			FinalTargetLocation += FVector(Offset2D.X, Offset2D.Y, 0.f);
		}
		//타겟지점이 총 코인 생성 수보다 적을경우 fallback
		else {
			FVector2D FallbackOffset = FMath::RandPointInCircle(120.f);
			FinalTargetLocation = PlayerLocation + FVector(FallbackOffset.X, FallbackOffset.Y, -50.f);
		}

		ACoin* Coin = World->SpawnActor<ACoin>(Coins, FinalStartLocation, FRotator::ZeroRotator, SpawnParams);
		if (!Coin) continue;

		if (UPrimitiveComponent* CoinComp = Coin->GetObjectPhysicsCollider()) {
			CoinComp->IgnoreActorWhenMoving(this, true);
		}

		Coin->SetLastOwner(this);
		Coin->LaunchToTargetLocation(FinalStartLocation, FinalTargetLocation);
		Coin->ForceNetUpdate();
	}
}
//코인이 이동할 목적지가 될 블록을 탐색 및 후보 선정
bool APlayer_Character::CollectNearbySafeBlocksFromMap(TArray<FVector>& SafeBlockLocations, int32 instanceSearchRadius, int32 instanceSearchHeight)
{
	SafeBlockLocations.Reset();

	UWorld* World = GetWorld();
	if (!World) return false;
	APlayMode_Match* GM = Cast<APlayMode_Match>(UGameplayStatics::GetGameMode(World));
	if (!GM) return false;
	AMapConstructor* Map = GM->GetCurrentMap();
	if (!Map) return false;

	int32 CenterX = 0;
	int32 CenterY = 0;
	int32 CenterZ = 0;
	bool bGridFound = Map->WorldToMapGrid(GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), CenterX, CenterY, CenterZ);
	//KillPlane에 빠졌을 때 맵 범위 밖일 가능성이 있으므로 Z값 보정하여 시도
	if (!bGridFound) {
		FVector FallbackLocation = GetActorLocation();
		FallbackLocation.Z += Map->BlockSize * 2.f;
		if (!bGridFound) { 
			return false;
		}
	}
	FVector StartLocation = GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.f);

	for (int32 ZOffset = instanceSearchHeight; ZOffset >= -instanceSearchHeight; -- ZOffset) {
		for (int32 XOffset = -instanceSearchRadius; XOffset <= instanceSearchRadius; ++XOffset) {
			for (int32 YOffset = -instanceSearchRadius; YOffset <= instanceSearchRadius; ++YOffset) {
				int32 CheckX = CenterX + XOffset;
				int32 CheckY = CenterY + YOffset;
				int32 CheckZ = CenterZ + ZOffset;
				FIntVector CheckLocation = FIntVector(CheckX, CheckY, CheckZ);
				//맵에 존재할 수 없는 좌표면 무시
				if (!Map->IsValidGrid(CheckX, CheckY, CheckZ)) {
					continue;
				}
				//일반 블럭이 아니거나 최상단 좌표가 아니면 무시
				if (!Map->IsNormalBlock(CheckX, CheckY, CheckZ) || !Map->IsTopBlock(CheckX, CheckY, CheckZ)) {
					continue;
				}
				//대상 블럭 위에 무언가가 있으면 무시
				if (!Map->IsEmptyOnFloorBlock(CheckLocation, {})) continue;
				//손실 플레이어의 위치는 무시
				if (CheckX == CenterX && CheckY == CenterY && CheckZ == CenterZ) continue;

				FVector BlockWorld = Map->GridToWorldCenter(CheckX, CheckY, CheckZ);

				//코인 착지 지점을 블록보다 아주 살짝 위로 올림
				BlockWorld.Z += 1.f;
				SafeBlockLocations.Add(BlockWorld);
			}
		}
	}
	if (SafeBlockLocations.Num() <= 0) {
		return false;
	}
	FVector PlayerLocation = GetActorLocation();
	SafeBlockLocations.Sort([PlayerLocation](const FVector& A, const FVector& B) {
		return FVector::DistSquared2D(PlayerLocation, A) < FVector::DistSquared2D(PlayerLocation, B);
	});

	return SafeBlockLocations.Num() > 0;
}

//코인 도착 대상 위치 목록을 생성
void APlayer_Character::BuildCoinTargetLocations(int32 RequiredTargetCount, AMapConstructor* CurrentMap, TArray<FVector>& TargetLocations) {
	TargetLocations.Reset();
	if (RequiredTargetCount <= 0) return;

	TSet<FIntVector> UsedTargetKeys;
	float NowRadius = SearchRadius;

	while (TargetLocations.Num() < RequiredTargetCount) {
		TArray<FVector> CandidateTargets;
		if (NowRadius > 10.f) break;
		//무한 루프 방지
		if (!CollectNearbySafeBlocksFromMap(CandidateTargets, NowRadius, SearchHeight)) {
			NowRadius += 2.f;
			continue;
		}
		for (FVector& Candidate : CandidateTargets) {
			FIntVector Key(FMath::RoundToInt(Candidate.X), FMath::RoundToInt(Candidate.Y), FMath::RoundToInt(Candidate.Z));
			if (UsedTargetKeys.Contains(Key)) continue;
			UsedTargetKeys.Add(Key);
			TargetLocations.Add(Candidate);
			if (TargetLocations.Num() >= RequiredTargetCount) { 
				return; 
			}
			NowRadius += 2.f;
		}
	}
	//오류로 인해 TargetLocation이 제대로 안잡혔을 경우 fallback
	if (TargetLocations.Num() <= 0) {
		while (TargetLocations.Num() < RequiredTargetCount) {
			FVector2D FallbackOffset = FMath::RandPointInCircle(120.f);
			TargetLocations.Add(GetActorLocation() + FVector(FallbackOffset.X, FallbackOffset.Y, -50.f));
		}
	}
}


//탐색된 후보 중 최종 목적지를 선정
FVector APlayer_Character::GetCoinTargetLocation(TArray<FVector>& SafeBlockLocations, TMap<int32, int32>& UsageCount) {
	if (SafeBlockLocations.Num() == 0) {
		return GetActorLocation() + FVector(0.f, 0.f, 40.f);
	}

	//Target으로 가장 사용이 적은 블록을 위주로 선정
	int32 MinUseCount = MAX_int32;
	for (int32 i = 0; i < SafeBlockLocations.Num(); i++) {
		int32 Count = UsageCount.Contains(i) ? UsageCount[i] : 0;
		MinUseCount = FMath::Min(MinUseCount, Count);
	}

	TArray<int32> Candidate;
	for (int32 i = 0; i < SafeBlockLocations.Num(); ++i) {
		int32 Count = UsageCount.Contains(i) ? UsageCount[i] : 0;
		if (Count <= MinUseCount + 1) {
			Candidate.Add(i);
		}
	}

	int32 PickedArrayIndex = Candidate[FMath::RandRange(0, Candidate.Num() - 1)];
	UsageCount.FindOrAdd(PickedArrayIndex)++;

	FVector Target = SafeBlockLocations[PickedArrayIndex];

	FVector2D Offset2D = FMath::RandPointInCircle(35.f);
	Target += FVector(Offset2D.X, Offset2D.Y, 20.f);

	return Target;
}

//생성할 코인의 List 설정
void APlayer_Character::SpawnCoinList(TArray<TSubclassOf<ACoin>>& CoinList, int32 Amount)
{
	//스폰해야 할 코인을 Big코인과 코인으로 분할 (BigCoin은 코인 10개 분량, MidCoin은 코인 5개 분량)
	//Amount 10 -> 1 value Coin 10
	//Amount 11 -> 1 value Coin 1, 10 value Coin 1
	//Amount 25 -> 1 value Coin 5, 10 value Coin 2
	//Amount 27 -> 1 value Coin 2, 5 value Coin 1, 10 value Coin 2
	CoinList.Reset();
	int32 CoinRemaining = Amount;

	while (CoinRemaining > 10) {
		if (BigCoinSlot) {
			CoinList.Add(BigCoinSlot);
		}
		else {
			for (int32 i = 0; i < 10; i++) {
				CoinList.Add(CoinSlot);
			}
		}
		CoinRemaining -= 10;
	}
	while (CoinRemaining > 5) {
		if (MidCoinSlot) {
			CoinList.Add(MidCoinSlot);
		}
		else {
			for (int32 i = 0; i < 5; i++) {
				CoinList.Add(CoinSlot);
			}
		}
		CoinRemaining -= 5;
	}
	for (int32 i = 0; i < CoinRemaining; i++) {
		CoinList.Add(CoinSlot);
	}
}

//플레이어 탈락
void APlayer_Character::Out(APlayer_Character* winnerplayer)
{
	if (bIsOut) return;
	if (!ConditionComp) return;
	if (!HasAuthority())return;
	bIsOut = true;

	//탈락하면 머리 위 위젯 제거
	SetPlayerWidgetVisibility(false);

	//다른 탈락(KillPlane 등)에서 SetInputBlock을 설정하지 않은 경우만 호출
	if (bCanControl) AddInputBlockController(FName("Out"), true, false, true, false);

	//변신 중이라면 즉시 변신 해제
	if (TransformationComp && TransformationComp->IsTransformed()) {
		TransformationComp->StopTransformation(true);
	}
	
	//탈락시킨 플레이어와 탈락한 플레이어의 Score 변경
	WinnerPlayer = winnerplayer;
	if (WinnerPlayer && !bEndMatchState) {
		APlayer_State* WinnerState = Cast<APlayer_State>(WinnerPlayer->GetPlayerState());
		if (WinnerState) {
			WinnerState->AddPlayerEliminate();
		}
	}
	//직접적으로 탈락시킨 플레이어가 없다면 LastAttackPlayer의 Score 변경 (LastAttackPlayer가 죽었다면 X)
	else if (!WinnerPlayer && LastAttackPlayer && !bEndMatchState) {
		if (LastAttackPlayer->bIsOut) {
			ClearLastAttackPlayer();
		}
		else {
			APlayer_State* LastAttackerState = Cast<APlayer_State>(LastAttackPlayer->GetPlayerState());
			if (LastAttackerState) {
				LastAttackerState->AddPlayerEliminate();
			}
		}
	}

	if(!bEndMatchState) GetThePlayerState()->AddPlayerOut();
	
	if (NowWeapon) DropWeapon(MoveStrength, false);
	if (NowObjects) DropObjects(MoveStrength, false);
	if (NowSupport) {
		NowSupport->Destroy();
		NowSupport = nullptr;
	}

	//PlayerState에 Item 정보 저장
	SaveNowItem();

	//플레이어 랭크 갱신
	if (APlayMode_Match* MatchMode = GetWorld()->GetAuthGameMode<APlayMode_Match>()) {
		MatchMode->UpdatePlayersRank();
	}

	Client_Out();

	GetWorldTimerManager().SetTimer(
		OutPlayerDestroyTimerHandle,
		this,
		&APlayer_Character::DestroyPlayer,
		OutAnimDuration,
		false
	);
}

//탈락한 플레이어 제거 후 서버에 리스폰 요청
void APlayer_Character::DestroyPlayer() {
	if (!HasAuthority() || bEndMatchState) return;
	AMatch_PlayerController* Match_PC = Cast<AMatch_PlayerController>(GetController());
	APlayMode_Match* Match = GetWorld()->GetAuthGameMode<APlayMode_Match>();
	if (!Match_PC || !Match) return;

	AActor* SpectatorTarget = Match->GetSpectatorTarget(this);
	APlayer_Character* Target = Cast<APlayer_Character>(SpectatorTarget);

	if (Target) {
		Match_PC->SetSpectatingDefaultCamera(false);
		Match_PC->SetCurrentSpectatingTarget(Target);
		Match_PC->Client_StartSpectatingPlayer(Target);
	}
	else {
		Match_PC->SetSpectatingDefaultCamera(true);
		Match_PC->SetCurrentSpectatingTarget(nullptr);
		Match_PC->Client_StartSpectatingDefaultCamera();
	}
	
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		AMatch_PlayerController* OtherPC = Cast<AMatch_PlayerController>(IT->Get());
		if (!OtherPC) continue;
		if (OtherPC == Match_PC) continue;

		if (!Match->GetRespawnMap().Contains(OtherPC)) continue;
		if (OtherPC->GetCurrentSpectatingTarget() != this) continue;

		if (Target) {
			OtherPC->SetSpectatingDefaultCamera(false);
			OtherPC->SetCurrentSpectatingTarget(Target);
			OtherPC->Client_StartSpectatingPlayer(Target);
		}
		else {
			OtherPC->SetSpectatingDefaultCamera(true);
			OtherPC->SetCurrentSpectatingTarget(nullptr);
			OtherPC->Client_StartSpectatingDefaultCamera();
		}
	}
	
	FTimerHandle DeadTimeHandle;
	GetWorldTimerManager().SetTimer(DeadTimeHandle, FTimerDelegate::CreateWeakLambda(this, [this, Match_PC, Match]() {
		if (Match_PC)
		{
			if (Match) {
				Match->RequestRespawn(Match_PC);
			}
		}
		// 죽은 플레이어 숨김 처리(콜리전 비활성화)
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		SetActorTickEnabled(false);
		SpeedControllers.Empty();
		BlockControllers.Empty();

		if (GetMesh()) {
			GetMesh()->SetRelativeLocation(DefaultMeshLocation);
		}
		if (springArmComp) {
			springArmComp->SetRelativeLocation(DefaultCamLocation);
			springArmComp->bEnableCameraLag = false;
		}
		bOutVisualSmoothing = false;
		VisualMeshLocation = FVector::ZeroVector;

		if (GetCapsuleComponent())
		{
			GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		if (GetMesh())
		{
			GetMesh()->SetVisibility(false, true);
		}
		//Respawn 후 제거 (이미 카메라는 Respawn한 곳으로 옮겼으므로 상관 x)
		SetLifeSpan(Match->GetRespawnTime() + 0.5f);
	}), 0.1f, false);
	
}

//공격 받은 방향으로 캐릭터 방향 변경
void APlayer_Character::TurnToAttackPlayer(const FVector& AttackDir) {
	if (AttackDir.IsNearlyZero()) return;

	FVector LookDir = AttackDir * FVector(-1.f, -1.f, -1.f);
	LookDir.Z = 0.f;
	LookDir = LookDir.GetSafeNormal();

	//강제로 플레이어 방향을 피격 방향으로 전환
	SetActorRotation(LookDir.Rotation(), ETeleportType::TeleportPhysics);
}

//탈락 시 장착 중인 아이템 저장
void APlayer_Character::SaveNowItem()
{
	if (!HasAuthority()) return;
	APlayer_State* PS = GetThePlayerState();
	if (!PS) return;

	if (NowItem) {
		PS->SetEquippedItem(NowItem->GetClass(), NowItem->NowUseCount);
	}
	else {
		PS->ClearEquippedItem();
	}
}

//리스폰 시 저장되어 있던 아이템 장착
void APlayer_Character::LoadNowItem()
{
	if (!HasAuthority()) return;
	if (NowItem) return;

	APlayer_State* PS = GetThePlayerState();
	if (!PS || !PS->CheckEquippedItem()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AItem* Item = World->SpawnActor<AItem>(PS->GetEquippedItem(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	if (!Item) return;

	Item->NowUseCount = PS->GetItemUseCount();
	PickItem(Item);
}


float APlayer_Character::WeightPenalty()
{
	return 50.f * FMath::Max(0.f, (float)Weight - 1.f);
}

float APlayer_Character::CalculateSpeed(float Default_Speed)
{
	float BaseSpeed = (Default_Speed < 0.f) ? BaseStats.Default_Speed : Default_Speed;
	float TotalMagnification = 1.f;
	float TotalOffset = 0.f;

	for (const FSpeedController& controller : SpeedControllers) {
		TotalMagnification *= controller.SpeedMagnification;
		TotalOffset += controller.SpeedOffset;
	}

	float NowSpeed = ((BaseSpeed - WeightPenalty()) * TotalMagnification + TotalOffset);
	return NowSpeed;
}

void APlayer_Character::UpdateMoveSpeed()
{
	move_Speed = CalculateSpeed();

	if (GetCharacterMovement()) {
		GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.f, move_Speed);
	}
}

void APlayer_Character::AddSpeedController(FName ControllerName, float Magnification, float offset, bool bConstantSpeed ,int32 Priority)
{
	if (ControllerName.IsNone()) return;

	bool bHasCurrentConstant = false;
	int32 CurrentConstantPriority = -1;

	//현재 SpeedController중 Constant가 있는지 확인
	for (const FSpeedController& controller : SpeedControllers) {
		if (controller.bConstant) {
			bHasCurrentConstant = true;
			CurrentConstantPriority = FMath::Max(CurrentConstantPriority, controller.SpeedPriority);
		}
	}
	//현재 SpeedController들 중 Constant가 있는 경우
	if (bHasCurrentConstant) {
		//현재 Constant보다 낮은 Priority가 들어오면 무시
		if (Priority < CurrentConstantPriority) return;
		//현재 Constant보다 같거나 높은 Priority SpeedController가 들어오면 기존 Constant Controller 제거
		for (auto IT = SpeedControllers.CreateIterator(); IT; ++IT) {
			if (IT->bConstant && IT->SpeedPriority <= Priority) {
				IT.RemoveCurrent();
			}
		}
		//같은 이름의 Controller는 제거 (갱신)
		RemoveSpeedControllerByName(ControllerName);

		FSpeedController NewController;
		NewController.SpeedControllerName = ControllerName;
		NewController.SpeedMagnification = Magnification;
		NewController.SpeedOffset = offset;
		NewController.bConstant = bConstantSpeed;
		NewController.SpeedPriority = Priority;

		SpeedControllers.Add(NewController);

		UpdateMoveSpeed();
		return;
	}
	//현재 Constant SpeedController가 없는 경우
	else {
		//같은 이름의 Controller는 제거 (갱신)
		RemoveSpeedControllerByName(ControllerName);

		bool bFinalConstantSpeed = false;
		//새로 들어오는 SpeedController가 Constant일때
		if (bConstantSpeed) {
			bool bHasHigherPriorityController = false;
			//새로 들어오는 SpeedController보다 높은 Priority가 있는지 확인
			for (const FSpeedController& controller : SpeedControllers) {
				if (controller.SpeedPriority > Priority) {
					bHasHigherPriorityController = true;
					break;
				}
			}
			//높은 Priority가 있다면 Constant해제
			if (bHasHigherPriorityController) bFinalConstantSpeed = false;
			//높은 Priority가 없다면 자신보다 같거나 낮은 Priority SpeedController 모두 제거, Constant 활성화
			else {
				RemoveSpeedControllerByPriority(Priority);
				bFinalConstantSpeed = true;
			}
		}
		//새로 들어오는 SpeedController가 Constant가 아니면 Priority와 상관없이 추가
		else {
			bFinalConstantSpeed = false;
		}
		
		FSpeedController NewController;
		NewController.SpeedControllerName = ControllerName;
		NewController.SpeedMagnification = Magnification;
		NewController.SpeedOffset = offset;
		NewController.bConstant = bFinalConstantSpeed;
		NewController.SpeedPriority = Priority;

		SpeedControllers.Add(NewController);

		UpdateMoveSpeed();
	}
	
}

void APlayer_Character::RemoveSpeedControllerByName(FName ControllerName)
{
	if (ControllerName.IsNone()) return;

	for (auto IT = SpeedControllers.CreateIterator(); IT; ++IT) {
		if (IT->SpeedControllerName == ControllerName) {
			IT.RemoveCurrent();
		}
	}
	
	UpdateMoveSpeed();
}

void APlayer_Character::RemoveSpeedControllerByPriority(int32 Priority)
{
	for (auto IT = SpeedControllers.CreateIterator(); IT; ++IT) {
		if (IT->SpeedPriority <= Priority) {
			IT.RemoveCurrent();
		}
	}

	UpdateMoveSpeed();
}



//현재 플레이어의 상태 데이터를 서버에서 획득
APlayer_State* APlayer_Character::GetThePlayerState() {
	return GetPlayerState<APlayer_State>();
}

//현재 플레이어의 상태 데이터를 반영
void APlayer_Character::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitPlayerWidget();
	BindPlayer_State();

	if (VisualManagerComp) {
		VisualManagerComp->RefreshPortraitMaterials();
	}
}

/*------------------애니메이션----------------------*/
//현재 장착중인 무기/물체의 공격 모션 몽타주 획득
UAnimMontage* APlayer_Character::GetCurrentAttackMontague(bool bUseAllBodyAnim)
{
	return nullptr;
}

//무기별 기본 그립 애니메이션 획득
UAnimSequenceBase* APlayer_Character::GetCurrentGripSequence()
{
	if (NowWeapon && NowWeapon->WeaponData) {
		return NowWeapon->WeaponData->WeaponGripSequence;
	}
	if (NowObjects && NowObjects->ObjectsData) {
		return NowObjects->ObjectsData->ObjectsGripSequence;
	}

	return nullptr;
}
//일반 공격 재생 애니메이션 선택 및 획득
UAnimSequence* APlayer_Character::GetNormalAttackSequence()
{
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	bool bSecondAttack = NormalAttackAnimIndex == 1 && (CurrentTime - LastNormalAttackTime) <= ChangeNormalAttackAnimationTime && SecondNormalAttack != nullptr;

	NormalAttackAnimIndex = bSecondAttack ? 2 : 1;
	LastNormalAttackTime = CurrentTime;

	return NormalAttackAnimIndex == 2 ? SecondNormalAttack : FirstNormalAttack;
}

void APlayer_Character::StartBigHitReaction()
{
	if (!HasAuthority()) return;
	if (bIsOut || bEndMatchState) return;
	if (!BigHittedMontage) return;

	//즉시 조준 해제
	bIsAiming = false;

	bIsBigHitReaction = true;
	bIsRecoverReaction = false;
	bIsHitted = true;

	BigHitStartTime = GetWorld()->GetTimeSeconds();
	BigHitStopTime = 0.f;

	bIsAiming = false;
	bIsDodging = false;
	bNowHoldingAttack = false;

	AddInputBlockController(FName("BigHit"), true, true, false, false);

	GetWorldTimerManager().ClearTimer(HittedResetTimerHandle);
	GetWorldTimerManager().ClearTimer(BigHitRecoverTimerHandle);

	Multicast_PlayOverrideMontage(BigHittedMontage, FName("Start"), true, false);

	float StartDuration = 0.1f;

	int32 StartSectionIndex = BigHittedMontage->GetSectionIndex(FName("Start"));
	if (StartSectionIndex != INDEX_NONE) {
		StartDuration = BigHittedMontage->GetSectionLength(StartSectionIndex);
	}
	StartDuration = FMath::Max(0.01f, StartDuration - 0.025f);

	GetWorldTimerManager().SetTimer(BigHitRecoverTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
		if (!HasAuthority()) return;
		if (!bIsBigHitReaction) return;
		if (bIsRecoverReaction) return;
		if (bIsOut || bEndMatchState) return;

		UCharacterMovementComponent* MoveComp = GetCharacterMovement();
		if (!MoveComp) {
			StartRecoverReaction();
			return;
		}

		FVector HorizontalVelocity = MoveComp->Velocity;
		HorizontalVelocity.Z = 0.f;
		
		float HorizontalSpeed = HorizontalVelocity.Size();
		if (HorizontalSpeed <= RecoverVelocityRule) {
			StartRecoverReaction();
			return;
		}
		
		//플레이어가 아직 날아가는 중이면 DuringSection 재생, 재생 후에도 여전히 날아가는 도중이면 해당 포즈에서 정지
		Multicast_PlayOverrideMontage(BigHittedMontage, FName("During"), false, bHoldBigHittingPose);

		BigHitStopTime = 0.f;
	}), StartDuration, false);

	ForceNetUpdate();
}

void APlayer_Character::UpdateBigHitReaction(float DeltaTime)
{
	if (!HasAuthority()) return;
	if (!bIsBigHitReaction) return;
	if (bIsRecoverReaction) return;
	if (bIsOut || bEndMatchState) return;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime - BigHitStartTime < BigHitRecoverMinCheckDelay) {
		BigHitStopTime = 0.f;
		return;
	}

	FVector HorizontalVelocity = MoveComp->Velocity;
	HorizontalVelocity.Z = 0.f;

	float HorizontalSpeed = HorizontalVelocity.Size();

	if (HorizontalSpeed <= RecoverVelocityRule) {
		BigHitStopTime += DeltaTime;

		if (BigHitStopTime >= BigHitRecoverStopHoldTime) {
			StartRecoverReaction();
		}
	}
	else {
		BigHitStopTime = 0.f;
	}
}

void APlayer_Character::StartRecoverReaction()
{
	if (!HasAuthority()) return;
	if (!bIsBigHitReaction) return;
	if (bIsRecoverReaction) return;
	if (bIsOut || bEndMatchState) return;

	bIsRecoverReaction = true;
	BigHitStopTime = 0.f;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement()) {
		MoveComp->StopMovementImmediately();
	}

	Multicast_PlayOverrideMontage(BigHittedMontage, FName("End"), false, false);
	float EndDuration = 0.5f;

	int32 EndSectionIndex = BigHittedMontage->GetSectionIndex(FName("End"));

	if (EndSectionIndex != INDEX_NONE) {
		EndDuration = BigHittedMontage->GetSectionLength(EndSectionIndex);
	}

	EndDuration = FMath::Max(0.01f, EndDuration - 0.025f);

	GetWorldTimerManager().ClearTimer(BigHitRecoverTimerHandle);
	GetWorldTimerManager().SetTimer(BigHitRecoverTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
		if (!HasAuthority()) return;
		if (!bIsBigHitReaction) return;
		if (!bIsRecoverReaction) return;
		if (bIsOut || bEndMatchState) return;
		
		Multicast_PlayRecoverReaction();
		float RecoverDuration = 0.3f;
		if (RecoverMontage) {
			RecoverDuration = RecoverMontage->GetPlayLength();
		}

		GetWorldTimerManager().ClearTimer(BigHitRecoverTimerHandle);
		GetWorldTimerManager().SetTimer(BigHitRecoverTimerHandle, this, &APlayer_Character::EndBigHitReaction, RecoverDuration, false);

	}), EndDuration, false);

	ForceNetUpdate();
}

void APlayer_Character::EndBigHitReaction()
{
	if (!HasAuthority()) return;

	bIsBigHitReaction = false;
	bIsRecoverReaction = false;
	bIsHitted = false;

	BigHitStopTime = 0.f;

	GetWorldTimerManager().ClearTimer(BigHitRecoverTimerHandle);

	if (!bIsOut && !bEndMatchState) {
		RemoveInputBlockController(FName("BigHit"));
	}

	if (ConditionComp) {
		ConditionComp->ResumeCurrentConditionAnimation();
	}

	ForceNetUpdate();
}

void APlayer_Character::BindPlayer_State()
{
	APlayer_State* PS = GetThePlayerState();
	if (!IsValid(PS)) return;

	//이미 같은 State가 있다면 중복 바인딩하지 않고 머터리얼 PortraitId 값만 적용
	if (NowPlayer_State == PS) {
		if (VisualManagerComp) {
			VisualManagerComp->RefreshPortraitMaterials();
		}
		return;
	}

	//기존 NowPlayer_State가 있다면 기존 바인딩 제거
	if (NowPlayer_State) {
		NowPlayer_State->OnPortraitIdChanged.RemoveAll(this);
	}

	NowPlayer_State = PS;
	NowPlayer_State->OnPortraitIdChanged.AddUObject(this, &APlayer_Character::HandlePortraitIdChanged);

	HandlePortraitIdChanged(NowPlayer_State->GetPortraitId());
}

void APlayer_Character::NotifyConditionEvent(EPlayerConditionEvent Event, bool bUseEndEffect)
{
	if (!HasAuthority()) return;

	if (ConditionComp)
	{
		ConditionComp->HandleConditionEvent(Event, bUseEndEffect);
	}
}

//입력이 없어도 이동 유지 상태로 설정/해제
void APlayer_Character::SetMaintainMoveOnNotInput(bool bEnable, float InNoInputMoveScale)
{
	bMaintainMoveOnNotInput = bEnable;
	NotInputMoveScale = FMath::Clamp(InNoInputMoveScale, 0.f, 1.f);

	if (!bMaintainMoveOnNotInput && !bMoveInputHolding) {
		LastPlayerdir = FVector::ZeroVector;

		if (!HasAuthority()) {
			Server_ClearMoveFacingYaw();
		}
		else {
			bHavingServerMoveFacingYaw = false;
		}
	}

	if (HasAuthority()) ForceNetUpdate();
}

//입력 없이 이동 유지 상태일 경우 자동 이동
void APlayer_Character::UpdateMaintainMoveOnNotInput(float DeltaTime)
{
	if (!bMaintainMoveOnNotInput) return;
	if (bMoveInputHolding) return;
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;
	if (move_Speed <= 0.f) return;
	if (TransformationComp && !TransformationComp->CanMoveDuringTransfomation()) return;
	
	FVector MoveDir = LastPlayerdir;
	MoveDir.Z = 0.f;

	if (MoveDir.IsNearlyZero(0.05f)) {
		MoveDir = GetActorForwardVector();
		MoveDir.Z = 0.f;
	}

	MoveDir = MoveDir.GetSafeNormal();
	if (MoveDir.IsNearlyZero()) return;

	AddMovementInput(MoveDir, NotInputMoveScale);

	if (!bIsAiming)
	{
		const float TargetYaw = MoveDir.Rotation().Yaw;
		ApplyPlayerRotation(TargetYaw, DeltaTime);

		if (!HasAuthority())
		{
			Server_SetMoveFacingYaw(TargetYaw);
		}
		else
		{
			ServerMoveFacingYaw = TargetYaw;
			bHavingServerMoveFacingYaw = true;
		}
	}
}

//캐릭터 피격 몽타주 재생
void APlayer_Character::PlayDamageAnimation(float Damage, bool bBigHit) {
	if (bBigHit) return;
	
	UAnimMontage* TargetMontage = nullptr;
	TargetMontage = HittedMontage;
	
	float ReleaseDelay = 0.15f;

	if (TargetMontage) {
		float AnimDuration = PlayAnimMontage(TargetMontage);
		if (AnimDuration > 0.f) {
			ReleaseDelay = AnimDuration;
		}
	}

	GetWorldTimerManager().ClearTimer(HittedResetTimerHandle);
	GetWorldTimerManager().SetTimer(HittedResetTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
		bIsHitted = false;
		if (HasAuthority()) {
			if (ConditionComp) {
				ConditionComp->ResumeCurrentConditionAnimation();
			}

			ForceNetUpdate();
		}
	}), ReleaseDelay, false);
}

void APlayer_Character::PlayAnimationDynamic(UAnimSequence* Sequence, FName SlotName, float BlendInTime, float BlendOutTime, float PlayRate, int32 LoopCount, int32 StartFrame)
{
	if (!Sequence) return;

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	float StartTime = 0.f;

	if (StartFrame > 0) {
		float AnimLength = Sequence->GetPlayLength();
		int32 NumKeys = Sequence->GetNumberOfSampledKeys();

		if (AnimLength > 0.01 && NumKeys > 1) {
			float FPS = (NumKeys - 1) / AnimLength;
			StartTime = StartFrame / FPS;
			StartTime = FMath::Clamp(StartTime, 0.f, FMath::Max(0.f, AnimLength));
		}
	}

	AnimInstance->PlaySlotAnimationAsDynamicMontage(Sequence, SlotName, BlendInTime, BlendOutTime, PlayRate, LoopCount, -1.f, StartTime);
}

bool APlayer_Character::NeedToPlayAllBodyAnimation()
{
	FVector Velocity2D = GetVelocity();
	Velocity2D.Z = 0.f;

	return Velocity2D.SizeSquared() < FMath::Square(10.f);
}

bool APlayer_Character::PlayEquipmentAnimation(EFunctionInterActionReason Reason)
{
	if (Reason == EFunctionInterActionReason::Move) {
		return false;
	}

	FEquipmentActionAnimation TargetAnimation;
	bool bFoundAnimation = false;
	int32 LoopCount = 1;
	int32 StartFrame = 0;

	//조준 애니메이션은 조준 해제까지 무한 루프
	if (Reason == EFunctionInterActionReason::Aim) {
		LoopCount = 999999;
	}

	if (NowWeapon && NowWeapon->WeaponData) {
		FEquipmentActionAnimation* FoundAnimation = NowWeapon->WeaponData->AdditionalAnimation.Find(Reason);

		if (FoundAnimation && FoundAnimation->IsValid()) {
			TargetAnimation = *FoundAnimation;
			bFoundAnimation = true;
		}
	}

	else if (NowObjects && NowObjects->ObjectsData) {
		FEquipmentActionAnimation* FoundAnimation = NowObjects->ObjectsData->AdditionalAnimation.Find(Reason);

		if (FoundAnimation && FoundAnimation->IsValid()) {
			TargetAnimation = *FoundAnimation;
			bFoundAnimation = true;
		}
	}

	if (!bFoundAnimation) return false;

	StartFrame = TargetAnimation.StartFrame;

	//장착된 무기/물체가 없는 경우 일반공격 모션 재생
	if (!NowWeapon && !NowObjects && Reason == EFunctionInterActionReason::Attack) {
		UAnimSequence* NormalAttackSequence = GetNormalAttackSequence();

		if (!NormalAttackSequence) return false;

		FName SlotName = GetActionSlotName(false);
		Multicast_PlayAnimationDynamic(NormalAttackSequence, SlotName, 0.025f, 0.1f, 1.f, 1, 0);

		return true;
	}

	//조준 중 공격 하여 공격 모션 재생 시 시작 프레임을 일부 건너뜀
	if (bIsAiming && Reason == EFunctionInterActionReason::Attack) {
		bool bHasAimAnimation = false;

		if (NowWeapon && NowWeapon->WeaponData) {
			FEquipmentActionAnimation* AimAnim = NowWeapon->WeaponData->AdditionalAnimation.Find(EFunctionInterActionReason::Aim);
			if(AimAnim) bHasAimAnimation = AimAnim->IsValid();
			
		}
		else if (NowObjects && NowObjects->ObjectsData) {
			FEquipmentActionAnimation* AimAnim = NowObjects->ObjectsData->AdditionalAnimation.Find(EFunctionInterActionReason::Aim);
			if(AimAnim) bHasAimAnimation = AimAnim->IsValid();
		}

		if (bHasAimAnimation && TargetAnimation.InterceptStartFrame >= 0) {
			StartFrame = TargetAnimation.InterceptStartFrame;
		}
	}

	//Override할 몽타주가 있는 경우 몽타주 재생
	if (TargetAnimation.MontageOverride) {
		Multicast_PlayOverrideMontage(TargetAnimation.MontageOverride);
		return true;
	}
	//Override할 몽타주가 없는 경우(전신 애니메이션의 경우) Sequence를 SlotName에 맞게 재생
	if (TargetAnimation.Sequence) {
		FName SlotName;

		//조준은 상체만 사용하므로 UpperBody로 고정
		if (Reason == EFunctionInterActionReason::Aim) {
			SlotName = FName(TEXT("UpperBody"));
		}
		//회피는 전신을 사용하므로 DefaultSlot으로 고정
		else if (Reason == EFunctionInterActionReason::Dodge) {
			SlotName = FName(TEXT("DefaultSlot"));
		}
		else {
			SlotName = GetActionSlotName(TargetAnimation.bForceFullBody);
		}

		Multicast_PlayAnimationDynamic(TargetAnimation.Sequence, SlotName, TargetAnimation.BlendInTime, TargetAnimation.BlendOutTime, TargetAnimation.PlayRate, LoopCount, StartFrame);

		//조준 중 공격 애니메이션이 끝나면 조준 애니메이션 복구
		if (Reason == EFunctionInterActionReason::Attack && bIsAiming) {
			float AnimLength = TargetAnimation.Sequence->GetPlayLength();
			float StartTime = 0.f;

			if (StartFrame > 0) {
				int32 NumKeys = TargetAnimation.Sequence->GetNumberOfSampledKeys();
				if (AnimLength > 0.01 && NumKeys > 1) {
					float FPS = (NumKeys - 1) / AnimLength;
					StartTime = StartFrame / FPS;
					StartTime = FMath::Clamp(StartTime, 0.f, FMath::Max(0.f, AnimLength));
				}
			}
			float RemainTime = (AnimLength - StartTime) / FMath::Max(TargetAnimation.PlayRate, 0.01f);
			RemainTime = FMath::Max(0.01f, RemainTime - TargetAnimation.BlendOutTime);

			GetWorldTimerManager().ClearTimer(ResumeAimAnimationTimerHandle);
			GetWorldTimerManager().SetTimer(ResumeAimAnimationTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
				if (!bIsAiming) return;
				if (bIsOut || bIsDodging || bIsHitted) return;

				PlayEquipmentAnimation(EFunctionInterActionReason::Aim);
			}), RemainTime, false);
		}
		return true;
	}

	return false;
}

FName APlayer_Character::GetActionSlotName(bool bUseFullBodyAnim)
{
	if (bUseFullBodyAnim || NeedToPlayAllBodyAnimation()) {
		return FName(TEXT("DefaultSlot"));
	}
	return FName(TEXT("UpperBody"));
}

/*----------------------------- RPC 모음 --------------------------------*/
//Server RPC 모음
void APlayer_Character::Server_Aim_Implementation(bool bAiming) {
	SetAimInternal(bAiming);
}
void APlayer_Character::Server_SetAim_Implementation(FVector NewAimPoint) {
	ServerAimPoint = NewAimPoint;
	ForceNetUpdate();
}
void APlayer_Character::Server_SetMoveFacingYaw_Implementation(float Yaw)
{
	ServerMoveFacingYaw = Yaw;
	bHavingServerMoveFacingYaw = true;
}

void APlayer_Character::Server_ClearMoveFacingYaw_Implementation()
{
	bHavingServerMoveFacingYaw = false;
}
void APlayer_Character::Server_SetControlYaw_Implementation(float yaw) {
	ServerControlYaw = yaw;
}
void APlayer_Character::Server_Interaction_Implementation() {
	InteractionInternal();
}
void APlayer_Character::Server_UseItem_Implementation() {
	UseItemInternal();
}
void APlayer_Character::Server_Drop_Implementation() {
	DropInternal();
}
void APlayer_Character::Server_Dodge_Implementation(FVector DodgeDir) {
	DodgeInternal(DodgeDir);
}
void APlayer_Character::Server_ApplyDamage_Implementation(float Damage, APlayer_Character* AttackPlayer)
{
	ApplyDamageInternal(Damage, AttackPlayer, nullptr, NowWeapon ? NowWeapon->WeaponData->bApplyKnockBack : true, NowWeapon ? NowWeapon->WeaponData->bApplyRotation : true, false);
}
void APlayer_Character::Server_Attack_Implementation(bool bHolding, FVector ClientAimPoint) {
	if (!bCanControl) return;
	if (bIsOut) return;
	if (bIsDodging) return;
	if (TransformationComp && !TransformationComp->CanAttackDuringTransformation()) return;
	//무기가 없는데 홀딩 중인 상태라면 무시 (Holding가능한 Usecount가 0인 무기를 던지기 시도 시 HoldAttack과 Attack의 중복 호출 방지)
	if (bHolding && !NowWeapon) return;

	ServerAimPoint = ClientAimPoint;

	//공격 속도로 공격 간 최소 간격 체크 (1 / 공격속도 <- 1초당 공격 가능 횟수)
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float AttackInterval = 1.f / FMath::Max(0.01f, AStat.AttackRate); //0 나누기 방지

	EWeaponAttackInputType InputType = EWeaponAttackInputType::Single;

	if (NowWeapon && NowWeapon->WeaponData) {
		if (FWeaponStats* Stat = NowWeapon->GetWeaponStats()) {
			InputType = Stat->AttackInputType;
		}
	}

	//홀딩 공격이 가능한 무기인지 여부
	bool bIsHoldInputType = InputType == EWeaponAttackInputType::Continuous || InputType == EWeaponAttackInputType::Repeat;

	//물체와 사용횟수가 없는 무기는 AttackInternal에서 던지기 적용
	if (NowObjects)
	{
		GetWorldTimerManager().ClearTimer(AttackEarlierDelayTimerHanlde);
		if (CurrentTime - LastAttackTime < 0.25f) return;
		LastAttackTime = CurrentTime;
		AttackInternal(true);
		return;
	}
	if (NowWeapon && !NowWeapon->CheckUseCounting()) {
		if (bIsHoldInputType && bHolding) {
			//Continuous 타입은 홀딩 상태 정리
			if (InputType == EWeaponAttackInputType::Continuous && bNowHoldingAttack) {
				NowWeapon->ReleaseAttackWeaponFunction();
				bNowHoldingAttack = false;
				OnWeaponChanged.Broadcast();
			}
			return;
		}
		GetWorldTimerManager().ClearTimer(AttackEarlierDelayTimerHanlde);
		//다 쓴 무기를 던지는 것은 그 무기의 공격 속도에 영향을 받지 않음
		if (CurrentTime - LastAttackTime < 0.25f) return;
		LastAttackTime = CurrentTime;
		AttackInternal(true);
		return;
	}


	if (InputType == EWeaponAttackInputType::Continuous) {
		if (!bNowHoldingAttack) {
			if (CurrentTime - LastAttackTime < AttackInterval) return;
			bNowHoldingAttack = true;
			if (NowWeapon && !NowWeapon->BeforeAttackWeaponFunction()) {
				bNowHoldingAttack = false;
				return;
			}
		}
		AttackInternal(false);
		return;
	}

	if (CurrentTime - LastAttackTime < AttackInterval) return;

	if (NowWeapon && !NowWeapon->BeforeAttackWeaponFunction()) {
		LastAttackTime = CurrentTime;
		return;
	}

	LastAttackTime = CurrentTime;
	
	float Delay = AStat.AttackEarlierDelay;

	if (Delay > 0.f) {
		//선딜레이가 있을 경우 애니메이션은 즉시 재생
		PlayEquipmentAnimation(EFunctionInterActionReason::Attack);
		//선딜레이 타이머 초기화 및 실행
		GetWorldTimerManager().ClearTimer(AttackEarlierDelayTimerHanlde);
		
		//공격 입력 시점의 TargetPoint를 고정 (투척 타입, Delay 영향 X)
		bool bFixThrowTarget = AStat.AttackType == EAttackType::Throw;
		FVector FixedAimPoint = ClientAimPoint; 

		GetWorldTimerManager().SetTimer(AttackEarlierDelayTimerHanlde, FTimerDelegate::CreateWeakLambda(this, [this, bFixThrowTarget, FixedAimPoint]() { 
			if (!bCanControl) return;
			if (bIsOut) return;
			if (bIsDodging) return;
			if (TransformationComp && !TransformationComp->CanAttackDuringTransformation()) return;
			if(bFixThrowTarget) ServerAimPoint = FixedAimPoint;
			
			AttackInternal(false); 
		}), Delay, false);
		return;
	}
	
	AttackInternal();
}

void APlayer_Character::Server_AttackRelease_Implementation()
{
	if (!NowWeapon || !NowWeapon->WeaponData) return;
	FWeaponStats* Stat = NowWeapon->GetWeaponStats();
	if (!Stat) return;

	//연속 공격 타입 무기의 공격이 끝난 경우
	if (Stat->AttackInputType == EWeaponAttackInputType::Continuous) {
		//홀딩 여부와 관계없이 Release입력을 무기에 전달
		NowWeapon->ReleaseAttackWeaponFunction();
		
		//홀딩 중이 아니면 Release를 검사할 필요가 없으므로 Return
		if (!bNowHoldingAttack) return;
		bNowHoldingAttack = false;
		LastAttackTime = GetWorld()->GetTimeSeconds();
		OnWeaponChanged.Broadcast();
		return;
	}
	if (Stat->AttackInputType == EWeaponAttackInputType::Repeat) return;
}

void APlayer_Character::Server_HoldAttack_Implementation()
{
	if (!NowWeapon) return;
	NowWeapon->UseWeapon();
}

//Coin 획득 RPC
void APlayer_Character::Server_AddCoin_Implementation(int32 CoinValue) {
	APlayer_State* playerState = GetThePlayerState();
	if (!playerState) return;
	if (CoinValue < 0) return;
	playerState->AddCoin(CoinValue);

	if (APlayMode_Match* MatchMode = GetWorld()->GetAuthGameMode<APlayMode_Match>()) {
		MatchMode->UpdatePlayersRank();
	}
}

//Coin 손실 RPC
void APlayer_Character::Server_SpawnLostCoin_Implementation(int32 Amount) {
	SpawnLostCoins(Amount);
}

//Multi RPC 모음
//피격반응행동 함수
void APlayer_Character::Multicast_PlayHitReaction_Implementation(float Damage, bool _bIsOut, bool bApplyRotation, FVector AttackDir, bool bBigHit) {
	if (_bIsOut) {
		if (OutMontage) PlayAnimMontage(OutMontage);
		return;
	}
	//회피 시 피격 애니메이션 재생 x
	if (bIsDodging) return;

	bIsHitted = true;

	if (bBigHit) {
		bIsBigHitReaction = true;
	}

	if (bApplyRotation) {
		TurnToAttackPlayer(AttackDir);
	}

	PlayDamageAnimation(Damage, bBigHit);
}

//현재 장착물의 일반 공격 모션 재생
void APlayer_Character::Multicast_PlayAnimationDynamic_Implementation(UAnimSequence* Sequence, FName SlotName, float BlendInTime, float BlendOutTime, float PlayRate, int32 LoopCount, int32 StartFrame)
{
	PlayAnimationDynamic(Sequence, SlotName, BlendInTime, BlendOutTime, PlayRate, LoopCount, StartFrame);
}

void APlayer_Character::Multicast_PlayOverrideMontage_Implementation(UAnimMontage* Montage, FName StartSection, bool bRestart, bool bPauseAfter)
{
	if (!Montage) return;
	if (!GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	bool bIsActive = AnimInstance->Montage_IsActive(Montage);

	if (bRestart || !bIsActive) AnimInstance->Montage_Play(Montage, 1.f);
	else AnimInstance->Montage_Resume(Montage);

	if (StartSection != NAME_None) AnimInstance->Montage_JumpToSection(StartSection, Montage);
	if (bPauseAfter) AnimInstance->Montage_Pause(Montage);
}


void APlayer_Character::Multicast_StopSlotAnimation_Implementation(FName SlotName, float BlendOutTime)
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	AnimInstance->StopSlotAnimation(BlendOutTime, SlotName);
}

void APlayer_Character::Multicast_PlayRecoverReaction_Implementation()
{
	if (!RecoverMontage) return;

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;
	
	AnimInstance->Montage_Play(RecoverMontage, 1.f);
}

void APlayer_Character::Multicast_StopMontage_Implementation(UAnimMontage* Montage, float BlendOutTime)
{
	if (!Montage) return;

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

	if (!AnimInstance) return;

	AnimInstance->Montage_Stop(BlendOutTime, Montage);
}
//Client RPC 모음
//탈락 시 가라앉음 연출 RPC(Liquid)
void APlayer_Character::Client_Out_Implementation()
{
	if (GetCharacterMovement()) {
		if (bIsOnLiquidWhenOut) {
			GetCharacterMovement()->GravityScale = 0.f;
			GetCharacterMovement()->Velocity = FVector(0.f, 0.f, bEndMatchState ? -SinkSpeed : 0.f);
		}
		else {
			GetCharacterMovement()->Velocity = FVector(0.f, 0.f, GetCharacterMovement()->Velocity.Z);
		}
	}
	//로컬에서만 서버 위치(실제 위치)에 시각 보정 
	if (IsLocallyControlled()) {
		bOutVisualSmoothing = true;
		LastActorLocation = GetActorLocation();
		VisualMeshLocation = FVector::ZeroVector;
		if (GetMesh()) {
			GetMesh()->SetRelativeLocation(DefaultMeshLocation);
		}
		if (springArmComp) {
			springArmComp->SetRelativeLocation(DefaultCamLocation);
			springArmComp->bEnableCameraLag = true;
			springArmComp->CameraLagSpeed = 3.f;
			springArmComp->CameraLagMaxDistance = 100.f;
		}

		// [사운드] 사망시
		// 캐릭터에 붙였던 귀(이어폰)를 다시 카메라로 원복
		if (APlayerController* PC = Cast<APlayerController>(GetController())) {
			PC->ClearAudioListenerOverride();
		}
	}
}

//플레이어 화면에 추가 이미지를 띄우기 시작
void APlayer_Character::Client_StartAdditionalImage_Implementation(int32 ImageID)
{
	if (!AdditionalImageWidgetClass) return;
	
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController()) return;

	if (!AdditionalImageWidget) {
		AdditionalImageWidget = CreateWidget<UPlayer_AdditionalWidget>(PC, AdditionalImageWidgetClass);
		if (!AdditionalImageWidget) return;

		AdditionalImageWidget->AddToViewport();
	}

	AdditionalImageWidget->SetImageByID(ImageID);
}

//플레이어 화면에 떠있던 추가 이미지 제거
void APlayer_Character::Client_EndAdditionalImage_Implementation()
{
	if (AdditionalImageWidget) {
		AdditionalImageWidget->PlayFadeOutAndRemove();
		AdditionalImageWidget = nullptr;
	}
}

// [사운드]====================================================================
// [사운드] 아이템 사용시 효과음 재생
void APlayer_Character::Multicast_PlayItemUseSound_Implementation(USoundBase* ItemUseSound)
{
	UE_LOG(LogTemp, Error, TEXT("Function : play"));
	if (ItemUseSound) {
		// 아이템 사용 사운드는 본인 몸에서 나오도록 붙임
		UGameplayStatics::SpawnSoundAttached(ItemUseSound, GetMesh(), NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, false, 1.f, 1.f, 0.f, nullptr);
		UE_LOG(LogTemp, Error, TEXT("Function : play Sound"));
	}
}

// [사운드] 오브젝트 폭발음 등 재생시(오브젝트가 파괴될 때 사운드)
void APlayer_Character::Multicast_PlyaObjectSound_Implementation(USoundBase* ObjectDestroySound, FVector PlayLocation)
{
	if (ObjectDestroySound) {
		UGameplayStatics::PlaySoundAtLocation(this, ObjectDestroySound, PlayLocation, 1.f, 1.f, 0.f, nullptr);
	}
}



/*체크해보기*/
//FMath::Max(A, B) -> A와 B 중 큰 쪽을 반환하는 함수
//OnRep -> 서버가 값을 변경했고 그 값이 Client에 반영되었을 경우 호출되는 함수
