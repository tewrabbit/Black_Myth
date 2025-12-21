// Fill out your copyright notice in the Description page of Project Settings.


#include "BoWidget.h"
#include "ParagonFengMao.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Actor.h"
#include "Math/Vector.h"

TSharedRef<SWidget> UBoWidget::RebuildWidget()
{
    // 保留基类的构建逻辑
    TSharedRef<SWidget> Widget = Super::RebuildWidget();

    // 在子类中只修改 HealthProgressBar 的位置和大小
    if (HealthProgressBar)
    {
        UCanvasPanelSlot* BarSlot = Cast<UCanvasPanelSlot>(HealthProgressBar->Slot);
        if (BarSlot)
        {
            // 只修改子类的尺寸和位置
            BarSlot->SetPosition(FVector2D(100.0f, 100.0f)); // 修改位置
            BarSlot->SetSize(FVector2D(400.0f, 40.0f)); // 修改大小
        }
    }

    return Widget;
}

void UBoWidget::SetBossReference(ABoss* InBoss)
{
    BossActor = InBoss;
}

void UBoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!BossActor) return;

    // 获取玩家的位置
    APawn* PlayerPawn = GetOwningPlayer()->GetPawn();
    if (!PlayerPawn) return; // 如果玩家没有Pawn，返回

    // 获取Boss的位置
    //FVector BossLocation = <ACharacter>BossActor->GetActorLocation();

    // 获取玩家的位置
    FVector PlayerLocation = PlayerPawn->GetActorLocation();

    // 计算玩家和Boss之间的距离
    //float Distance = FVector::Dist(BossLocation, PlayerLocation);

    // 判断玩家是否在Boss的显示范围内
    //bool bIsInRange = Distance <= CircleRadius;

    // 根据玩家与Boss的距离控制血条的显示
    //if (HealthProgressBar)
    //{
    //    // 如果玩家在范围内，显示血条；否则隐藏血条
    //    HealthProgressBar->SetVisibility(bIsInRange ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    //}
}