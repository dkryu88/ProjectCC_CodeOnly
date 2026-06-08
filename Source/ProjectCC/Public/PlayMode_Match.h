// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Shop/ShopStock.h"
#include "Match_EventListDataAsset.h"
#include "PlayMode_Match.generated.h"

/**
 * 매치 내에서 사용하는 GameMode
 */
class APlayer_Character;
class AMatch_PlayerController;
class APlayerController;
class APlayer_State;
class AMapConstructor;
class ATargetPoint;
class ACoin;
class UWeaponListDataAsset;
class UItemListDataAsset;
class UObjectsListDataAsset;
class UMatch_EventListDataAsset;
class AMatch_Event;

UENUM()
enum class EGrade : uint8 {
	Grade_B,
	Grade_A,
	Grade_S
};

USTRUCT()
struct FRespawnPoint {
	GENERATED_BODY()
	
	UPROPERTY()
	TObjectPtr<ATargetPoint> Point = nullptr;

	UPROPERTY()
	float NearestPlayerDistanceSq = -1.f;
};

USTRUCT()
struct FRespawnInfo {
	GENERATED_BODY()

	//자동 리스폰 타이머
	FTimerHandle AutoRespawnTimerHandle;
	//리스폰 활성화 타이머
	FTimerHandle ActiveRespawnTimerHandle;

	bool bCanRespawn = false;
};

UCLASS()
class PROJECTCC_API APlayMode_Match : public AGameMode
{
	GENERATED_BODY()
	
public:
	APlayMode_Match();

protected:
	virtual void BeginPlay() override;

protected:
	//매치 진행 시간 (60 * 5 = 300 <-- 5분)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match")
	int32 MatchDuration = 300;
	//매치 진행 시간 관리
	FTimerHandle MatchTimerHandle;
	//매치 시작 딜레이 타이머
	FTimerHandle MatchDelayTimerHandle;
	//매치 시작 카운트다운 시작 타이머
	FTimerHandle CountdownTimerHandle;
	int32 CurrentCountdownNumber = -1;
	//매치 종료 화면 이동 타이머
	FTimerHandle GoResultLevelTimerHandle;
	FTimerHandle HostGoResultLevelTimerHandle;

	//전체 플레이어 수
	UPROPERTY()
	int32 MaxPlayers = 0;
	//현재 맵 확인
	UPROPERTY()
	TObjectPtr<AMapConstructor> CurrentMap = nullptr;
	//게임에 사용할 무기 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AllWeapon")
	TObjectPtr<UWeaponListDataAsset> AllWeaponList;
	//게임에 사용할 아이템 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AllWeapon")
	TObjectPtr<UItemListDataAsset> AllItemList;
	//게임에 사용할 물체 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AllWeapon")
	TObjectPtr<UObjectsListDataAsset> AllObjectsList;
	//근처에 플레이어 캐릭터가 있으면 리스폰이 되지 않는 거리
	UPROPERTY()
	float NonRespawnRadius = 1000.f;
	//리스폰 지점들
	UPROPERTY()
	TArray<TObjectPtr<ATargetPoint>> RespawnPoints;
	//리스폰 최소 대기시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Out")
	float RespawnTime = 5.f;
	//리스폰 최대 대기 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Out")
	float MaxRespawnWaitTime = 10.f;
	//리스폰 지점을 OverPoint를 사용할지 여부
	UPROPERTY(VisibleAnywhere, Category = "RespawnPoint")
	bool bUseOverRespawnPoints = false;
	/*--------------------Random Event----------------------*/
	//공용 매치 이벤트 리스트
	UPROPERTY(EditDefaultsOnly, Category="MatchEvent")
	TObjectPtr<UMatch_EventListDataAsset> Match_CommonEventList;
	//매치 이벤트 발동 시간
	UPROPERTY(EditDefaultsOnly ,Category="MatchEvent")
	TArray<int32> MatchEventTimes = { 225, 75 };
	//지금 발동중인 이벤트들
	UPROPERTY()
	TArray<TObjectPtr<AMatch_Event>> ActiveMatchEvents;
	//이미 발동했던 이벤트
	UPROPERTY()
	TSet<TSubclassOf<AMatch_Event>> UsedEvents;
	//이벤트 알림 경고 시간
	UPROPERTY(EditDefaultsOnly, Category="MatchEvent | UI")
	int32 MatchEventNoticeSeconds = 30;
	//현재 이벤트를 보류 중인지 여부
	UPROPERTY()
	bool bPendingMatchEvent = false;
	//현재 이벤트를 발동 중인지 여부
	UPROPERTY()
	bool bActiveMatchEvent = false;
	//현재 보류중인 이벤트 데이터
	UPROPERTY()
	FMatchEventData PendingEventData;
	//현재 발동중인 이벤트 이름
	UPROPERTY()
	FName CurrentEventName = NAME_None;
	//현재 발동중인 이벤트 남은 시간
	UPROPERTY()
	float CurrentEventTime = -1.f;
	//현재 몇 번째 매치 이벤트인지 확인
	int32 MatchEventTimeIndex = 0;
	//PortraitId 바인딩
	void AssignPortraitId();
	//매치 이벤트 시간 체크
	void CheckMapEvent(int32 RemainingTime);
	//랜덤 이벤트 데이터를 리스트에서 획득
	bool SelectRandomMatchEvent(FMatchEventData& EventData);
	void StartMatchEvent();
	void StopMatchEvent();
	void ApplyActiveEventToPlayer(APlayer_Character* Player);
	//UI 갱신
	void UpdateMatchEventNoticeUI(int32 RemainingTime);
	void UpdateActiveMatchEventUI();
	void NotifyMatchEventFinished(AMatch_Event* FinishedEvent);
	bool PrepareNextMatchEvent();

