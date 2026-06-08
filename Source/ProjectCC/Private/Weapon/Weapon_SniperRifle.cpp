// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon_SniperRifle.h"
#include "Player_Character.h"
#include "ETC/Weapon_SniperScopeWidget.h"
#include "Camera/CameraActor.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"

AWeapon_SniperRifle::AWeapon_SniperRifle()
{
	bReplicates = true;
}

void AWeapon_SniperRifle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon_SniperRifle, bIsScopeModeActive);
	DOREPLIFETIME(AWeapon_SniperRifle, ScopeCameraActor);
}

bool AWeapon_SniperRifle::InteractionWeaponFunction(EFunctionInterActionReason Reason)
{
	if (!EquippedPlayer) return true;
	switch (Reason) {
	case EFunctionInterActionReason::Aim:
	{
		if (!CheckUseCounting()) return true;
		ToggleScopeMode();
		return false;
	}
	case EFunctionInterActionReason::Attack:
	{
		if (!bIsScopeModeActive) return true;
		return false;
	}
	case EFunctionInterActionReason::Jump:
		if (!bIsScopeModeActive) return true;
		bIsScopeModeActive = false;
		ApplyScopeModeEffects(false);

		if (!HasAuthority()) Server_SyncScopeState(false);
		else Client_ForceExitScope();

		return true;

	case EFunctionInterActionReason::Dodge:
	case EFunctionInterActionReason::InterAction:
	case EFunctionInterActionReason::Drop:
	case EFunctionInterActionReason::Hitted:
	{
		if (bIsScopeModeActive) {
			bIsScopeModeActive = false;
			ApplyScopeModeEffects(false);

			if (!HasAuthority()) Server_SyncScopeState(false);
			else Client_ForceExitScope();
		}
		return true;
	}
	default: return true;
	}
}

bool AWeapon_SniperRifle::BeforeAttackWeaponFunction()
{
	if (!EquippedPlayer) return false;
	if (!bIsScopeModeActive) return true;
	
	if (!CheckUseCounting()) {
		ApplyScopeModeEffects(false);

		if (HasAuthority())
		{
			Client_ForceExitScope();
		}
		else
		{
			Server_SyncScopeState(false);
		}

		return false;
	}

	if (EquippedPlayer->IsLocallyControlled()) {
		Local_RequestFire();
	}

	return false;
}

float AWeapon_SniperRifle::OnPreHit(APlayer_Character* TPlayer, bool& bSkipRotation) {
	bSkipRotation = false;
	if (bIsScopeModeActive) return 1.f;
	return 0.25f;
}

//스나이퍼는 Preview를 생성하지 않음
bool AWeapon_SniperRifle::BuildAimPreviewData(APlayer_Character* Player, FAimPreviewVisualData& PreviewData)
{
	PreviewData.Reset();
	return false;
}

void AWeapon_SniperRifle::EquipEffect_Implementation(APlayer_Character* Player) {
	if (!Player) return;

	Client_InitializeScopeUI(Player);
	if (HasAuthority())
	{
		SetOwner(Player);
		Player->OnTakeAnyDamage.AddDynamic(this, &AWeapon_SniperRifle::HandleOwnerTakenDamage);
	}
}

void AWeapon_SniperRifle::UnEquipEffect_Implementation(APlayer_Character* Player)
{
	if (IsValid(Player) && HasAuthority()) {
		Player->OnTakeAnyDamage.RemoveDynamic(this, &AWeapon_SniperRifle::HandleOwnerTakenDamage);
	}
	//저격 모드를 사용하지 않은 상태에서 무기 해제 시 회전값이 0,0,0으로 초기화되는 현상 수정
	if (bIsScopeModeActive) {
		bIsScopeModeActive = false;
		ApplyScopeModeEffects(false);
	}
}


void AWeapon_SniperRifle::AdditionalUnEquipWeaponFunction()
{
	if (bIsScopeModeActive) {
		bIsScopeModeActive = false;
		ApplyScopeModeEffects(false);

		if (HasAuthority()) {
			Client_ForceExitScope();
		}
		else {
			Server_SyncScopeState(false);
		}
	}
}

void AWeapon_SniperRifle::ToggleScopeMode() {
	if (!EquippedPlayer) return;
	bool bNewState = !bIsScopeModeActive;
	ApplyScopeModeEffects(bNewState);

	//서버 동기화
	if (!HasAuthority()) {
		Server_SyncScopeState(bNewState);
	}
}

