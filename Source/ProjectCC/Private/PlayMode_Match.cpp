// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayMode_Match.h"
#include "Player_Character.h"
#include "Player_State.h"
#include "Player_ControllerWidget.h"
#include "Coin.h"
#include "Weapon.h"
#include "Item.h"
#include "ItemDataAsset.h"
#include "Objects.h"
#include "WeaponListDataAsset.h"
#include "ItemListDataAsset.h"
#include "ObjectsListDataAsset.h"
#include "Match_State.h"
#include "AllPlayMode_SessionSubsystem.h"
#include "Match_PlayerController.h"
#include "Match_Event.h"
#include "Match_EventListDataAsset.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "MapConstructor.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

APlayMode_Match::APlayMode_Match()
{
	GameStateClass = AMatch_State::StaticClass();
	PlayerStateClass = APlayer_State::StaticClass();
	PlayerControllerClass = AMatch_PlayerController::StaticClass();

	CoinWaveTime = { 300, 200, 100 };
}

// Called when the game starts or when spawned
void APlayMode_Match::BeginPlay()
{
	Super::BeginPlay();
	GetRespawnPoints();

	if (UAllPlayMode_SessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UAllPlayMode_SessionSubsystem>()) {
		MaxPlayers = SessionSubsystem->GetTotalMatchPlayers();
	}

	SetNowMap();
	
	AMatch_State* _MatchState = GetGameState<AMatch_State>();
	if (_MatchState) {
		_MatchState->SetMatchStarted(false);
		_MatchState->SetMatchEnded(false);
		_MatchState->SetMatchTime(MatchDuration);
	}
}


AActor* APlayMode_Match::ChoosePlayerStart_Implementation(AController* Player)
{
	if (RespawnPoints.Num() <= 0) { 
		GetRespawnPoints();
	}
	if (RespawnPoints.Num() <= 0) {
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	APlayer_State* PS = Player ? Player->GetPlayerState<APlayer_State>() : nullptr;
	int32 SlotIndex = -1;

	if (PS) {
		SlotIndex = PS->GetSpawnSlotIndex();

		if (SlotIndex == -1) {
			SlotIndex = AllocateSpawnSlot();

			if (SlotIndex == -1) {
				SlotIndex = 0;
			}

			PS->SetSpawnSlotIndex(SlotIndex);
		}
	}

	if (RespawnPoints.IsValidIndex(SlotIndex) && RespawnPoints[SlotIndex]) {
		return RespawnPoints[SlotIndex];
	}

	if (RespawnPoints[0]) {
		return RespawnPoints[0];
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}


//매치 시작 시 기능
void APlayMode_Match::StartMatchLogic()
{
	AMatch_State* _MatchState = GetGameState<AMatch_State>();
	if (!_MatchState) return;

	bSpawnedCoins = false;
	CoinWaveTimeIndex = 0;

	MatchEventTimeIndex = 0;
	ActiveMatchEvents.Reset();
	UsedEvents.Reset();
	
	_MatchState->SetMatchTime(MatchDuration);
	_MatchState->SetMatchStarted(true);
	_MatchState->SetMatchEnded(false);

	UpdatePlayersRank();
	GetWorldTimerManager().SetTimer(MatchTimerHandle, this, &APlayMode_Match::UpdateMatchTimer, 1.0f, true);
	GetWorldTimerManager().SetTimer(SupplyTimerHandle, this, &APlayMode_Match::SupplyWeaponAndItem, SupplyCheckInterval, true);
	GetWorldTimerManager().SetTimer(SpawnObjectsTimerHandle, this, &APlayMode_Match::SupplyObjects, SpawnObjectsCheckInterval, true);
	
}
//매치 시작 시 모든 플레이어를 리스폰 지점에서 다시 스폰
void APlayMode_Match::SpawnAllPlayersAtRespawnPoint() {
	if (!HasAuthority()) return;

	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get());
		if (!PC) continue;

		APlayer_Character* Player = Cast<APlayer_Character>(PC->GetPawn());

		FTransform SpawnTransform;
		if (!ChooseRespawnPoint(PC, SpawnTransform)) continue;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = PC;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APlayer_Character* NewPlayer = GetWorld()->SpawnActor<APlayer_Character>(DefaultPawnClass, SpawnTransform, SpawnParams);
		if (!NewPlayer) continue;

		PC->Possess(NewPlayer);
		PC->bAutoManageActiveCameraTarget = true;
		PC->SetViewTarget(NewPlayer);
		PC->Client_ApplyGameInputMode();

		if (Player) {
			Player->Destroy();
		}

		NewPlayer->LoadNowItem();
	}
}

//매치 진행 Timer 관리
void APlayMode_Match::UpdateMatchTimer()
{
	AMatch_State* _MatchState = GetGameState<AMatch_State>();
	if (!_MatchState) return;
	if (!_MatchState->IsMatchStarted() || _MatchState->IsMatchEnded()) return;
	int32 NewMatchTime = _MatchState->GetMatchTime() - 1;
	_MatchState->SetMatchTime(NewMatchTime);

	CheckCoinWave(NewMatchTime);

	UpdateMatchEventNoticeUI(NewMatchTime);
	CheckMapEvent(NewMatchTime);

	UpdateActiveMatchEventUI();

	if (NewMatchTime <= 0) {
		EndMatchLogic();
	}
}
//매치 종료 시 기능
void APlayMode_Match::EndMatchLogic()
{
	AMatch_State* _MatchState = GetGameState<AMatch_State>();
	if (!_MatchState) return;
	
	StopMatchEvent();

	GetWorldTimerManager().ClearTimer(MatchTimerHandle);
	GetWorldTimerManager().ClearTimer(SupplyTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnObjectsTimerHandle);

	_MatchState->SetMatchTime(0);
	_MatchState->SetMatchStarted(false);
	_MatchState->SetMatchEnded(true);

	UpdatePlayersRank();
	//모든 플레이어의 최종 스코어 획득
	SendMatchResultsToPlayers();

	//플레이어 입력 정지, 카메라 정지, UI입력만 가능하게 변경
	for (FConstPlayerControllerIterator PCIT = GetWorld()->GetPlayerControllerIterator(); PCIT; ++PCIT) {
		if (AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(PCIT->Get())) {
			if (APlayer_Character* player = Cast<APlayer_Character>(PC->GetPawn())) {
				player->SetPlayerEndMatchState();
				player->AddInputBlockController(FName("MatchEnd"), true, true, true, false);
			}
			PC->Client_ApplyUIInputMode();
		}
	}

	GetWorldTimerManager().SetTimer(GoResultLevelTimerHandle, this, &APlayMode_Match::SendClientsToResultLevel, 3.f, false);
}