	void BroadcastMatchEventCountdown(FName EventName, int32 SecondsUntilEvent);
	void BroadcastMatchEventActive(FName EventName, int32 RemainingSeconds);
	void BroadcastMatchEventEnded();
	/* ----------------------Coin Wave-------------------------- */
	UPROPERTY(EditDefaultsOnly, Category="CoinWave")
	TSubclassOf<ACoin> Coin;
	UPROPERTY(EditDefaultsOnly, Category="CoinWave")
	TArray<int32> CoinWaveTime;
	UPROPERTY(EditDefaultsOnly, Category="CoinWave")
	int32 CoinCount = 50;
	UPROPERTY(EditDefaultsOnly, Category="CoinWave")
	float CoinSpawnZOffset = 50.f;
	UPROPERTY(VisibleAnywhere, Category="CoinWave")
	int32 CoinWaveTimeIndex = 0;
	UPROPERTY(VisibleAnywhere, Category="CoinWave")
	bool bSpawnedCoins = false;
	UFUNCTION()
	void CheckCoinWave(int32 RemainingTime);
	UFUNCTION()
	void CoinWave();
	/* ------------------------Supply---------------------------- */
	/*-----------확률------------*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category =" Supply")
	int32 PercentOfB = 60;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply")
	int32 PercentOfA = 30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply")
	int32 PercentOfS = 10;
	/*-----------------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Supply")
	float SupplyCheckInterval = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Supply")
	float SpawnObjectsCheckInterval = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Supply")
	int32 SupplyWeaponMaxCount = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply")
	int32 SupplyItemMaxCount = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Supply")
	float SupplySpawnZOffset = 150.f;
	//무기/아이템 보급 타이머
	FTimerHandle SupplyTimerHandle; 
	//물체 소환 타이머
	FTimerHandle SpawnObjectsTimerHandle;
	UFUNCTION()
	void SupplyWeaponAndItem();
	UFUNCTION()
	void SupplyObjects();
	UFUNCTION(BlueprintCallable, Category = "Supply")
	int32 CountWorldWeapon();
	UFUNCTION(BlueprintCallable, Category = "Supply")
	int32 CountWorldItem();
	UFUNCTION(BlueprintCallable, Category="Supply")
	TSubclassOf<AWeapon> SelectSupplyWeapon();
	UFUNCTION(BlueprintCallable, Category = "Supply")
	TSubclassOf<AItem> SelectSupplyItem();
	UFUNCTION(BlueprintCallable, Category = "Supply")
	TSubclassOf<AObjects> SelectSpawnObjects();
	UFUNCTION(BlueprintCallable, Category = "Supply")
	bool SpawnSupplyWeapon();
	UFUNCTION(BlueprintCallable, Category = "Supply")
	bool SpawnSupplyItem();
	UFUNCTION(BlueprintCallable, Category = "Supply")
	bool SpawnSupplyObjects();
public:
	UFUNCTION(BlueprintCallable)
	AMapConstructor* GetCurrentMap() { return CurrentMap; }

protected:
	//매치 시작 기능
	void StartMatchLogic();
	//매치 시작 시 모든 플레이어를 리스폰 지점에서 스폰
	void SpawnAllPlayersAtRespawnPoint();
	//매치 진행 Timer 관리
	void UpdateMatchTimer();
	//매치 종료 기능
	void EndMatchLogic();
	//모든 플레이어의 매치 결과 데이터를 저장하고 모든 로컬 플레이어에게 전달
	void SendMatchResultsToPlayers();
	//모든 플레이어(클라이언트, 호스트)를 매치 결과 화면으로 이동(세션 종료)
	void SendClientsToResultLevel();
	void SendHostToResultLevel();

	/*상점 이용 관련 함수*/
	//선정될 무기/아이템의 등급 결정
	EGrade GetRandomGrade();
	//상점의 물품 가격 획득
	int32 GetShopPrice(EShopBoxs Box);
	//구매한 박스의 결과를 획득
	bool GetRandomRewardInShopBox(APlayer_State* PS, EShopBoxs Box, TSubclassOf<AWeapon>& ResultWeapon, TSubclassOf<AItem>& ResultItem);
	//각 박스의 랜덤 결과물을 획득
	bool GetRandomEquipmentWithGrade(APlayer_State* PS, EGrade Grade, TSubclassOf<AWeapon>& ResultWeapon, TSubclassOf<AItem>& ResultItem);

public:
	//카운트다운 시작/갱신
	void BeginCountdown();
	void UpdateCountdown();
	void SetAllPlayersWaitingStart();
	void SetAllPlayersCountdown(int32 number);
	void SetAllPlayersUI();

