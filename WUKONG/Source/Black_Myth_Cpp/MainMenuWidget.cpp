// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuWidget.h"
#include "MainMenuPlayerController.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"       // 用于加载关卡
#include "Kismet/KismetSystemLibrary.h"   // 用于退出游戏

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
    UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
    WidgetTree->RootWidget = RootOverlay;

    if (RootOverlay)
    {
        // --- 1. 背景图片 ---
        BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BGImage"));
        if (BackgroundImage)
        {
            UOverlaySlot* BGSlot = RootOverlay->AddChildToOverlay(BackgroundImage);
            if (BGSlot)
            {
                // 让图片填满全屏
                BGSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
                BGSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
            }

            // 加载背景图 (请替换成你的图片路径)
            FString Path = TEXT("/Game/UI/menu");
            UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path);
            if (Tex) BackgroundImage->SetBrushFromTexture(Tex);
            else BackgroundImage->SetColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.2f, 1.0f)); // 没图就显示深蓝色
        }

        // --- 2. 垂直盒子 (用来存放那一排按钮) ---
        UVerticalBox* MenuContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuContainer"));
        if (MenuContainer)
        {
            UOverlaySlot* BoxSlot = RootOverlay->AddChildToOverlay(MenuContainer);
            if (BoxSlot)
            {
                // 让按钮菜单 居中 显示
                BoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
                BoxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
            }

            // --- 3. 使用辅助函数创建4个按钮 ---

            // 按钮1：开始游戏 (绑定 OnStartGameClicked)
            CreateMenuButton(MenuContainer, TEXT("Start Game"), TEXT("OnStartGameClicked"));


            // 按钮4：退出游戏 (绑定 OnQuitGameClicked)
            CreateMenuButton(MenuContainer, TEXT("Quit Game"), TEXT("OnQuitGameClicked"));
        }
    }

    return RootOverlay->TakeWidget();
}

// --- 辅助函数：创建漂亮的按钮 ---
UButton* UMainMenuWidget::CreateMenuButton(UVerticalBox* ParentBox, FString ButtonText, FName FunctionName)
{
    if (!ParentBox) return nullptr;

    // 1. 创建按钮
    UButton* NewBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

    // 2. 将按钮放入垂直盒子
    UVerticalBoxSlot* BoxSlot = ParentBox->AddChildToVerticalBox(NewBtn);
    if (BoxSlot)
    {
        BoxSlot->SetPadding(FMargin(0.0f, 10.0f)); // 按钮之间的间距
        BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); // 宽度填充
    }

    // 3. 给按钮里面加文字
    UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (BtnText)
    {
        BtnText->SetText(FText::FromString(ButtonText));
        BtnText->SetJustification(ETextJustify::Center); // 文字居中
        NewBtn->AddChild(BtnText);
    }

    // 4. 绑定点击事件 (核心)
    FScriptDelegate Delegate;
    Delegate.BindUFunction(this, FunctionName);
    NewBtn->OnClicked.Add(Delegate);

    return NewBtn;
}

// ================== 功能实现接口 ==================

void UMainMenuWidget::OnStartGameClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Start Game Clicked!"));
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        AMainMenuPlayerController* MPC = Cast<AMainMenuPlayerController>(PC);
        if (MPC)
        {
            MPC->SwitchToGameInputMode();  
        }
    }
     UGameplayStatics::OpenLevel(this, FName("LandscapeAutoMaterial_MountainRange_Example"));
}

void UMainMenuWidget::OnSelectLevelClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Select Level Clicked!"));

}

void UMainMenuWidget::OnCreditsClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Credits Clicked!"));

}

void UMainMenuWidget::OnQuitGameClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Quit Game Clicked!"));

    // 获取 PlayerController 以执行退出
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
    }
}
