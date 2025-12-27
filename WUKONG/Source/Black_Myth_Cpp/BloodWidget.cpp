// Fill out your copyright notice in the Description page of Project Settings.

#include "BloodWidget.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> UBloodWidget::RebuildWidget()
{
    // 1. 构建根节点 (Canvas Panel)
     // 使用 WidgetTree->ConstructWidget 来创建组件
    RootCanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));

    // 将其设置为这个 UserWidget 的根组件
    WidgetTree->RootWidget = RootCanvasPanel;

    // 2. 创建进度条 (Progress Bar)
    HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));

    // 3. 将进度条添加到 Canvas 中
    if (RootCanvasPanel && HealthProgressBar)
    {
        // AddChildToCanvas 会返回一个 Slot，我们需要用它来设置位置和大小
        UCanvasPanelSlot* BarSlot = RootCanvasPanel->AddChildToCanvas(HealthProgressBar);

        if (BarSlot)
        {

            // 锚点 (Anchors): 左上角 (0,0)
            BarSlot->SetAnchors(FAnchors(0.0f, 0.0f));

            // 位置 (Position): X=50, Y=50
            BarSlot->SetPosition(FVector2D(50.0f, 50.0f));

            // 大小 (Size): 300 x 30
            BarSlot->SetSize(FVector2D(300.0f, 30.0f));
        }

        FProgressBarStyle Style = HealthProgressBar->GetWidgetStyle();

        Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f, 1.0f));
        Style.FillImage.TintColor = FSlateColor(FLinearColor::White);

        HealthProgressBar->SetWidgetStyle(Style);
    }

    // 4. 返回底层的 Slate Widget
    return Super::RebuildWidget();
}

void UBloodWidget::UpdateHealth(float Current, float Max)
{
    if (HealthProgressBar && Max > 0)
    {
        HealthProgressBar->SetPercent(FMath::Clamp(Current / Max, 0.0f, 1.0f));
    }
}