void AWeapon_SniperRifle::ApplyScopeModeEffects(bool bEnabled)
{
	if (!bEnabled) {
		bIsScopeModeActive = false;
		if (ScopeWidget && ScopeWidget->IsInViewport()) {
			ScopeWidget->RemoveFromParent();
		}

		GetWorldTimerManager().ClearTimer(ScopeLogicTimerHandle);
	}

	if (!EquippedPlayer) return;

	EquippedPlayer->bIsDodgeLocked = bEnabled;
	EquippedPlayer->bIsInteractionLocked = bEnabled;
	EquippedPlayer->bCanCamControl = !bEnabled;

	if (bEnabled) {
		EquippedPlayer->AddSpeedController(TEXT("SniperZoom"), 0.f, 0.f);

		if (EquippedPlayer->GetCharacterMovement()) {
			EquippedPlayer->GetCharacterMovement()->StopMovementImmediately();
		}
	}
	else {
		EquippedPlayer->RemoveSpeedControllerByName(TEXT("SniperZoom"));
	}

	APlayerController* PC = Cast<APlayerController>(EquippedPlayer->GetController());
	if (!PC) return;

	if (bEnabled) {
		bIsScopeModeActive = true;
		ScopeEntryRotation = PC->GetControlRotation();

		if (!ScopeCameraActor)
		{
			FActorSpawnParameters SpawnParams;
			ScopeCameraActor = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), SpawnParams);
			
			if (ScopeCameraActor) {
				ScopeCameraActor->SetReplicates(true);
				ScopeCameraActor->bAlwaysRelevant = true;
			}
		}

		if (!ScopeCameraActor) return;

		VirtualCursorOffset = FVector2D::ZeroVector;

		const FVector CamLocation = EquippedPlayer->GetActorLocation() + FVector(0.f, 0.f, ScopeCameraHeight);

		ScopeCameraActor->SetActorLocation(CamLocation);
		ScopeCameraActor->SetActorRotation(FRotator(-90.f, ScopeEntryRotation.Yaw, 0.f));

		PC->SetViewTargetWithBlend(ScopeCameraActor, ToScopeCameraBlendTime);

		if (EquippedPlayer->IsLocallyControlled()) {
			int32 SizeX = 0;
			int32 SizeY = 0;
			PC->GetViewportSize(SizeX, SizeY);

			PC->SetMouseLocation(SizeX / 2, SizeY / 2);
			CurrentCrosshairPos = FVector2D(SizeX / 2.f, SizeY / 2.f);

			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;

			if (!ScopeWidget && ScopeWidgetClass) {
				ScopeWidget = CreateWidget<UUserWidget>(PC, ScopeWidgetClass);
			}

			if (ScopeWidget && !ScopeWidget->IsInViewport()) {
				ScopeWidget->AddToViewport();

				ScopeWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				ScopeWidget->SetPositionInViewport(FVector2D(0.f, 0.f), false);
			}

			GetWorldTimerManager().SetTimer(ScopeLogicTimerHandle, this, &AWeapon_SniperRifle::UpdateScopeLogic, LogicUpdateInterval, true);
		}
	}
	else {
		USpringArmComponent* Arm = EquippedPlayer->FindComponentByClass<USpringArmComponent>();
		if (Arm) {
			Arm->bEnableCameraRotationLag = false;
			Arm->bEnableCameraLag = false;
		}

		PC->SetControlRotation(ScopeEntryRotation);
		PC->SetViewTargetWithBlend(EquippedPlayer, ToOriginCameraBlendTime);

		if (Arm) {
			Arm->UpdateChildTransforms();

			GetWorldTimerManager().SetTimerForNextTick([Arm]() {
				if (Arm) {
					Arm->bEnableCameraRotationLag = true;
					Arm->bEnableCameraLag = true;
				}
			});
		}

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
}