void APlayMode_Match::SendMatchResultsToPlayers()
{
	AGameStateBase* GS = GameState;
	if (!GS) return;

	TArray<FMatchResultData> AllResults;
	FName CurrentMapName = CurrentMap ? CurrentMap->GetFName() : NAME_None;
	
	for (APlayerState* PS_Base : GS->PlayerArray) {
		APlayer_State* PS = Cast<APlayer_State>(PS_Base);
		if (!PS) continue;

		FMatchResultData Data;
		Data.Nickname = *PS->GetNickName();
		Data.Coin = PS->GetPlayerCoin();
		Data.Rank = PS->GetPlayerRank();
		Data.Eliminate = PS->GetPlayerEliminate();
		Data.Out = PS->GetPlayerOut();
		Data.MapName = CurrentMapName;

		AllResults.Add(Data);
	}

	AllResults.Sort([](const FMatchResultData& A, const FMatchResultData& B) {
		return A.Rank < B.Rank;
	});

	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		AMatch_PlayerController* MPC = Cast<AMatch_PlayerController>(IT->Get());
		if (!MPC) return;

		APlayer_State* OwnPS = MPC->GetPlayerState<APlayer_State>();
		if (!OwnPS) continue;

		FMatchResultData OwnData;
		OwnData.Nickname = *OwnPS->GetNickName();
		OwnData.Rank = OwnPS->GetPlayerRank();
		OwnData.Coin = OwnPS->GetPlayerCoin();
		OwnData.Eliminate = OwnPS->GetPlayerEliminate();
		OwnData.Out = OwnPS->GetPlayerOut();
		OwnData.MapName = CurrentMapName;

		MPC->Client_SaveResultData(OwnData, AllResults);
	}
}

void APlayMode_Match::SendClientsToResultLevel()
{
	if (!HasAuthority()) return;

	HostPC = nullptr;
	HostRank = -1;
	HostCoin = -1;

	FString LevelNameString = UGameplayStatics::GetCurrentLevelName(this, true);
	HostMapname = FName(*LevelNameString);

	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; IT++) {
		AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get());
		if (!PC) continue;

		APlayer_State* PS = PC->GetPlayerState<APlayer_State>();
		if (!PS) continue;

		if (PC->IsLocalController()) {
			HostPC = PC;
			HostRank = PS->GetPlayerRank();
			HostCoin = PS->GetPlayerCoin();
			continue;
		}

		PC->Client_EndMatch();
	}

	GetWorldTimerManager().SetTimer(HostGoResultLevelTimerHandle, this, &APlayMode_Match::SendHostToResultLevel, 0.7f, false);
}

void APlayMode_Match::SendHostToResultLevel()
{
	if (!HasAuthority()) return;
	if (!HostPC) return;

	HostPC->EndMatch();
}

EGrade APlayMode_Match::GetRandomGrade()
{
	int32 Random = FMath::RandRange(1, 100);

	if (Random <= 50) {
		return EGrade::Grade_B;
	}
	else if (Random <= 75 && Random > 50) {
		return EGrade::Grade_A;
	}
	else if (Random <= 100 && Random > 75) {
		return EGrade::Grade_S;
	}

	return EGrade::Grade_B;
}

const TArray<TSubclassOf<AWeapon>>& APlayMode_Match::GetWeaponListByGrade(EGrade grade)
{
	static const TArray<TSubclassOf<AWeapon>> EmptyList;

	if (!AllWeaponList) {
		return EmptyList;
	}

	switch (grade) {
	case EGrade::Grade_B: return AllWeaponList->BWeapons;
	case EGrade::Grade_A: return AllWeaponList->AWeapons;
	case EGrade::Grade_S: return AllWeaponList->SWeapons;
	}

	return EmptyList;
}

const TArray<TSubclassOf<AItem>>& APlayMode_Match::GetItemListByGrade(EGrade grade)
{
	static const TArray<TSubclassOf<AItem>> EmptyList;

	if (!AllItemList) {
		return EmptyList;
	}

	switch (grade) {
	case EGrade::Grade_B: return AllItemList->BItems;
	case EGrade::Grade_A: return AllItemList->AItems;
	case EGrade::Grade_S: return AllItemList->SItems;
	}

	return EmptyList;
}

int32 APlayMode_Match::GetShopPrice(EShopBoxs Box)
{
	switch (Box) {
	case EShopBoxs::B_Box:		return 10;
	case EShopBoxs::A_Box:		return 30;
	case EShopBoxs::S_Box:		return 50;
	case EShopBoxs::Random_Box: return 40;
	default:					return 999;
	}
}

bool APlayMode_Match::GetRandomRewardInShopBox(APlayer_State* PS, EShopBoxs Box, TSubclassOf<AWeapon>& ResultWeapon, TSubclassOf<AItem>& ResultItem)
{
	ResultWeapon = nullptr;
	ResultItem = nullptr;

	if (!PS) return false;

	switch (Box) {
	case EShopBoxs::B_Box:
		return GetRandomEquipmentWithGrade(PS, EGrade::Grade_B, ResultWeapon, ResultItem);
	case EShopBoxs::A_Box:
		return GetRandomEquipmentWithGrade(PS, EGrade::Grade_A, ResultWeapon, ResultItem);
	case EShopBoxs::S_Box:
		return GetRandomEquipmentWithGrade(PS, EGrade::Grade_S, ResultWeapon, ResultItem);
	case EShopBoxs::Random_Box: 
		{
			int32 Random = FMath::RandRange(1, 100);
			if (Random <= 50) {
				return GetRandomEquipmentWithGrade(PS, EGrade::Grade_B, ResultWeapon, ResultItem);
			}
			else if (Random > 50 && Random <= 80) {
				return GetRandomEquipmentWithGrade(PS, EGrade::Grade_A, ResultWeapon, ResultItem);
			}
			else if (Random > 80 && Random <= 90) {
				return GetRandomEquipmentWithGrade(PS, EGrade::Grade_S, ResultWeapon, ResultItem);
			}
			else {
				return true;
			}
		}
	}

	return true;
}

bool APlayMode_Match::GetRandomEquipmentWithGrade(APlayer_State* PS, EGrade Grade , TSubclassOf<AWeapon>& ResultWeapon, TSubclassOf<AItem>& ResultItem)
{
	ResultWeapon = nullptr;
	ResultItem = nullptr;

	if (!PS) return false;

	const TArray<TSubclassOf<AWeapon>>& WeaponList = GetWeaponListByGrade(Grade);
	const TArray<TSubclassOf<AItem>>& ItemList = GetItemListByGrade(Grade);

	bool bAlreadyHasItem = PS->CheckEquippedItem();

	struct FRewardCandidate {
		bool bIsWeapon = false;
		TSubclassOf<AWeapon> Weapon;
		TSubclassOf<AItem> Item;
	};

	TArray<FRewardCandidate> Candidates;

	for (const TSubclassOf<AWeapon>& weapon : WeaponList) {
		if (!weapon) continue;

		FRewardCandidate WeaponData;
		WeaponData.bIsWeapon = true;
		WeaponData.Weapon = weapon;
		Candidates.Add(WeaponData);
	}

	//아이템은 현재 장착 대기중인 아이템이 있다면 Reward 목록에서 제외
	if (!bAlreadyHasItem) {
		for (const TSubclassOf<AItem>& item : ItemList) {
			if (!item) continue;

			FRewardCandidate ItemData;
			ItemData.bIsWeapon = false;
			ItemData.Item = item;
			Candidates.Add(ItemData);
		}
	}

	if (Candidates.Num() <= 0) {
		return false;
	}

	int32 Index = FMath::RandRange(0, Candidates.Num() - 1);
	FRewardCandidate& Result = Candidates[Index];

	if (Result.bIsWeapon) {
		ResultWeapon = Result.Weapon;
	}

	else {
		ResultItem = Result.Item;
	}

	return true;
}