	//매치 내에서 플레이어들의 순위 반영
	void UpdatePlayersRank();

	//현재 레벨의 리스폰 포인트들 획득
	void GetRespawnPoints(bool bChooseOverRespawnPoint = false);
	//플레이어 리스폰 처리
	void EnableRespawn(APlayerController* PC);
	void TryRespawn(APlayerController* PC);
	void RequestRespawn(APlayerController* PC);
	void Respawn(AMatch_PlayerController* PC);
	//최대 리스폰 대기 시간 획득
	float GetRespawnTime() { return MaxRespawnWaitTime; }
	//RespawnPointOver 사용 설정
	void SetUseRespawnPointOver(bool bEnable);
	
	//각 플레이어 별 리스폰 관리 맵
	TMap<TObjectPtr<APlayerController>, FRespawnInfo> RespawnMap;

	//현재 레벨의 맵을 획득
	void SetNowMap();

	//리스폰 대기 중 관전 대상 설정
	AActor* GetSpectatorTarget(APlayer_Character* OutPlayer);

	//모든 플레이어 로딩 체크 및 매치 딜레이 카운트 설정
	void CheckAllPlayersLoadedAndStartDelay();
	//매치 딜레이 카운트 시작
	void StartDelay();
	//매치 플레이 시작
	void MatchStart();
	//모든 플레이어 게임 플레이 방지 (딜레이 중)
	void SetAllPlayersGameplayLocked(bool bLocked);
	//리스폰 데이터 획득
	TMap<TObjectPtr<APlayerController>, FRespawnInfo>& GetRespawnMap() { return RespawnMap; }

	//플레이어 상점 이용
	bool TryShoppingBox(class AMatch_PlayerController* PC, EShopBoxs Box);

	//등급별 무기, 아이템 리스트 획득
	const TArray<TSubclassOf<AWeapon>>& GetWeaponListByGrade(EGrade grade);
	const TArray<TSubclassOf<AItem>>& GetItemListByGrade(EGrade grade);

public:
	bool GetAlivePlayers(TArray<APlayer_Character*>& OutPlayers, APlayerController* OutPlayerPC);
	bool ChooseRespawnPoint(APlayerController* RespawnPC, FTransform& OutSpawnTransform);
	int32 AllocateSpawnSlot();
	bool IsRespawnPointChosed(int32 RespawnIndex, float CheckRadius = 150.f);

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	//호스트를 LV_Result로 이동시키기 위한 데이터 설정
	AMatch_PlayerController* HostPC = nullptr;
	int32 HostRank = 0;
	int32 HostCoin = 0;
	FName HostMapname;
};