void AWeapon_SniperRifle::UpdateScopeLogic() {
	if (!bIsScopeModeActive || !EquippedPlayer || !ScopeCameraActor) return;
	if(!EquippedPlayer->IsLocallyControlled()) return;

	APlayerController* PC = Cast<APlayerController>(EquippedPlayer->GetController());
	if (!PC) return;

	float MouseX = 0.f;
	float MouseY = 0.f;
	PC->GetInputMouseDelta(MouseX, MouseY);

	UE_LOG(LogTemp, Warning, TEXT("[SniperScope] MouseDelta X=%.3f Y=%.3f"), MouseX, MouseY);

	int32 SizeX = 0;
	int32 SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);

	FVector2D Center(SizeX / 2.f, SizeY / 2.f);
	float MaxRadius = SizeY / 4.f;

	VirtualCursorOffset.X += MouseX * MoveSensitivity;
	VirtualCursorOffset.Y += MouseY * MoveSensitivity;

	if (VirtualCursorOffset.Size() > MaxRadius) {
		VirtualCursorOffset = VirtualCursorOffset.GetSafeNormal() * MaxRadius;
	}

	FVector RightDir = ScopeCameraActor->GetActorRightVector();
	RightDir.Z = 0.f;
	RightDir = RightDir.GetSafeNormal();

	FVector UpDir = FVector::CrossProduct(RightDir, FVector::UpVector).GetSafeNormal();
	
	FVector CurrentCamLocation = ScopeCameraActor->GetActorLocation();
	FVector MovementDelta = (RightDir * VirtualCursorOffset.X + UpDir * VirtualCursorOffset.Y) * ScrollSpeedMultiplier;
	FVector NewCamLocation = CurrentCamLocation + MovementDelta;

	FVector PlayerBaseLocation = EquippedPlayer->GetActorLocation() + FVector(0.f, 0.f, ScopeCameraHeight);

	FVector ToTarget = NewCamLocation - PlayerBaseLocation;
	ToTarget.Z = 0.f;

	if (ToTarget.Size() > MaxScrollDistance) {
		FVector BoundaryNormal = ToTarget.GetSafeNormal();
		FVector SlideDelta = FVector::VectorPlaneProject(MovementDelta, BoundaryNormal);
		NewCamLocation = CurrentCamLocation + SlideDelta;

		FVector FinalDir = NewCamLocation - PlayerBaseLocation;
		FinalDir.Z = 0.f;
		FinalDir = FinalDir.GetSafeNormal();

		NewCamLocation = PlayerBaseLocation + FinalDir * (MaxScrollDistance - 1.f);
	}

	FHitResult CameraHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(EquippedPlayer);
	QueryParams.AddIgnoredActor(this);

	float CameraCollisionRadius = 100.f;
	FCollisionShape SweepShape = FCollisionShape::MakeSphere(CameraCollisionRadius);

	bool bIsCamBlocked = GetWorld()->SweepSingleByChannel(CameraHit, CurrentCamLocation, NewCamLocation, FQuat::Identity, ECC_Camera, SweepShape, QueryParams);
	
	if (bIsCamBlocked) {
		FVector SlideDelta = FVector::VectorPlaneProject(MovementDelta, CameraHit.Normal);
		NewCamLocation = CurrentCamLocation + SlideDelta;
	}

	NewCamLocation.Z = EquippedPlayer->GetActorLocation().Z + ScopeCameraHeight;
	ScopeCameraActor->SetActorLocation(NewCamLocation);

	float CrosshairMaxRadius = SizeY / 32.f;
	FVector2D TargetCrosshairOffset = VirtualCursorOffset * (CrosshairMaxRadius / MaxRadius);

	FVector2D TargetPos;
	TargetPos.X = Center.X + TargetCrosshairOffset.X;
	TargetPos.Y = Center.Y - TargetCrosshairOffset.Y;

	CurrentCrosshairPos = FMath::Vector2DInterpTo(CurrentCrosshairPos, TargetPos, LogicUpdateInterval, CrosshairInterpSpeed);

	if (UWeapon_SniperScopeWidget* SniperWidget = Cast< UWeapon_SniperScopeWidget>(ScopeWidget)) {
		SniperWidget->SetScopeScreenPosition(CurrentCrosshairPos);
	}

	if (IsValid(ScopeCameraActor)) {
		ScopeCameraActor->SetActorRotation(FRotator(-90.f, ScopeEntryRotation.Yaw, 0.f));
	}

	// 디버그: 저격 이동 가능 범위
	const FVector CircleCenter =
		EquippedPlayer->GetActorLocation() + FVector(0.f, 0.f, ScopeCameraHeight - 300.f);

	DrawDebugCircle(
		GetWorld(),
		CircleCenter,
		MaxScrollDistance,
		100,
		FColor::Cyan,
		false,
		-1.f,
		0,
		5.f,
		FVector(1, 0, 0),
		FVector(0, 1, 0),
		false
	);
}