//카운트다운 시작
void APlayMode_Match::BeginCountdown()
{
	GetWorldTimerManager().ClearTimer(MatchDelayTimerHandle);
	CurrentCountdownNumber = 6;

	SetAllPlayersCountdown(CurrentCountdownNumber);

	GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &APlayMode_Match::UpdateCountdown, 1.f, true);
}
//카운트다운 갱신
void APlayMode_Match::UpdateCountdown()
{
	CurrentCountdownNumber--;

	if (CurrentCountdownNumber > 0) {
		SetAllPlayersCountdown(CurrentCountdownNumber);
		return;
	}

	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	MatchStart();
}

void APlayMode_Match::SetAllPlayersWaitingStart()
{
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		if (AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get())) {
			PC->Client_SetPreMatchDelay();
		}
	}
}

void APlayMode_Match::SetAllPlayersCountdown(int32 number)
{
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		if (AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get())) {
			PC->Client_UpdateCountDown(number);
		}
	}
}

void APlayMode_Match::SetAllPlayersUI()
{
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		if (AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get())) {
			PC->Client_StartPlayingUI();
		}
	}
}
//Portrait Id 바인딩
void APlayMode_Match::AssignPortraitId()
{
	if (!GameState) return;

	for (int32 i = 0; i < GameState->PlayerArray.Num(); ++i) {
		APlayer_State* PS = Cast<APlayer_State>(GameState->PlayerArray[i]);
		if (!PS) continue;
		PS->SetPortraitId(i);
	}
}

//플레이어들 순위를 실시간 반영
void APlayMode_Match::UpdatePlayersRank()
{
	TArray<APlayer_State*> Players;
	AMatch_State* MS = GetGameState<AMatch_State>();
	if (!MS) return;
	for (TObjectPtr<APlayerState> PS : MS->PlayerArray) {
		if (APlayer_State* MyPS = Cast<APlayer_State>(PS)) {
			Players.Add(MyPS);
		}
	}
	//동률 처리 우선순위 : 1. 코인 > 2. 탈락 시킨 횟수 - 탈락한 횟수  > 3. 탈락 시킨 횟수 > 4. 탈락한 횟수
	Players.Sort([](const APlayer_State& APlayer,const APlayer_State& BPlayer) {
		if (APlayer.GetPlayerCoin() != BPlayer.GetPlayerCoin()) {
			return APlayer.GetPlayerCoin() > BPlayer.GetPlayerCoin();
		}
		int32 APlayer_Score = APlayer.GetPlayerEliminate() - APlayer.GetPlayerOut();
		int32 BPlayer_Score = BPlayer.GetPlayerEliminate() - BPlayer.GetPlayerOut();
		if (APlayer_Score != BPlayer_Score) {
			return APlayer_Score > BPlayer_Score;
		}
		if (APlayer.GetPlayerEliminate() != BPlayer.GetPlayerEliminate()) {
			return APlayer.GetPlayerEliminate() > BPlayer.GetPlayerEliminate();
		}
		if (APlayer.GetPlayerOut() != BPlayer.GetPlayerOut()) {
			return APlayer.GetPlayerOut() < BPlayer.GetPlayerOut();
		}

		return false;
	});

	int32 CurrentRank = 0;

	for (int32 i = 0; i < Players.Num(); i++) {
		if (i == 0) CurrentRank = 1;
		else {
			APlayer_State* PrePlayer = Players[i - 1];
			APlayer_State* NowPlayer = Players[i];
			int32 PrePlayerScore = PrePlayer->GetPlayerEliminate() - PrePlayer->GetPlayerOut();
			int32 NowPlayerScore = NowPlayer->GetPlayerEliminate() - NowPlayer->GetPlayerOut();
			//코인 수, 아웃시킨 횟수 - 아웃한 횟수 ,아웃시킨 횟수, 아웃한 횟수가 모두 같은지 확인
			bool bSameRank = (
				PrePlayer->GetPlayerCoin() == NowPlayer->GetPlayerCoin() && 
				PrePlayerScore == NowPlayerScore &&
				PrePlayer->GetPlayerEliminate() == NowPlayer->GetPlayerEliminate() && 
				PrePlayer->GetPlayerOut() == NowPlayer->GetPlayerOut()
			);

			if (!bSameRank) CurrentRank = i + 1;
		}
		Players[i]->SetPlayerRank(CurrentRank);
	}
}

//탈락한 플레이어의 관전 대상 획득 (1순위 탈락시킨 플레이어, 2순위 랜덤 플레이어, 3순위 고정 카메라)
AActor* APlayMode_Match::GetSpectatorTarget(APlayer_Character* OutPlayer) {
	if (!OutPlayer) return nullptr;

	//1순위 : 탈락 시킨 플레이어 카메라를 대상
	APlayer_Character* Winner = OutPlayer->WinnerPlayer;
	if (Winner && !Winner->IsOut()) {
		return Winner;
	}

	//2순위 : 랜덤한 플레이어 카메라를 대상
	TArray<APlayer_Character*> NotOutPlayers;
	//탈락하지 않은 플레이어 확인
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		APlayerController* PC = IT->Get();
		if (!PC) continue;

		APlayer_Character* Player = Cast<APlayer_Character>(PC->GetPawn());
		if (!Player || Player==OutPlayer || Player->IsOut()) continue;
		NotOutPlayers.Add(Player);
	}
	//탈락하지 않은 플레이어들 중 랜덤한 한 명 선정
	if (NotOutPlayers.Num() > 0) {
		const int32 index = FMath::RandRange(0, NotOutPlayers.Num() - 1);
		return NotOutPlayers[index];
	}

	//3순위 : 맵에 배치된 기본 카메라 (Client에서 찾아서 지정 / 여기서는 nullptr)
	return nullptr;
}
//모든 플레이어 로딩 체크 및 매치 딜레이 카운트 설정
void APlayMode_Match::CheckAllPlayersLoadedAndStartDelay()
{
	if (!HasAuthority()) return;
	

	AMatch_State* _MatchState = GetGameState<AMatch_State>();
	if (!_MatchState || !GameState) return;

	if (GetWorldTimerManager().IsTimerActive(MatchDelayTimerHandle)) return;

	if (MaxPlayers <= 0) { 
		return; 
	}

	if (GameState->PlayerArray.Num() < MaxPlayers) { 
		return; 
	}

	for (APlayerState* PSBase : GameState->PlayerArray) {
		APlayer_State* PS = Cast<APlayer_State>(PSBase);
		if (!PS || !PS->IsMatchLevelLoaded()) return;
	}

	AssignPortraitId();

	SetAllPlayersGameplayLocked(true);

	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get());
		if (!PC) continue;

		APlayer_Character* Player = Cast<APlayer_Character>(PC->GetPawn());
		if (!Player) continue;

		PC->Client_ApplyGameInputMode();
	}

	StartDelay();
}
//매치 딜레이 카운트 시작
void APlayMode_Match::StartDelay()
{
	AMatch_State* _MatchState = GetGameState<AMatch_State>();
	if (!_MatchState) return;

	float ServerNow = _MatchState->GetServerWorldTimeSeconds();
	_MatchState->SetDelayEndServerTime(ServerNow + 10.f);
	//시작 전 입력 막기, UI 제거
	SetAllPlayersGameplayLocked(true);
	SetAllPlayersWaitingStart();

	float WaitingTime = 4.f;

	GetWorldTimerManager().SetTimer(MatchDelayTimerHandle, this, &APlayMode_Match::BeginCountdown, 4.f, false);
}

