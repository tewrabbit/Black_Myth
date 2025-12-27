#include "UQuickItemSlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UQuickItemSlotWidget::RebuildWidget()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("QuickRoot"));
    WidgetTree->RootWidget = Root;

    // Icon
    IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("QuickIcon"));
    {
        auto* S = Root->AddChildToOverlay(IconImage);
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
        IconImage->SetColorAndOpacity(FLinearColor::White);
    }

    // Cooldown overlay
    CooldownOverlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("QuickCDOverlay"));
    {
        auto* S = Root->AddChildToOverlay(CooldownOverlay);
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
        CooldownOverlay->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
        CooldownOverlay->SetVisibility(ESlateVisibility::Hidden);
    }

    // Cooldown text center
    CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuickCDText"));
    {
        auto* S = Root->AddChildToOverlay(CooldownText);
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Center);
        CooldownText->SetVisibility(ESlateVisibility::Hidden);
    }

    // Qty text bottom-left
    QtyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuickQty"));
    {
        auto* S = Root->AddChildToOverlay(QtyText);
        S->SetHorizontalAlignment(HAlign_Left);
        S->SetVerticalAlignment(VAlign_Bottom);
        S->SetPadding(FMargin(6.f));
        QtyText->SetVisibility(ESlateVisibility::Hidden);
    }

    // 默认隐藏（没有快捷物品）
    SetVisibility(ESlateVisibility::Hidden);

    return Super::RebuildWidget();
}

void UQuickItemSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bIsCoolingDown) return;

    CooldownTimer -= InDeltaTime;
    if (CooldownTimer <= 0.f)
    {
        bIsCoolingDown = false;
        CooldownTimer = 0.f;
        if (CooldownOverlay) CooldownOverlay->SetVisibility(ESlateVisibility::Hidden);
        if (CooldownText)    CooldownText->SetVisibility(ESlateVisibility::Hidden);
    }
    else
    {
        if (CooldownText)
        {
            CooldownText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), CooldownTimer)));
        }
    }
}

void UQuickItemSlotWidget::SetQuickItem(const FItemStack* ItemOrNull)
{
    if (!ItemOrNull || !ItemOrNull->Icon || ItemOrNull->Quantity <= 0)
    {
        bHasItem = false;
        CurrentQty = 0;
        CurrentCooldown = 0.f;

        SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    bHasItem = true;
    CurrentQty = ItemOrNull->Quantity;
    CurrentCooldown = ItemOrNull->CooldownSeconds;

    if (IconImage) IconImage->SetBrushFromTexture(ItemOrNull->Icon, true);

    if (QtyText)
    {
        QtyText->SetText(FText::FromString(FString::FromInt(CurrentQty)));
        QtyText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    SetVisibility(ESlateVisibility::Visible);
}

void UQuickItemSlotWidget::StartCooldown(float Duration)
{
    if (Duration <= 0.f) return;
    bIsCoolingDown = true;
    CooldownTimer = Duration;

    if (CooldownOverlay) CooldownOverlay->SetVisibility(ESlateVisibility::Visible);
    if (CooldownText)    CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

bool UQuickItemSlotWidget::Use()
{
    if (!bHasItem) return false;
    if (bIsCoolingDown) return false;
    if (CurrentQty <= 0) return false;

    // 让外部去“扣背包数量/触发效果”
    OnUseRequested.ExecuteIfBound();

    // 自己进入冷却（外部扣完数量后会刷新 UI）
    StartCooldown(CurrentCooldown);
    return true;
}
