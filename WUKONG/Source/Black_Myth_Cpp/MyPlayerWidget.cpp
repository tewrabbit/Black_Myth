// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerWidget.h"
#include "SkillSlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h" 
#include "InventoryWidget.h"
#include "UQuickItemSlotWidget.h"

TSharedRef<SWidget> UMyPlayerWidget::RebuildWidget()
{
    // 1. 先调用父类的 RebuildWidget
    TSharedRef<SWidget> BuiltWidget = Super::RebuildWidget();

    // 2. 检查父类的画布是否存在 (它是 protected 的，所以我们可以直接用)
    if (RootCanvasPanel)
    {
        // 3. 手动创建蓝条组件
        ManaBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ManaBar"));

        if (ManaBar)
        {
            // 4. 把蓝条添加到父类创建的那个画布上
            UCanvasPanelSlot* ManaSlot = RootCanvasPanel->AddChildToCanvas(ManaBar);

            if (ManaSlot)
            {
                // --- 调整位置 ---
                ManaSlot->SetAnchors(FAnchors(0.0f, 0.0f)); // 左上角锚点

                // 父类血条位置是 (50, 50)，高度是 30。
                // 所以血条的底部在 Y = 80。
                // 我们把蓝条放在 Y = 90 的位置，留 10 像素间隙。
                ManaSlot->SetPosition(FVector2D(50.0f, 90.0f));

                // 大小设置 (和血条一样宽，一样高)
                ManaSlot->SetSize(FVector2D(300.0f, 30.0f));
            }

            // --- 调整颜色 (变成蓝色) ---
            FProgressBarStyle Style = ManaBar->GetWidgetStyle();

            // 填充色：纯蓝
            Style.FillImage.TintColor = FSlateColor(FLinearColor(0.0f, 0.2f, 1.0f, 1.0f));

            // 背景色：深灰 (可选，为了看清楚空槽)
            Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));

            ManaBar->SetWidgetStyle(Style);

            // 初始设为 100%
            ManaBar->SetPercent(1.0f);
        }

        // 2. 创建技能槽 (不再是一堆图片和文字，而是一个控件)
        USizeBox* SkillContainer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SkillContainer"));
        SkillContainer->SetWidthOverride(160.0f);
        SkillContainer->SetHeightOverride(160.0f);

        // 把盒子放到左下角
        UCanvasPanelSlot* SkillPosSlot = RootCanvasPanel->AddChildToCanvas(SkillContainer);
        if (SkillPosSlot)
        {
            SkillPosSlot->SetAnchors(FAnchors(0.0f, 1.0f)); // 左下角
            SkillPosSlot->SetPosition(FVector2D(50.0f, -150.0f)); // 往上提
            SkillPosSlot->SetSize(FVector2D(160.0f, 160.0f));
            SkillPosSlot->SetAlignment(FVector2D(0.0f, 0.0f));
        }

        // 核心：实例化我们的自定义类 USkillSlotWidget
        SkillSlot = WidgetTree->ConstructWidget<USkillSlotWidget>(USkillSlotWidget::StaticClass(), TEXT("MySkillSlot"));

        // 把自定义控件塞进 SizeBox 里
        SkillContainer->SetContent(SkillSlot);


        // =========================
        // ✅ 1) 创建右下角快捷槽
        // =========================
        USizeBox* QuickSlotContainer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuickSlotContainer"));
        QuickSlotContainer->SetWidthOverride(96.f);
        QuickSlotContainer->SetHeightOverride(96.f);

        UCanvasPanelSlot* QuickSlotPos = RootCanvasPanel->AddChildToCanvas(QuickSlotContainer);
        if (QuickSlotPos)
        {
            QuickSlotPos->SetAnchors(FAnchors(1.f, 1.f));          // 右下角锚点
            QuickSlotPos->SetAlignment(FVector2D(1.f, 1.f));       // 对齐到控件右下角
            QuickSlotPos->SetPosition(FVector2D(-50.f, -50.f));    // 往左上偏移（负数）
            QuickSlotPos->SetSize(FVector2D(124.f, 124.f));
        }

        // 创建快捷槽控件
        QuickSlotWidget = WidgetTree->ConstructWidget<UQuickItemSlotWidget>(
            UQuickItemSlotWidget::StaticClass(), TEXT("QuickItemSlot")
        );
        QuickSlotContainer->SetContent(QuickSlotWidget);


        // =========================
        // ✅ 2) 创建背包（如果你还没创建）
        //    然后把快捷槽传进去
        // =========================
        if (!InventoryWidget)
        {
            InventoryWidget = WidgetTree->ConstructWidget<UInventoryWidget>(
                UInventoryWidget::StaticClass(), TEXT("InventoryWidget")
            );

            // 你想把背包放哪都行，这里示例放中间偏左
            UCanvasPanelSlot* InvSlot = RootCanvasPanel->AddChildToCanvas(InventoryWidget);
            if (InvSlot)
            {
                InvSlot->SetAnchors(FAnchors(0.5f, 0.5f));
                InvSlot->SetAlignment(FVector2D(0.5f, 0.5f));
                InvSlot->SetPosition(FVector2D(0.f, 0.f));
                InvSlot->SetSize(FVector2D(1200.f, 750.f));
            }
        }
        InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
        // 把快捷槽“注入”背包，背包就能在选中血瓶/蓝瓶时更新右下角显示
        if (InventoryWidget && QuickSlotWidget)
        {
            InventoryWidget->SetQuickSlot(QuickSlotWidget);
        }
    }

    // 5. 返回整个构建好的界面
    return BuiltWidget;
}

void UMyPlayerWidget::UpdateMana(float CurrentMana, float MaxMana)
{
    if (ManaBar && MaxMana > 0)
    {
        float Percent = CurrentMana / MaxMana;
        ManaBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
    }
}

bool UMyPlayerWidget::TriggerSkillCooldown(float Duration,int choice)
{
    if (SkillSlot)
    {
       return SkillSlot->StartCooldown(choice, Duration);
    }
    return false;
}

void UMyPlayerWidget::UseQuickItem()
{
    if (InventoryWidget)
    {
        InventoryWidget->UseSelectedQuickItem();
    }
}

void UMyPlayerWidget::ToggleInventory()
{
    if (!InventoryWidget) return;

    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    const bool bIsHidden = (InventoryWidget->GetVisibility() == ESlateVisibility::Hidden);

    if (bIsHidden)
    {
        // --- 打开背包：显示 + 鼠标 + UI输入 ---
        InventoryWidget->SetVisibility(ESlateVisibility::Visible);

        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);

        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);

        // 可选：禁用角色转向（防止鼠标一动角色就转）
        // PC->SetIgnoreLookInput(true);
    }
    else
    {
        // --- 关闭背包：隐藏 + 关鼠标 + 游戏输入 ---
        InventoryWidget->SetVisibility(ESlateVisibility::Hidden);

        PC->SetInputMode(FInputModeGameOnly());
        PC->SetShowMouseCursor(false);

        // 可选：恢复角色转向
        // PC->SetIgnoreLookInput(false);
    }
}