//매치 플레이 시작
void APlayMode_Match::MatchStart()
{
	//서버가 된 Host의 경우 Session 상태를 검색 불가 상태로 변경
	if (HasAuthority()) {
		if (UAllPlayMode_SessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UAllPlayMode_SessionSubsystem>()) {
			SessionSubsystem->MarkSessionInGame();
		}
	}

	GetWorldTimerManager().ClearTimer(MatchDelayTimerHandle);
	SetAllPlayersGameplayLocked(false);

	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		if (AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get())) {
			PC->Client_ApplyGameInputMode();
		}
	}

	SetAllPlayersUI();
	StartMatchLogic();
}
//모든 플레이어 게임 플레이 방지 (딜레이 중)
void APlayMode_Match::SetAllPlayersGameplayLocked(bool bLocked)
{
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		APlayerController* PC = IT->Get();
		if (!PC) continue;

		APlayer_Character* Player = Cast<APlayer_Character>(PC->GetPawn());
		if (!Player) continue;

		if (bLocked) {
			Player->AddInputBlockController(FName("MatchReady"), true, false, true, false);
		}
		else {
			Player->RemoveInputBlockController(FName("MatchReady"));
		}
		
	}
}

bool APlayMode_Match::TryShoppingBox(AMatch_PlayerController* PC, EShopBoxs Box)
{
	if (!HasAuthority()) return false;
	if (!PC) return false;
	//리스폰 대기중인 플레이어가 아닌 경우 상점 이용 불가
	if (!RespawnMap.Contains(PC)) return false;

	APlayer_State* PS = PC->GetPlayerState<APlayer_State>();
	if (!PS) return false;

	//가진 돈이 부족하면 구매 실패
	int32 Price = GetShopPrice(Box);
	if (PS->GetPlayerCoin() < Price) return false;

	TSubclassOf<AWeapon> RewardWeapon = nullptr;
	TSubclassOf<AItem> RewardItem = nullptr;

	if (!GetRandomRewardInShopBox(PS, Box, RewardWeapon, RewardItem)) {
		return false;
	}

	//가격만큼 코인 차감
	PS->AddCoin(-Price);

	//선정된 무기/아이템을 대기 목록에 장착
	if (RewardItem) {
		AItem* Item = RewardItem->GetDefaultObject<AItem>();
		int32 UseCount = Item ? Item->ItemData->MaxUseCount : 1;
		PS->SetEquippedItem(RewardItem, UseCount);
	}

	if (RewardWeapon) {
		PS->SetReservedWeapon(RewardWeapon);
	}

	//코인이 변경된 만큼 순위 변경
	UpdatePlayersRank();
	return true;
}

//현재 아웃되지 않은 플레이어들 탐색
bool APlayMode_Match::GetAlivePlayers(TArray<APlayer_Character*>& OutPlayers, APlayerController* OutPlayerPC)
{
	OutPlayers.Reset();

	UWorld* World = GetWorld();
	if (!World) return false;

	for (FConstPlayerControllerIterator PIT = World->GetPlayerControllerIterator(); PIT; ++PIT) {
		APlayerController* PC = PIT->Get();
		if (!PC) continue;
		if (PC == OutPlayerPC) continue;

		APlayer_Character* PlayerCharacter = Cast<APlayer_Character>(PC->GetPawn());
		if (!PlayerCharacter) continue;
		if (PlayerCharacter->IsOut()) continue;

		OutPlayers.Add(PlayerCharacter);
	}
	return OutPlayers.Num() > 0;
}

//리스폰 지점 선정
bool APlayMode_Match::ChooseRespawnPoint(APlayerController* RespawnPC, FTransform& OutSpawnTransform)
{
	if (RespawnPoints.Num() <= 0) return false;

	TArray<APlayer_Character*> AlivePlayers;
	GetAlivePlayers(AlivePlayers, RespawnPC);

	//현재 아웃되지 않은 플레이어가 없을 경우 그냥 랜덤 리스폰
	if (AlivePlayers.Num() <= 0) {
		int32 RandomIndex = FMath::RandRange(0, RespawnPoints.Num() - 1);
		ATargetPoint* RandomPoint = RespawnPoints[RandomIndex];
		if (!RandomPoint) return false;

		OutSpawnTransform = RandomPoint->GetActorTransform();
		return true;
	}

	TArray<FRespawnPoint> Candidates;
	FRespawnPoint FallBackRespawnPoint;
	float NonRespawnRadiusSq = FMath::Square(NonRespawnRadius);

	for (ATargetPoint* point : RespawnPoints) {
		if (!point) continue;
		
		FVector SpawnLocation = point->GetActorLocation();
		float NearestPlayerDistanceSq = -1.f;
		float FallestPlayerDistanceSq = -1.f;
		bool bNonSpawn = false;
		bool bFallbackSpawnPoint = false;
		//현재 아웃되지 않은 플레이어들의 리스폰 지점과의 거리와 근처에 플레이어 캐릭터가 있으면 리스폰이 되지 않는 거리 비교
		for (APlayer_Character* OtherPlayer : AlivePlayers) {
			if (!OtherPlayer) continue;
			float DistanceSq = FVector::DistSquared(SpawnLocation, OtherPlayer->GetActorLocation());

			if (DistanceSq < NonRespawnRadiusSq && NonRespawnRadiusSq > 0.f) {
				bNonSpawn = true;
				break;
			}
			if (DistanceSq > FallestPlayerDistanceSq) {
				FallestPlayerDistanceSq = DistanceSq;
				bFallbackSpawnPoint = true;
			}
			if (NearestPlayerDistanceSq < 0.f) {
				NearestPlayerDistanceSq = DistanceSq;
			}
			else {
				NearestPlayerDistanceSq = FMath::Min(NearestPlayerDistanceSq, DistanceSq);
			}
		}
		
		if (bFallbackSpawnPoint) {
			FallBackRespawnPoint.Point = point;
			FallBackRespawnPoint.NearestPlayerDistanceSq = FallestPlayerDistanceSq;
		}
		if (bNonSpawn) continue;
	
		FRespawnPoint TargetRespawnPoint;
		TargetRespawnPoint.Point = point;
		TargetRespawnPoint.NearestPlayerDistanceSq = NearestPlayerDistanceSq;
		Candidates.Add(TargetRespawnPoint);
	}

	//리스폰 최소 반경 조건을 만족하는 리스폰 지점이 없을 경우 FallBack
	//가장 멀리 떨어진 Respawn Point로 선정
	if (Candidates.Num() <= 0) {
		if (FallBackRespawnPoint.NearestPlayerDistanceSq == -1.f) {
			UE_LOG(LogTemp, Error, TEXT("No Fallback RespawnPoint"));
			return false;
		}
		OutSpawnTransform = FallBackRespawnPoint.Point->GetActorTransform();
		return true;
	}

	Candidates.Sort([](const FRespawnPoint& APoint, const FRespawnPoint& BPoint) {
		return APoint.NearestPlayerDistanceSq < BPoint.NearestPlayerDistanceSq;
	});

	int32 PickCount = FMath::Min(3, Candidates.Num());
	int32 RandomIndex = FMath::RandRange(0, PickCount - 1);

	if (!Candidates[RandomIndex].Point) return false;

	OutSpawnTransform = Candidates[RandomIndex].Point->GetActorTransform();
	return true;
}
//각 플레이어들의 시작 Spawn 포인트 할당 
int32 APlayMode_Match::AllocateSpawnSlot()
{
	if (RespawnPoints.Num() <= 0) return -1;

	TSet<int32> UsedSlots;

	if (GameState) {
		for (APlayerState* PSBase : GameState->PlayerArray) {
			APlayer_State* PS = Cast<APlayer_State>(PSBase);
			if (!PS) continue;

			if (PS->GetSpawnSlotIndex() != -1) {
				UsedSlots.Add(PS->GetSpawnSlotIndex());
			}
		}
	}

	TArray<int32> NonChosedSlots;
	for (int32 i = 0; i < RespawnPoints.Num(); ++i) {
		if (!UsedSlots.Contains(i)) {
			NonChosedSlots.Add(i);
		}
	}

	if (NonChosedSlots.Num() <= 0) return -1;
	
	int32 RandomArrayIndex = FMath::RandRange(0, NonChosedSlots.Num() - 1);
	return NonChosedSlots[RandomArrayIndex];
}
//Spawn 포인트가 할당 되었는지 확인
bool APlayMode_Match::IsRespawnPointChosed(int32 RespawnIndex, float CheckRadius)
{
	if (!RespawnPoints.IsValidIndex(RespawnIndex)) return true;

	FVector SpawnLocation = RespawnPoints[RespawnIndex]->GetActorLocation();
	float CheckRadiusSq = CheckRadius * CheckRadius;

	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		APlayerController* PC = IT->Get();
		if (!PC) continue;
		APawn* Pawn = PC->GetPawn();
		if (!Pawn) continue;

		if (FVector::DistSquared(Pawn->GetActorLocation(), SpawnLocation) <= CheckRadiusSq) return true;
	}
	return false;
}

