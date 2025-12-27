// Fill out your copyright notice in the Description page of Project Settings.


#include "BoWidget.h"
#include "ParagonGideon.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Actor.h"
#include "Math/Vector.h"

TSharedRef<SWidget> UBoWidget::RebuildWidget()
{
    TSharedRef<SWidget> Widget = Super::RebuildWidget();

    // 在子类中只修改 HealthProgressBar 的位置和大小
    if (HealthProgressBar)
    {
        UCanvasPanelSlot* BarSlot = Cast<UCanvasPanelSlot>(HealthProgressBar->Slot);
        if (BarSlot)
        {
            BarSlot->SetAnchors(FAnchors(0.5f, 1.0f));


            BarSlot->SetAlignment(FVector2D(0.5f, 1.0f));

            BarSlot->SetPosition(FVector2D(0.0f, -50.0f));

            BarSlot->SetSize(FVector2D(800.0f, 40.0f));

            HealthProgressBar->SetVisibility(ESlateVisibility::Visible);
        }
    }

    return Widget;
}

void UBoWidget::SetBossReference(AParagonGideon* InBoss)
{
    BossActor = InBoss;
}

void UBoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    //Super::NativeTick(MyGeometry, InDeltaTime);

    if (!BossActor) return;

    // 1. 处理距离显示逻辑 (保持你原有的逻辑)
    APawn* PlayerPawn = GetOwningPlayerPawn();
    if (PlayerPawn)
    {
        float Distance = FVector::Dist(BossActor->GetActorLocation(), PlayerPawn->GetActorLocation());
        bool bIsInRange = Distance <= CircleRadius;

        // 如果想测试，可以先把这行注释掉，强制设为 Visible
        HealthProgressBar->SetVisibility(bIsInRange ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    // 2. --- 新增：实时更新血量 ---
    // 假设 BossActor 有 GetHealth() 之类的函数，或者直接访问 Health 变量
    // 这里假设最大血量是 100，你需要根据实际情况修改
    UpdateHealth(BossActor->CurrentHealth, BossActor->MaxHealth);
}