// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Title_Widget.generated.h"

/**
 * 
 */
class UEditableTextBox;
class UButton;
class UTextBlock;
class UThrobber;
class UImage;
class UOverlay;

//FOnTitlePlayRequested 이벤트 타입 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTitlePlayRequested, FString, NickName, EMatchMode, MatchMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTitleCancelRequested);


UCLASS()
class PROJECTCC_API UTitle_Widget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	//위젯이 생성된 후 호출
	virtual void NativeConstruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	//닉네임 획득
	UFUNCTION(BlueprintCallable, Category = "Title UI")
	FString GetNicknameString();
	//닉네임 설정
	UFUNCTION(BlueprintCallable, Category = "Title UI")
	void SetNicknameText(const FString& nickname);
	//닉네임 입력, 수정 허용/방지
	UFUNCTION()
	void SetNicknameLocked(bool bLocked);
	//매칭 모드로 위젯 변경
	UFUNCTION()
	void SetMatchingMode(bool bMatching);
	//매칭 성공 모드로 위젯 변경
	UFUNCTION()
	void SetJoinCompleteMode();
	//Text_Status의 Text 변경
	UFUNCTION(BlueprintCallable, Category = "Title UI")
	void SetStatusMessage(const FText& text);
	//닉네임 입력 칸에 키보드 입력을 받게 함
	UFUNCTION(BlueprintCallable, Category = "Title UI")
	void FocusNicknameBox();
	UFUNCTION(BlueprintCallable, Category="Title UI")
	void ClearStatusMessage();
	//상태 메세지를 온전히 보여주도록 설정
	UFUNCTION(BlueprintCallable, Category = "Title UI")
	void SetStatusMessageShowing(const FText& text);
	//상태 메세지를 서서히 사라지도록 설정
	UFUNCTION(BlueprintCallable, Category = "Title UI")
	void SetStatusMessageFadeOut(const FText& text, float VisibleTime);
	//Play 버튼 클릭 시 게임 전체에 BroadCast
	UPROPERTY(BlueprintAssignable, Category = "Title UI")
	FOnTitlePlayRequested OnTitlePlayRequested;
	UPROPERTY(BlueprintAssignable)
	FOnTitleCancelRequested OnTitleCancelRequested;

	//에러메세지 제거 타이머
	FTimerHandle ClearText_StatusTimer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Title UI")
	float ErrorFadeTime = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Title UI")
	float ErrorVisibleTime = 3.f;

	bool bStatusFading = false;
	float StatusElapsedTime = 0.f;
	bool bNicknameLocked = false;
	float CurrentStatusVisibleTIme = 0.f;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Title UI")
	EMatchMode SelectedMatchMode;

	//위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> EditableTextBox_Nickname;
	//위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Play;

	//2인-4인플레이 전환 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_TwoPlayer;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_FourPlayer;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_TwoPlayerButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_FourPlayerButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_TwoPlayer;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_FourPlayer;

	//매치모드 선택 프레임
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_SelectedFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Title UI")
	FLinearColor UnSelectedButtonTint = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

	//프레임 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title UI")
	float FrameMoveSpeed = 20.f;
	//프레임 위치 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title UI|Match Mode Visual")
	FVector2D FramePositionOffset = FVector2D(0.f, 0.f);
	//프레임 이동 중 여부
	bool bFrameMoving = false;
	//프레임 목표 위치
	FVector2D TargetFramePosition = FVector2D::ZeroVector;

	//위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Exit;
	//위젯 바인딩(안해도 컴파일 가능)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;
	//위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_CancelMatch;
	//위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UThrobber> Throbber_Matching;

	//Play버튼 클릭 시 호출
	UFUNCTION()
	void HandlePlayButtonClicked();
	UFUNCTION()
	void HandleCancelButtonClicked();
	UFUNCTION()
	void HandleExitButtonClicked();

	//매치모드 전환 버튼 클릭 시 호출
	UFUNCTION()
	void HandleTwoPlayerButtonClicked();
	UFUNCTION()
	void HandleFourPlayerButtonClicked();

	//닉네임 입력 박스 텍스트 변경 시 갱신
	UFUNCTION()
	void HandleNickNameTextChanged(const FText& NewText);

	UFUNCTION(BlueprintCallable, Category="Title UI")
	void SetMatchMode(EMatchMode TheMatchMode);

	//닉네임 입력 박스 갱신
	void RefreshNicknameBox();

	//매치모드 버튼 프레임 갱신
	void ApplyMatchMode(EMatchMode NewMatchMode, bool bInstant);
	void RefreshMatchModeButtons();
	void UpdateSelectedFrameTargetPosition();
	void SnapSelectedFrameToTargetLocation();

	//[클릭]
	UFUNCTION()
	void PlayCommonUIClickSound();
};