//현재 레벨에 있는 리스폰 지점 획득
void APlayMode_Match::GetRespawnPoints(bool bChooseOverRespawnPoint)
{
	RespawnPoints.Reset();

	TArray<AActor*> FoundPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetPoint::StaticClass(), FoundPoints);

	for (AActor* Actor : FoundPoints) {
		ATargetPoint* Point = Cast<ATargetPoint>(Actor);
		if (!Point) continue;

		if (!bChooseOverRespawnPoint) {
			if (Point->ActorHasTag(TEXT("RespawnPoint"))) {
				RespawnPoints.Add(Point);
			}
		}
		else {
			if (Point->ActorHasTag(TEXT("RespawnPointOver"))) {
				RespawnPoints.Add(Point);
			}
		}
	}
}

//리스폰 가능 상태로 변경(최소 리스폰 대기 시간 충족)
void APlayMode_Match::EnableRespawn(APlayerController* PC)
{
	if (!HasAuthority()) return;
	if (!PC) return;

	FRespawnInfo* RespawnInfo = RespawnMap.Find(PC);
	if (!RespawnInfo) return;

	RespawnInfo->bCanRespawn = true;

	AMatch_PlayerController* Match_PC = Cast<AMatch_PlayerController>(PC);
	if (!Match_PC)return;

	Match_PC->Client_SetRespawnState(true, true);
}

//최소 리스폰 대기 시간 후 리스폰 요청(플레이어 입력) 
void APlayMode_Match::TryRespawn(APlayerController* PC)
{
	if (!HasAuthority()) return;
	if (!PC) return;

	FRespawnInfo* RespawnInfo = RespawnMap.Find(PC);
	if (!RespawnInfo) return;
	if (!RespawnInfo->bCanRespawn) return;

	GetWorldTimerManager().ClearTimer(RespawnInfo->ActiveRespawnTimerHandle);
	GetWorldTimerManager().ClearTimer(RespawnInfo->AutoRespawnTimerHandle);

	AMatch_PlayerController* Match_PC = Cast<AMatch_PlayerController>(PC);
	if (!Match_PC) return;
	
	Respawn(Match_PC);
}

//탈락한 플레이어 리스폰 요청
void APlayMode_Match::RequestRespawn(APlayerController* PC) {
	if (!HasAuthority()) return;
	if (!PC) return;

	if (RespawnMap.Contains(PC)) return;

	FRespawnInfo& RespawnInfo = RespawnMap.Add(PC);
	RespawnInfo.bCanRespawn = false;

	AMatch_PlayerController* Match_PC = Cast<AMatch_PlayerController>(PC);
	if (!Match_PC) {
		return;
	}

	Match_PC->Client_SetRespawnState(true, false);

	//MinTime 뒤 수동 리스폰 활성화
	GetWorldTimerManager().SetTimer(RespawnInfo.ActiveRespawnTimerHandle, FTimerDelegate::CreateUObject(this, &APlayMode_Match::EnableRespawn, PC), RespawnTime, false);

	//MaxTime 뒤 자동 리스폰
	GetWorldTimerManager().SetTimer(RespawnInfo.AutoRespawnTimerHandle, FTimerDelegate::CreateUObject(this, &APlayMode_Match::Respawn, Match_PC), MaxRespawnWaitTime, false);
}

//플레이어 리스폰
void APlayMode_Match::Respawn(AMatch_PlayerController* PC) {
	if (!HasAuthority()) return;
	if (!PC) return;

	FRespawnInfo* RespawnInfo = RespawnMap.Find(PC);
	if (RespawnInfo) {
		GetWorldTimerManager().ClearTimer(RespawnInfo->ActiveRespawnTimerHandle);
		GetWorldTimerManager().ClearTimer(RespawnInfo->AutoRespawnTimerHandle);
		RespawnMap.Remove(PC);
	}

	FTransform SpawnTransform;
	//리스폰 위치 획득(없으면 큰일;;)
	if (!ChooseRespawnPoint(PC, SpawnTransform)) {
		UE_LOG(LogTemp, Error, TEXT("No Respawn spot"));
		return;
	}
	//Spawn한 Character의 소유자를 플레이어로 지정
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = PC;
	//리스폰 지점에 겹칠 시 위치 보정
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APlayer_Character* SpawnPlayer = GetWorld()->SpawnActor<APlayer_Character>(DefaultPawnClass, SpawnTransform, SpawnParams);
	
	//없으면 큰일;;
	if (!SpawnPlayer) {
		UE_LOG(LogTemp, Error, TEXT("Fail to Respawn player"));
		return;
	}

	PC->Possess(SpawnPlayer);
	PC->bAutoManageActiveCameraTarget = true;
	PC->SetViewTarget(SpawnPlayer);

	PC->Client_StartPlayingUI();
	PC->Client_SetRespawnState(false, false);

	SpawnPlayer->LoadNowItem();
	SpawnPlayer->ApplyResevedWeapon();

	ApplyActiveEventToPlayer(SpawnPlayer);

	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		AMatch_PlayerController* OtherPC = Cast<AMatch_PlayerController>(IT->Get());
		if (!OtherPC) continue;
		if (OtherPC == PC) continue;

		//리스폰 대기중인 플레이어만 적용
		if (!RespawnMap.Contains(OtherPC)) continue;
		//기본 카메라를 보고있는 플레이어만 적용
		if (!OtherPC->IsSpectatingDefaultCamera()) continue;

		OtherPC->SetSpectatingDefaultCamera(false);
		OtherPC->Client_StartSpectatingPlayer(SpawnPlayer);
	}
}

