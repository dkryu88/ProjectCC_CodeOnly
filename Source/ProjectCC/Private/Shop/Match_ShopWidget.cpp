// Fill out your copyright notice in the Description page of Project Settings.


#include "Shop/Match_ShopWidget.h"
#include "Components/Button.h"
#include "Shop/ShopStock.h"
#include "Match_PlayerController.h"
#include "Components/TextBlock.h"
#include "Match_State.h"
#include "Player_State.h"

void UMatch_ShopWidget::NativeConstruct() {
	Super::NativeConstruct();

	if (Button_B_Box) {
		Button_B_Box->IsFocusable = false;
		Button_B_Box->OnClicked.AddDynamic(this, &UMatch_ShopWidget::OnClicked_B_Box);
	}
	if (Button_A_Box) {
		Button_A_Box->IsFocusable = false;
		Button_A_Box->OnClicked.AddDynamic(this, &UMatch_ShopWidget::OnClicked_A_Box);
	}
	if (Button_S_Box) {
		Button_S_Box->IsFocusable = false;
		Button_S_Box->OnClicked.AddDynamic(this, &UMatch_ShopWidget::OnClicked_S_Box);
	}
	if (Button_Random_Box) {
		Button_Random_Box->IsFocusable = false;
		Button_Random_Box->OnClicked.AddDynamic(this, &UMatch_ShopWidget::OnClicked_Random_Box);
	}

	RefreshPriceTexts();
}

void UMatch_ShopWidget::Purchase(EShopBoxs Box)
{
	AMatch_PlayerController* PC = GetOwningPlayer<AMatch_PlayerController>();
	if (!PC) return;

	PC->Server_Purchase(Box);
}

void UMatch_ShopWidget::RefreshPriceTexts()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AMatch_State* MS = World->GetGameState<AMatch_State>();
	if (!MS) return;

	if (Text_B_BoxPrice) {
		Text_B_BoxPrice->SetText(FText::AsNumber(MS->GetShopPrice(EShopBoxs::B_Box)));
	}
	if (Text_A_BoxPrice) {
		Text_A_BoxPrice->SetText(FText::AsNumber(MS->GetShopPrice(EShopBoxs::A_Box)));
	}
	if (Text_S_BoxPrice) {
		Text_S_BoxPrice->SetText(FText::AsNumber(MS->GetShopPrice(EShopBoxs::S_Box)));
	}
	if (Text_Random_BoxPrice) {
		Text_Random_BoxPrice->SetText(FText::AsNumber(MS->GetShopPrice(EShopBoxs::Random_Box)));
	}
}

void UMatch_ShopWidget::OnClicked_B_Box()
{
	Purchase(EShopBoxs::B_Box);
}

void UMatch_ShopWidget::OnClicked_A_Box()
{
	Purchase(EShopBoxs::A_Box);
}

void UMatch_ShopWidget::OnClicked_S_Box()
{
	Purchase(EShopBoxs::S_Box);
}

void UMatch_ShopWidget::OnClicked_Random_Box()
{
	Purchase(EShopBoxs::Random_Box);
}
