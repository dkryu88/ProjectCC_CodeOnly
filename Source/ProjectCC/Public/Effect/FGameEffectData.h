// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "FGameEffectData.generated.h"

class AActor;
class USceneComponent;

UENUM(BlueprintType)
enum class EEffectType : uint8
{
	Spawn,
	Hit,
	Destroy,
	Custom
};

UENUM(BlueprintType)
enum class EGameEffectSpawnLocationType : uint8 {
	WorldLocation,
	ActorLocation,
	SocketLocation,
	AttackRangeStart,
	HitPoint,
};

UENUM(BlueprintType)
enum class EGameEffectSpawnRotationType : uint8{
	WorldRotation,
	ActorRotation,
	SocketRotation,
	ForwardRotation,
	HitNormalRotation
};

USTRUCT(BlueprintType)
struct FGameEffectContext {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator WorldRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HitPoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HitNormal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector AttackRangeStart = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName OverrideSocketName = NAME_None;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY()
	TObjectPtr<USceneComponent> SourceComponent = nullptr;
};

USTRUCT(BlueprintType)
struct FGameEffectFloatParam {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParamName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.f;
};

USTRUCT(BlueprintType)
struct FGameEffectVectorParam {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParamName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Value = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FGameEffectBoolParam {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParamName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Value = false;
};

USTRUCT(BlueprintType)
struct FGameEffectRuntimeParams {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameEffectFloatParam> FloatParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameEffectVectorParam> VectorParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameEffectBoolParam> BoolParams;

	void AddFloatParam(FName ParamName, float Value) {
		if (ParamName.IsNone()) return;

		FGameEffectFloatParam NewParam;
		NewParam.ParamName = ParamName;
		NewParam.Value = Value;
		FloatParams.Add(NewParam);
	}

	void AddVectorParam(FName ParamName, FVector Value) {
		if (ParamName.IsNone()) return;

		FGameEffectVectorParam NewParam;
		NewParam.ParamName = ParamName;
		NewParam.Value = Value;
		VectorParams.Add(NewParam);
	}

	void AddBoolParam(FName ParamName, bool Value) {
		if (ParamName.IsNone()) return;

		FGameEffectBoolParam NewParam;
		NewParam.ParamName = ParamName;
		NewParam.Value = Value;
		BoolParams.Add(NewParam);
	}

	bool TryGetFloat(FName ParamName, float& OutValue) const {
		for (const FGameEffectFloatParam& Param : FloatParams) {
			if (Param.ParamName == ParamName) {
				OutValue = Param.Value;
				return true;
			}
		}

		return false;
	}

	bool TryGetVector(FName ParamName, FVector& OutValue) const {
		for (const FGameEffectVectorParam& Param : VectorParams) {
			if (Param.ParamName == ParamName) {
				OutValue = Param.Value;
				return true;
			}
		}

		return false;
	}

	bool TryGetBool(FName ParamName, bool& OutValue) const {
		for (const FGameEffectBoolParam& Param : BoolParams) {
			if (Param.ParamName == ParamName) {
				OutValue = Param.Value;
				return true;
			}
		}

		return false;
	}
};

USTRUCT(BlueprintType)
struct FGameEffectData {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> NiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EEffectType Type = EEffectType::Spawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EGameEffectSpawnLocationType SpawnLocationType = EGameEffectSpawnLocationType::WorldLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EGameEffectSpawnRotationType SpawnRotationType = EGameEffectSpawnRotationType::WorldRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector LocationOffset = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Scale = FVector::OneVector;
};
