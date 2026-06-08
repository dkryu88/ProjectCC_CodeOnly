// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Item.h"
#include "Weapon.h"
#include "Player_State.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCoinChanged, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRankChanged, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEliminateChanged, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOutChanged, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnItemChanged, TSubclassOf<AItem>, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNicknameChanged, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPortraitIdChanged, int32);

/**
 * 서버에서 실행 되어야 하는 플레이어 상태(코인 갯수, 적을 탈락시킨 수 등)을 지정
 */

//LV_Ready에서 플레이어에게 보여줄 로딩 상태
UENUM(BlueprintType)
enum class EReadySyncState : uint8
{
	None,
	JoinedReadyLevel,
	ProfileSynced,
	ReadyScreenLoaded,
	ReadyToTravel
};

UCLASS()
class PROJECTCC_API APlayer_State : public APlayerState
{
	GENERATED_BODY()
	
public:
	APlayer_State();

	virtual void CopyProperties(APlayerState* NewPlayerState) override;
	virtual void OverrideWith(APlayerState* OldPlayerState) override;

public:
	//UI Delegate 바인딩
	FOnCoinChanged OnCoinChanged;
	FOnRankChanged OnRankChanged;
	FOnEliminateChanged OnEliminateChanged;
	FOnOutChanged OnOutChanged;
	FOnItemChanged OnItemChanged;
	FOnNicknameChanged OnNicknameChanged;
	FOnPortraitIdChanged OnPortraitIdChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerCoin, BlueprintReadOnly, Category="PlayerState")
	int32 PlayerCoin = 0;
	UPROPERTY(ReplicatedUsing = OnRep_PlayerRank, BlueprintReadOnly, Category="PlayerState")
	int32 PlayerRank = 1;
	UPROPERTY(ReplicatedUsing = OnRep_Eliminate, BlueprintReadOnly, Category="PlayerState")
	int32 PlayerEliminate = 0;
	UPROPERTY(ReplicatedUsing = OnRep_Out, BlueprintReadOnly, Category="PlayerState")
	int32 PlayerOut = 0;
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItem)
	TSubclassOf<AItem> EquippedItem;
	UPROPERTY(Replicated)
	TSubclassOf<AWeapon> ReservedWeapon;
	UPROPERTY(Replicated)
	int32 ItemUseCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerNickname, BlueprintReadOnly)
	FString NickName;
	UPROPERTY(ReplicatedUsing = OnRep_PortraitId, BlueprintReadOnly)
	int32 PortraitId = -1;
	UPROPERTY(Replicated, BlueprintReadOnly)
	EReadySyncState ReadySyncState = EReadySyncState::None;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 StartSpawnSlotIndex = -1;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bMatchLevelLoaded = false;
public:
	UFUNCTION()
	void OnRep_PlayerNickname();
	UFUNCTION()
	void OnRep_PlayerCoin();
	UFUNCTION()
	void OnRep_PlayerRank();
	UFUNCTION()
	void OnRep_Eliminate();
	UFUNCTION()
	void OnRep_Out();
	UFUNCTION()
	void OnRep_EquippedItem();
	UFUNCTION()
	void OnRep_PortraitId();
public:
	void AddPlayerEliminate();
	void AddPlayerOut();
	void AddCoin(int32 CoinValue);
	void SetPlayerCoin(int32 coin);
	void SetPlayerRank(int32 rank);
	void SetPlayerEliminate(int32 Eliminate);
	void SetPlayerOut(int32 Out);
	void SetEquippedItem(TSubclassOf<AItem> TheItem, int32 TheUseCount);
	void SetNickName(const FString& nickname);
	void SetPortraitId(int32 portraitId);
	void SetReadySyncState(EReadySyncState State);
	void SetMatchLevelLoaded(bool bLoaded);
	void SetSpawnSlotIndex(int32 Index);
	void SetReservedWeapon(TSubclassOf<AWeapon> weapon);
	void ClearEquippedItem();
	void ClearReservedWeapon();

	int32 GetPlayerCoin() const { return PlayerCoin; }
	int32 GetPlayerRank() const { return PlayerRank; } 
	int32 GetPlayerEliminate() const { return PlayerEliminate; }
	int32 GetPlayerOut() const { return PlayerOut; }
	TSubclassOf<AItem> GetEquippedItem() const { return EquippedItem; }
	TSubclassOf<AWeapon> GetReservedWeapon() const { return ReservedWeapon; }
	int32 GetItemUseCount() const { return ItemUseCount; }
	const FString& GetNickName() const { return NickName; }
	int32 GetPortraitId() const { return PortraitId; }
	EReadySyncState GetReadySyncState() const { return ReadySyncState; }
	int32 GetSpawnSlotIndex() const { return StartSpawnSlotIndex; }
	bool IsMatchLevelLoaded() const { return bMatchLevelLoaded; }

	bool CheckEquippedItem() const { return EquippedItem != nullptr; }
	bool CheckReservedWeapon() const { return ReservedWeapon != nullptr; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

/*알아두기
PlayerState(Player_State의 원형)은 언리얼에서 각각의 플레이어에게 생성함
즉 여기에 있는 Property들은 플레이어들이 각각 가짐 <-- 멀티에서 유용
*/