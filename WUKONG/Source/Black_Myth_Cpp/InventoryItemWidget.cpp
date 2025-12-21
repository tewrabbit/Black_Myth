#include "InventoryItemWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

TSharedRef<SWidget> UInventoryItemWidget::RebuildWidget()
{
    // 用 SizeBox 包住整个格子，点击更自然
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ItemSizeBox"));
    SizeBox->SetWidthOverride(128.f);  // 设置格子的宽度
    SizeBox->SetHeightOverride(128.f); // 设置格子的高度

    // 用 Button 包住整个格子
    ClickButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ItemBtn"));
    SizeBox->AddChild(ClickButton);

    // 将 SizeBox 设置为根控件
    WidgetTree->RootWidget = SizeBox;

    // 创建 Overlay
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ItemOverlay"));
    ClickButton->AddChild(Root);

    // Icon
    IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIcon"));
    {
        auto* S = Root->AddChildToOverlay(IconImage);
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
        IconImage->SetColorAndOpacity(FLinearColor::White);
    }

    // Selected overlay
    SelectedOverlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedOverlay"));
    {
        auto* S = Root->AddChildToOverlay(SelectedOverlay);
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
        SelectedOverlay->SetColorAndOpacity(FLinearColor(0.2f, 0.7f, 1.f, 0.25f));
        SelectedOverlay->SetVisibility(ESlateVisibility::Hidden);
    }

    // Qty text bottom-left
    QtyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemQty"));
    {
        auto* S = Root->AddChildToOverlay(QtyText);
        S->SetHorizontalAlignment(HAlign_Left);
        S->SetVerticalAlignment(VAlign_Bottom);
        S->SetPadding(FMargin(6.f));
    }

    // Bind click
    if (ClickButton)
    {
        ClickButton->OnClicked.AddDynamic(this, &UInventoryItemWidget::HandleClicked);
    }

    return Super::RebuildWidget();
}

void UInventoryItemWidget::SetData(const FItemStack& Item, bool bSelected)
{
    Key = Item.StackKey();

    if (IconImage && Item.Icon)
    {
        IconImage->SetBrushFromTexture(Item.Icon, true);

        FSlateBrush B = IconImage->GetBrush();
        B.ImageSize = CellSize;
        IconImage->SetBrush(B);
    }

    if (QtyText)
    {
        QtyText->SetText(FText::FromString(FString::FromInt(Item.Quantity)));
    }

    if (SelectedOverlay)
    {
        SelectedOverlay->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
}

void UInventoryItemWidget::HandleClicked()
{
    OnClicked.ExecuteIfBound(Key);
}