void AWeapon_SniperRifle::Local_RequestFire() {
	if (!EquippedPlayer || !EquippedPlayer->IsLocallyControlled()) return;
	if (!bIsScopeModeActive) return;

	APlayerController* PC = Cast<APlayerController>(EquippedPlayer->GetController());
	if (!PC) return;

	FVector WorldLocation;
	FVector WorldDirection;

	if (!PC->DeprojectScreenPositionToWorld(CurrentCrosshairPos.X, CurrentCrosshairPos.Y, WorldLocation, WorldDirection)) {
		return;
	}

	FCollisionObjectQueryParams AllObjects;
	AllObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	AllObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
	AllObjects.AddObjectTypesToQuery(ECC_Pawn);
	AllObjects.AddObjectTypesToQuery(ECC_PhysicsBody);
	AllObjects.AddObjectTypesToQuery(ECC_GameTraceChannel4);
	AllObjects.AddObjectTypesToQuery(ECC_GameTraceChannel5);

	FHitResult GroundHIt;

	FVector Start = WorldLocation;
	FVector End = Start + WorldDirection * FireMaxRange;

	if (GetWorld()->LineTraceSingleByObjectType(GroundHIt, Start, End, AllObjects)) {
		FVector TargetPoint = GroundHIt.ImpactPoint;
		FVector TraceStart = TargetPoint + FVector(0.f, 0.f, ScopeCameraHeight);
		FVector TraceEnd = TargetPoint - FVector(0.f, 0.f, 50.f);

		Server_ExecuteFireTrace(TraceStart, TraceEnd);

		if (bAutoExitScopeAfterFire) {
			ApplyScopeModeEffects(false);
		}
	}
}

void AWeapon_SniperRifle::HandleOwnerTakenDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Error, TEXT("Snipermode Damaged!!!!!!"));
	// 피격 시 즉시 저격 모드 해제
	if (bIsScopeModeActive && bAutoExitScopeOnDamage) {
		bIsScopeModeActive = false;
		ApplyScopeModeEffects(false);
		Client_ForceExitScope();
	}
}

//Server/Client RPC 모음
void AWeapon_SniperRifle::Client_InitializeScopeUI_Implementation(APlayer_Character* Player) {
	if (!Player || !Player->IsLocallyControlled()) return;

	APlayerController* PC = Player->GetLocalViewingPlayerController();

	if (PC && ScopeWidgetClass && !ScopeWidget) {
		ScopeWidget = CreateWidget<UUserWidget>(PC, ScopeWidgetClass);
	}
}

void AWeapon_SniperRifle::Server_SyncScopeState_Implementation(bool bNewZoom) {
	bIsScopeModeActive = bNewZoom;
	ApplyScopeModeEffects(bNewZoom);
}

void AWeapon_SniperRifle::Server_ExecuteFireTrace_Implementation(FVector TraceStart, FVector TraceEnd) {
	if (!EquippedPlayer) return;
	if (!WeaponData) return;
	if (!bIsScopeModeActive) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float AttackInterval = 1.f / FMath::Max(0.1f, GetWeaponStats()->AttackRate);

	if (CurrentTime - EquippedPlayer->LastAttackTime < AttackInterval) return;

	EquippedPlayer->LastAttackTime = CurrentTime;

	UseWeapon();

	if (EquippedPlayer) {
		EquippedPlayer->OnWeaponChanged.Broadcast();
		OnWeaponUseCountChanged.Broadcast();
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(EquippedPlayer);
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel5);

	FHitResult Hit;
	const FCollisionShape SweepShape = FCollisionShape::MakeSphere(BulletSweepRadius);

	const bool bHit = GetWorld()->SweepSingleByObjectType(
		Hit,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ObjectParams,
		SweepShape,
		QueryParams
	);

	if (bHit)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			float FinalDamage = GetWeaponStats()->Attack;

			if (APlayer_Character* HitPlayer = Cast<APlayer_Character>(HitActor))
			{
				bool bSkipRot = false;
				FinalDamage *= OnPreHit(HitPlayer, bSkipRot);
			}

			UGameplayStatics::ApplyDamage(
				HitActor,
				FinalDamage,
				EquippedPlayer->GetController(),
				EquippedPlayer,
				nullptr
			);

			ApplyHitEffect(HitActor);
		}
	}

	FVector CapsuleCenter = (TraceStart + TraceEnd) * 0.5f;
	float CapsuleHalfHeight = FVector::Dist(TraceStart, TraceEnd) * 0.5f;
	FQuat CapsuleRot = FRotationMatrix::MakeFromZ(TraceStart - TraceEnd).ToQuat();

	DrawDebugCapsule(
		GetWorld(),
		CapsuleCenter,
		CapsuleHalfHeight + 50.f,
		BulletSweepRadius,
		CapsuleRot,
		FColor::Red,
		false,
		2.f
	);

	if (!CheckUseCounting() || bAutoExitScopeAfterFire) {
		ApplyScopeModeEffects(false);
		Client_ForceExitScope();
		return;
	}
}

void AWeapon_SniperRifle::Client_ForceExitScope_Implementation()
{
	ApplyScopeModeEffects(false);
}