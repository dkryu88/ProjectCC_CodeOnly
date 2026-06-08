// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player_AdditionalWidget.generated.h"

/**
 * 
 */
class UImage;
class UTexture2D;
class UWidgetAnimation;

USTRUCT(BlueprintType)
struct FAdditionalImage {
	GENERATED_BODY()

	//이미지 이름(사용처)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ImageName;
	//이미지 번호
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ImageID = -1;
	//이미지 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Image;
};

UCLASS(BlueprintType, Blueprintable)
class PROJECTCC_API UPlayer_AdditionalWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void PlayFadeOutAndRemove();

	UFUNCTION(BlueprintCallable)
	void SetImageByID(int32 ImageID);

	UFUNCTION(BlueprintCallable)
	void ClearImage();

protected:
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> FadeOutAnim;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Additional;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AdditionalImage")
	TArray<FAdditionalImage> AdditionalImages;

	UFUNCTION()
	void HandleFadeOutFinished();
};