//리스폰 지점을 RespawnPointOver를 사용하도록 설정
void APlayMode_Match::SetUseRespawnPointOver(bool bEnable)
{
	if (!HasAuthority()) return;

	bUseOverRespawnPoints = bEnable;

	GetRespawnPoints(bUseOverRespawnPoints);

	if (bUseOverRespawnPoints && RespawnPoints.Num() <= 0) {
		bUseOverRespawnPoints = false;
		GetRespawnPoints(false);
	}
}

//현재 레벨의 맵을 획득
void APlayMode_Match::SetNowMap()
{
	CurrentMap = nullptr;
	AActor* Foundmap = UGameplayStatics::GetActorOfClass(GetWorld(), AMapConstructor::StaticClass());
	if (!Foundmap) {
		UE_LOG(LogTemp, Warning, TEXT("Failed to Map Loading"));
		return;
	}
	CurrentMap = Cast<AMapConstructor>(Foundmap);
	if (!CurrentMap) {
		UE_LOG(LogTemp, Warning, TEXT("Failed to Map Casting"));
		return;
	}
}

/*-------------Match Event-------------------*/
//매치 이벤트 시간 체크
void APlayMode_Match::CheckMapEvent(int32 RemainingTime)
{
	if (!HasAuthority()) return;
	if (MatchEventTimes.Num() <= 0) return;
	if (MatchEventTimeIndex >= MatchEventTimes.Num()) return;

	int32 TargetTime = MatchEventTimes[MatchEventTimeIndex];

	if (RemainingTime > TargetTime) return;

	StartMatchEvent();
	MatchEventTimeIndex += 1;
}

//리스트에서 랜덤한 매치 이벤트 획득
bool APlayMode_Match::SelectRandomMatchEvent(FMatchEventData& EventData)
{
	EventData = FMatchEventData();
	TArray<FMatchEventData> Candidates;

	if (Match_CommonEventList) {
		for (const FMatchEventData& Eventdata : Match_CommonEventList->Events) {
			if (!Eventdata.Event) continue;
			//현재 매치에 이미 발동했던 Event는 제외
			if (UsedEvents.Contains(Eventdata.Event)) continue;
			Candidates.Add(Eventdata);
		}
	}
	
	if (CurrentMap && CurrentMap->Match_MapEventList) {
		for (const FMatchEventData& Eventdata : CurrentMap->Match_MapEventList->Events) {
			if (!Eventdata.Event) continue;
			//현재 매치에 이미 발동했던 Event는 제외
			if (UsedEvents.Contains(Eventdata.Event)) continue;
			Candidates.Add(Eventdata);
		}
	}

	if (Candidates.Num() <= 0) return false;

	int32 Index = FMath::RandRange(0, Candidates.Num() - 1);
	EventData = Candidates[Index];

	return true;
}

void APlayMode_Match::StartMatchEvent()
{
	if (!HasAuthority()) return;
	if (!CurrentMap) return;
	//현재 남아있는 맵 이벤트가 있다면 즉시 종료
	StopMatchEvent();

	FMatchEventData SelectedEventData;
	if (bPendingMatchEvent) {
		SelectedEventData = PendingEventData;
	}
	else {
		if (!SelectRandomMatchEvent(SelectedEventData)) return;
	}

	bPendingMatchEvent = false;
	PendingEventData = FMatchEventData();

	if (!SelectedEventData.Event) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AMatch_Event* NewEvent = GetWorld()->SpawnActor<AMatch_Event>(SelectedEventData.Event, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (!NewEvent) return;

	ActiveMatchEvents.Add(NewEvent);
	UsedEvents.Add(SelectedEventData.Event);

	CurrentEventName = SelectedEventData.EventName;
	CurrentEventTime = GetWorld()->GetTimeSeconds() + SelectedEventData.EventDuration;
	bActiveMatchEvent = true;

	NewEvent->StartEvent(CurrentMap, this, SelectedEventData.EventDuration);

	int32 RemainingSeconds = FMath::Max(0, FMath::CeilToInt(SelectedEventData.EventDuration));
	BroadcastMatchEventActive(CurrentEventName, RemainingSeconds);
}

void APlayMode_Match::StopMatchEvent()
{
	for (AMatch_Event* Event : ActiveMatchEvents) {
		if (IsValid(Event)) {
			Event->StopEvent();
		}
	}

	ActiveMatchEvents.Reset();

	CurrentEventName = NAME_None;
	CurrentEventTime = -1.f;
	bActiveMatchEvent = false;

	BroadcastMatchEventEnded();
}
void APlayMode_Match::ApplyActiveEventToPlayer(APlayer_Character* Player)
{
	if (!HasAuthority()) return;
	if (!Player) return;

	for (AMatch_Event* Event : ActiveMatchEvents) {
		if (!IsValid(Event)) continue;
		if (!Event->IsEventRunning()) continue;

		Event->ApplyEventToPlayer(Player);
	}
}
void APlayMode_Match::UpdateMatchEventNoticeUI(int32 RemainingTime)
{
	if (!HasAuthority()) return;
	if (MatchEventTimes.Num() <= 0) return;
	if (MatchEventTimeIndex >= MatchEventTimes.Num()) return;

	int32 TargetTime = MatchEventTimes[MatchEventTimeIndex];
	int32 SecondsUntilEvent = RemainingTime - TargetTime;

	if (SecondsUntilEvent > MatchEventNoticeSeconds || SecondsUntilEvent <= 0) {
		return;
	}

	if (!bPendingMatchEvent) {
		if (!PrepareNextMatchEvent()) return;
	}

	BroadcastMatchEventCountdown(PendingEventData.EventName, SecondsUntilEvent);
}

void APlayMode_Match::UpdateActiveMatchEventUI()
{
	if (!HasAuthority()) return;
	if (!bActiveMatchEvent) return;
	if (CurrentEventName.IsNone()) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float RemainTime = CurrentEventTime - CurrentTime;

	if (RemainTime <= 0.f) {
		StopMatchEvent();
		return;
	}

	int32 RemainSeconds = FMath::Max(0, FMath::CeilToInt(RemainTime));

	BroadcastMatchEventActive(CurrentEventName, RemainSeconds);
}

void APlayMode_Match::NotifyMatchEventFinished(AMatch_Event* FinishedEvent)
{
	if (!HasAuthority()) return;

	ActiveMatchEvents.Remove(FinishedEvent);

	if (ActiveMatchEvents.Num() <= 0) {
		CurrentEventName = NAME_None;
		CurrentEventTime = -1.f;
		bActiveMatchEvent = false;

		BroadcastMatchEventEnded();
	}
}
bool APlayMode_Match::PrepareNextMatchEvent()
{
	if (bPendingMatchEvent) return true;

	FMatchEventData SelectedEventData;
	if (!SelectRandomMatchEvent(SelectedEventData)) return false;

	PendingEventData = SelectedEventData;
	bPendingMatchEvent = true;

	return true;
}
void APlayMode_Match::BroadcastMatchEventCountdown(FName EventName, int32 SecondsUntilEvent)
{
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get());
		if (!PC) continue;

		PC->Client_UpdateMatchEventCountdown(EventName, SecondsUntilEvent);
	}
}
void APlayMode_Match::BroadcastMatchEventActive(FName EventName, int32 RemainingSeconds)
{
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get());
		if (!PC) continue;

		PC->Client_ShowMatchEventActive(EventName, RemainingSeconds);
	}
}
void APlayMode_Match::BroadcastMatchEventEnded()
{
	for (FConstPlayerControllerIterator IT = GetWorld()->GetPlayerControllerIterator(); IT; ++IT) {
		AMatch_PlayerController* PC = Cast<AMatch_PlayerController>(IT->Get());
		if (!PC) continue;

		PC->Client_HideMatchEventUI();
	}
}

