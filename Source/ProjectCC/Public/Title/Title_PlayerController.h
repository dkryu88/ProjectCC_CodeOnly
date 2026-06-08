// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Title_PlayerController.generated.h"

/**
 * 
 */
class UTitle_Widget;
class ATitle_PlayerCharacter;
class ACameraActor;
class UTitle_AdditionalWidget;

UCLASS()
class PROJECTCC_API ATitle_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ATitle_PlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	UFUNCTION()
	void ToggleAdditionalWidget();
	UFUNCTION()
	void CloseAdditionalWidget();
	UFUNCTION()
	void ConfirmQuitGame();

protected:
	//타이틀 화면 위젯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title UI")
	TSubclassOf<UTitle_Widget> TitleWidget;

	//타이틀 화면 위젯 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UTitle_Widget> TitleWidgetInstance;

	//프리뷰 캐릭터
	UPROPERTY(Transient)
	TObjectPtr<ATitle_PlayerCharacter> PreviewCharacter;

	//카메라 태그 (검색에 사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Title Camera")
	FName TitleCameraTag = TEXT("Title Camera");

	//타이틀 화면 카메라
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> TitleCamera;

	//프리뷰 캐릭터 회전 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Title Preview")
	float PreviewRotateSpeed = 0.25;

	//프리뷰 캐릭터 태그 (검색에 사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Title Preview")
	FName PreviewActorTag = TEXT("Title Preview");

	//타이틀 추가 위젯//
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UTitle_AdditionalWidget> Title_AdditionalWidget;
	UPROPERTY()
	TObjectPtr<UTitle_AdditionalWidget> AdditionalWidget;
	
	bool bIsPreviewDragging = false;
	float LastMouseX = 0.f;

protected:
	//타이틀 화면 카메라 검색 및 설정
	void FindAndSetTitleCamera();
	//타이틀 화면 위젯 배치
	void CreateAndShowTitleWidget();
	//프리뷰 캐릭터 검색
	void FindPreviewActor();
	//타이틀 화면 Input 설정
	void SetTitleInputMode();
	
	//프리뷰 캐릭터 회전
	void UpdatePreviewRotation();
	//마우스 우클릭 시작
	void OnLeftMousePressed();
	//마우스 우클릭 완료
	void OnLeftMouseReleased();

	//플레이 버튼 입력 시 기능
	UFUNCTION()
	void HandlePlayRequested(FString NickName);
	UFUNCTION()
	void HandleCancelRequested();

	UFUNCTION()
	void HandleSessionStateChanged(ESessionUIState State, const FString& Message);

	//닉네임이 유효한지 검사
	bool ValidateNickname(const FString& nickname, FString& OutSanitized, FText& OutErrorText);
};
