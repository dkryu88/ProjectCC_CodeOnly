// Fill out your copyright notice in the Description page of Project Settings.


#include "AllPlayMode_GameInstance.h"
// [사운드]
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UAllPlayMode_GameInstance::SetLocalPlayerNickname(const FString& NewNickName)
{
	LocalPlayerNickName = NewNickName;
	//앞뒤 공백 제거
	LocalPlayerNickName.TrimStartAndEndInline();
}

FString UAllPlayMode_GameInstance::GetPlayerLocalNickname()
{
	return LocalPlayerNickName;
}

// [사운드]bgm 재생
void UAllPlayMode_GameInstance::PlayBgm(USoundBase* NewBgm)
{
	if (!NewBgm) return;
	if (CurrentBgmComponent && CurrentBgmComponent->IsPlaying() && CurrentBgmComponent->GetSound() == NewBgm)return;

	// 다음 노래 예약(임시저장)
	NextBgm = NewBgm;

	// 현재 재생중이라면
	if (CurrentBgmComponent && CurrentBgmComponent->IsPlaying()) {
		// 1초동안 소리크기 0으로 줄어듬 + 재생정지
		CurrentBgmComponent->FadeOut(BgmFadeDuration, 0.f);

		// fadeout이 끝나는 1초 뒤에 PlayNextBgm함수가 실행되도록 타이머 설정
		GetWorld()->GetTimerManager().SetTimer(BgmTransitionTimer, this, &UAllPlayMode_GameInstance::PlayNextBgm, BgmFadeDuration, false);
	}
	else{
		// 재생중인 음악이 없다면 바로 다음 노래 재생
		PlayNextBgm();
	}
}

// [사운드]PlayBgm함수에서 예약된 다음 노래 재생
void UAllPlayMode_GameInstance::PlayNextBgm()
{
	if (!NextBgm) return;

	CurrentBgmComponent = UGameplayStatics::CreateSound2D(this, NextBgm, 1.f, 1.f, 0.f, nullptr, false, false);

	if (CurrentBgmComponent) {
		// 1초동안 정상볼륨으로 키움 + 재생
		CurrentBgmComponent->FadeIn(BgmFadeDuration, 1.f);
	}
	//다음 노래 변수 초기화
	NextBgm = nullptr;
}

// [사운드]bgm 정지
void UAllPlayMode_GameInstance::StopBgm()
{
	if (CurrentBgmComponent && CurrentBgmComponent->IsPlaying()) {
		CurrentBgmComponent->FadeOut(BgmFadeDuration, 0.f);
	}
	GetWorld()->GetTimerManager().ClearTimer(BgmTransitionTimer);
}

// [사운드]bgm 재생시간 return
float UAllPlayMode_GameInstance::GetCurrnetBgmDuration() const
{
	if (CurrentBgmComponent && CurrentBgmComponent->GetSound()) {
		return CurrentBgmComponent->GetSound()->GetDuration();
	}
	return 0.0f;
}

// [사운드]bgm 볼륨 조절
void UAllPlayMode_GameInstance::AdjustBgmVolume(float FadeTime, float TargetVolum)
{
	if (CurrentBgmComponent && CurrentBgmComponent->IsPlaying()) {
		CurrentBgmComponent->AdjustVolume(FadeTime, TargetVolum);
	}
}

// [사운드]bgm 재생 속도
void UAllPlayMode_GameInstance::SetBgmPitch(float NewPitch)
{
	if (CurrentBgmComponent && CurrentBgmComponent->IsPlaying()) {
		CurrentBgmComponent->SetPitchMultiplier(NewPitch);
	}
}


//FadeIn = Play() + 소리 서서히 키우기
//FadeOut = 소리 서서히 줄이기 + Stop()