/*--------------Coin Wave----------------*/
//CoinWave 시간 체크
void APlayMode_Match::CheckCoinWave(int32 RemainingTime)
{
	if (bSpawnedCoins) return;
	if (CoinWaveTime.Num() <= 0)return;
	if (CoinWaveTimeIndex >= CoinWaveTime.Num()) return;
	if (RemainingTime > CoinWaveTime[CoinWaveTimeIndex]) return;

	CoinWave();
	CoinWaveTimeIndex += 1;

	if (CoinWaveTimeIndex >= CoinWaveTime.Num()) {
		bSpawnedCoins = true;
	}
}
//CoinWave 기능
void APlayMode_Match::CoinWave()
{
	if (bSpawnedCoins) return;
	if (!HasAuthority()) return;
	if (!GetWorld()) return;
	if (!CurrentMap) return;
	if (!Coin)return;

	TArray<FIntVector> CandidateLocation;

	if (CurrentMap->FloorBlocksData.Num() > 0) {
		CandidateLocation = CurrentMap->FloorBlocksData;
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Invalid SpawnValidFloorBlocksData"));
		return;
	}

	for (int32 i = CandidateLocation.Num() - 1; i > 0; --i) {
		int32 SwapIndex = FMath::RandRange(0, i);
		if (i != SwapIndex) {
			CandidateLocation.Swap(i, SwapIndex);
		}
	}

	int32 TargetSpawnCount = FMath::Min(CoinCount, CandidateLocation.Num());
	int32 SpawnedCount = 0;

	for (FIntVector& Grid : CandidateLocation) {
		if (SpawnedCount >= TargetSpawnCount) {
			break;
		}

		if (!CurrentMap->IsEmptyOnFloorBlock(Grid, {})) continue;

		FVector SpawnLocation = CurrentMap->GridToWorldCenter(Grid.X, Grid.Y, Grid.Z);
		SpawnLocation.X += FMath::RandRange(0.f, 10.f);
		SpawnLocation.Y += FMath::RandRange(0.f, 10.f);
		SpawnLocation.Z += CoinSpawnZOffset;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AActor* SpawnedCoin = GetWorld()->SpawnActor<AActor>(Coin, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

		if (SpawnedCoin) {
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("SpawnMatchCoins : Spawned %d coins"), SpawnedCount);
}

/* 무기, 아이템 Supply */

//무기, 아이템 보급
void APlayMode_Match::SupplyWeaponAndItem()
{
	if (!HasAuthority()) return;
	if (!CurrentMap) return;

	int32 CurrentWeaponCount = CountWorldWeapon();
	int32 CurrentItemCount = CountWorldItem();

	if (CurrentWeaponCount >= SupplyWeaponMaxCount) {
		return;
	}
	if (CurrentItemCount >= SupplyItemMaxCount) {
		return;
	}

	int32 WeaponShortage = SupplyWeaponMaxCount - CurrentWeaponCount;
	int32 ItemShortage = SupplyItemMaxCount - CurrentItemCount;
	int32 SpawnWeaponChangePercent = FMath::Clamp(WeaponShortage * 10, 0, 100);
	int32 SpawnItemChangePercent = FMath::Clamp(ItemShortage * 20, 0, 100);
	int32 Random = FMath::RandRange(1, 100);

	UE_LOG(LogTemp, Warning, TEXT("SupplyCheck : WeaponCount = %d / Shortage = %d / Chance = %d / Roll = %d"),
		CurrentWeaponCount, WeaponShortage, SpawnWeaponChangePercent, Random);
	UE_LOG(LogTemp, Warning, TEXT("SupplyCheck : ItemCount = %d / Shortage = %d / Chance = %d / Roll = %d"),
		CurrentItemCount, ItemShortage, SpawnItemChangePercent, Random);

	if (Random <= SpawnWeaponChangePercent) {
		bool bSpawned = SpawnSupplyWeapon();

		UE_LOG(LogTemp, Warning, TEXT("SupplyCheck : Spawn Result = %s"),
			bSpawned ? TEXT("Success") : TEXT("Fail"));
	}

	if (Random <= SpawnItemChangePercent) {
		bool bSpawned = SpawnSupplyItem();

		UE_LOG(LogTemp, Warning, TEXT("SupplyCheck : Spawn Result = %s"),
			bSpawned ? TEXT("Success") : TEXT("Fail"));
	}
}

void APlayMode_Match::SupplyObjects()
{
	if (!HasAuthority()) return;
	if (!CurrentMap) return;
	
	int32 Random = FMath::RandRange(1, 100);

	if (Random <= 25) {
		bool bSpawned = SpawnSupplyObjects();

		UE_LOG(LogTemp, Warning, TEXT("SupplyCheck : Spawn Result = %s"),
			bSpawned ? TEXT("Success") : TEXT("Fail"));
	}
}

//World 내에 있는 무기 수 획득
int32 APlayMode_Match::CountWorldWeapon()
{
	UWorld* World = GetWorld();
	if (!World) return 0;

	int32 WeaponCount = 0;

	for (TActorIterator<AWeapon> It(World); It; ++It) {
		AWeapon* Weapon = *It;
		if (!IsValid(Weapon)) continue;
		if (Weapon->IsActorBeingDestroyed()) continue;

		if (Weapon->GetAttachParentActor() != nullptr) continue;

		++WeaponCount;
	}

	UE_LOG(LogTemp, Warning, TEXT("Weapon: %d"), WeaponCount);
	
	return WeaponCount;
}

//World 내에 있는 아이템 수 획득
int32 APlayMode_Match::CountWorldItem()
{
	UWorld* World = GetWorld();
	if (!World) return 0;

	int32 ItemCount = 0;

	for (TActorIterator<AItem> It(World); It; ++It) {
		AItem* Item = *It;
		if (!IsValid(Item)) continue;
		if (Item->IsActorBeingDestroyed()) continue;

		if (Item->GetAttachParentActor() != nullptr) continue;

		++ItemCount;
	}

	UE_LOG(LogTemp, Warning, TEXT("Item: %d"), ItemCount);

	return ItemCount;
}

////보급 대상 무기 선정 (확률에 따라 선정)
TSubclassOf<AWeapon> APlayMode_Match::SelectSupplyWeapon()
{
	if (!AllWeaponList) return nullptr;

	int32 Random = FMath::RandRange(1, 100);
	TArray<TSubclassOf<AWeapon>>* TargetPool = nullptr;

	if (Random <= PercentOfB) {
		TargetPool = &AllWeaponList->BWeapons;
	}
	else if (Random <= PercentOfB + PercentOfA && PercentOfB < Random) {
		TargetPool = &AllWeaponList->AWeapons;
	}
	else if (100 - PercentOfS < Random ) {
		TargetPool = &AllWeaponList->SWeapons;
	}

	//선택된 등급 풀이 비었다면 fallback
	if (!TargetPool || TargetPool->Num() == 0) {
		if (AllWeaponList->BWeapons.Num() > 0) {
			TargetPool = &AllWeaponList->BWeapons;
		}
		else if (AllWeaponList->AWeapons.Num() > 0) {
			TargetPool = &AllWeaponList->AWeapons;
		}
		else if (AllWeaponList->SWeapons.Num() > 0) {
			TargetPool = &AllWeaponList->SWeapons;
		}
		else {
			return nullptr;
		}
	}
	
	int32 Index = FMath::RandRange(0, TargetPool->Num() - 1);
	return (*TargetPool)[Index];
}

//보급 대상 아이템 선정 (확률에 따라 선정)
TSubclassOf<AItem> APlayMode_Match::SelectSupplyItem()
{
	if (!AllItemList) return nullptr;

	int32 Random = FMath::RandRange(1, 100);
	TArray<TSubclassOf<AItem>>* TargetPool = nullptr;

	if (Random <= PercentOfB) {
		TargetPool = &AllItemList->BItems;
	}
	else if (Random <= PercentOfB + PercentOfA && PercentOfB < Random) {
		TargetPool = &AllItemList->AItems;
	}
	else if (100 - PercentOfS < Random) {
		TargetPool = &AllItemList->SItems;
	}

	//선택된 등급 풀이 비었다면 fallback
	if (!TargetPool || TargetPool->Num() == 0) {
		if (AllItemList->BItems.Num() > 0) {
			TargetPool = &AllItemList->BItems;
		}
		else if (AllItemList->AItems.Num() > 0) {
			TargetPool = &AllItemList->AItems;
		}
		else if (AllItemList->SItems.Num() > 0) {
			TargetPool = &AllItemList->SItems;
		}
		else {
			return nullptr;
		}
	}

	int32 Index = FMath::RandRange(0, TargetPool->Num() - 1);
	return (*TargetPool)[Index];
}

//소환 대상 물체 선정
TSubclassOf<AObjects> APlayMode_Match::SelectSpawnObjects()
{
	if (!AllObjectsList) return nullptr;

	int32 Random = FMath::RandRange(1, 100);
	TArray<TSubclassOf<AObjects>>* TargetPool = nullptr;

	if (Random <= 50) {
		TargetPool = &AllObjectsList->EquipObjects;
	}
	else if (Random <= 100 && 50 < Random) {
		TargetPool = &AllObjectsList->NormalObjects;
	}

	//선택된 등급 풀이 비었다면 fallback
	if (!TargetPool || TargetPool->Num() == 0) {
		if (AllObjectsList->EquipObjects.Num() > 0) {
			TargetPool = &AllObjectsList->EquipObjects;
		}
		else if (AllObjectsList->NormalObjects.Num() > 0) {
			TargetPool = &AllObjectsList->NormalObjects;
		}
		else{
			return nullptr;
		}
	}

	int32 Index = FMath::RandRange(0, TargetPool->Num() - 1);
	return (*TargetPool)[Index];
}

//보급 대상 무기 소환
bool APlayMode_Match::SpawnSupplyWeapon()
{
	if (!HasAuthority()) return false;
	if (!GetWorld()) return false;
	if (!CurrentMap) return false;

	TSubclassOf<AWeapon> SelectedWeapon = SelectSupplyWeapon();
	if (!SelectedWeapon) return false;

	if (CurrentMap->FloorBlocksData.Num() <= 0) return false;

	TArray<FIntVector> CandidateLocation = CurrentMap->FloorBlocksData;

	for (int32 i = CandidateLocation.Num() - 1; i > 0; --i) {
		int32 SwapIndex = FMath::RandRange(0, i);
		if (i != SwapIndex) CandidateLocation.Swap(i, SwapIndex);
	}

	for (const FIntVector& Grid : CandidateLocation) {
		FIntVector TargetGrid = Grid;

		if (!CurrentMap->IsEmptyOnFloorBlock(TargetGrid, {})) continue;

		FVector SpawnLocation = CurrentMap->GridToWorldCenter(Grid.X, Grid.Y, Grid.Z);
		//소환 대상 좌표의 X,Y 좌표에 약간의 랜덤성 부여
		SpawnLocation += FVector(FMath::FRandRange(0.f, 5.f), FMath::FRandRange(0.f, 5.f), SupplySpawnZOffset);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(SelectedWeapon, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

		if (SpawnedWeapon) {
			UE_LOG(LogTemp, Warning, TEXT("SpawnSupplyWeapon : Spawned %s at Grid(%d, %d, %d)"), *SpawnedWeapon->GetName(), Grid.X, Grid.Y, Grid.Z);
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("SpawnSupplyWeapon : No valid empty floor found"));
	return false;
}

//보급 대상 아이템 소환
bool APlayMode_Match::SpawnSupplyItem()
{
	if (!HasAuthority()) return false;
	if (!GetWorld()) return false;
	if (!CurrentMap) return false;

	TSubclassOf<AItem> SelectedItem = SelectSupplyItem();
	if (!SelectedItem) return false;

	if (CurrentMap->FloorBlocksData.Num() <= 0) return false;

	TArray<FIntVector> CandidateLocation = CurrentMap->FloorBlocksData;

	for (int32 i = CandidateLocation.Num() - 1; i > 0; --i) {
		int32 SwapIndex = FMath::RandRange(0, i);
		if (i != SwapIndex) CandidateLocation.Swap(i, SwapIndex);
	}

	for (const FIntVector& Grid : CandidateLocation) {
		FIntVector TargetGrid = Grid;

		if (!CurrentMap->IsEmptyOnFloorBlock(TargetGrid, {})) continue;

		FVector SpawnLocation = CurrentMap->GridToWorldCenter(Grid.X, Grid.Y, Grid.Z);
		//소환 대상 좌표의 X,Y 좌표에 약간의 랜덤성 부여
		SpawnLocation += FVector(FMath::FRandRange(0.f, 5.f), FMath::FRandRange(0.f, 5.f), SupplySpawnZOffset);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AItem* SpawnedItem = GetWorld()->SpawnActor<AItem>(SelectedItem, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

		if (SpawnedItem) {
			return true;
		}
	}

	return false;
}

//선정 대상 물체 소환
bool APlayMode_Match::SpawnSupplyObjects()
{
	if (!HasAuthority()) return false;
	if (!GetWorld()) return false;
	if (!CurrentMap) return false;

	TSubclassOf<AObjects> SelectedObjects = SelectSpawnObjects();
	if (!SelectedObjects) return false;

	if (CurrentMap->FloorBlocksData.Num() <= 0) return false;

	TArray<FIntVector> CandidateLocation = CurrentMap->FloorBlocksData;

	for (int32 i = CandidateLocation.Num() - 1; i > 0; --i) {
		int32 SwapIndex = FMath::RandRange(0, i);
		if (i != SwapIndex) CandidateLocation.Swap(i, SwapIndex);
	}

	for (const FIntVector& Grid : CandidateLocation) {
		FIntVector TargetGrid = Grid;

		if (!CurrentMap->IsEmptyOnFloorBlock(TargetGrid, {})) continue;

		FVector SpawnLocation = CurrentMap->GridToWorldCenter(Grid.X, Grid.Y, Grid.Z);
		//소환 대상 좌표의 X,Y 좌표에 약간의 랜덤성 부여
		SpawnLocation += FVector(FMath::FRandRange(0.f, 5.f), FMath::FRandRange(0.f, 5.f), SupplySpawnZOffset);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AObjects* SpawnedObjects = GetWorld()->SpawnActor<AObjects>(SelectedObjects, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

		if (SpawnedObjects) {
			return true;
		}
	}
	return false;
}
