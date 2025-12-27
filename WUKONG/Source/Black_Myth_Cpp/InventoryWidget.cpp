#include "InventoryWidget.h"
#include "InventoryItemWidget.h"
#include "UQuickItemSlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Spacer.h"
#include "Components/Image.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"

static UTexture2D* LoadTexHard(const TCHAR* Path)
{
    return LoadObject<UTexture2D>(nullptr, Path);
}

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 初始为空：不加假数据
    RefreshItemList(EItemCategory::Medicine);
}

TSharedRef<SWidget> UInventoryWidget::RebuildWidget()
{
    UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    WidgetTree->RootWidget = RootOverlay;

    {
        UImage* BG = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        BG->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.5f));
        auto* S = RootOverlay->AddChildToOverlay(BG);
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
    }

    UHorizontalBox* MainLayout = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    {
        auto* S = RootOverlay->AddChildToOverlay(MainLayout);
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
        S->SetPadding(FMargin(10.f));
    }

    CategoryBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    MainLayout->AddChildToHorizontalBox(CategoryBox);

    CreateCategoryButton(TEXT("Medicine"), EItemCategory::Medicine);
    CreateCategoryButton(TEXT("Others"), EItemCategory::Other);

    USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
    Spacer->SetSize(FVector2D(20.f, 0.f));
    MainLayout->AddChildToHorizontalBox(Spacer);

    ItemGridBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
    ItemGridBox->SetInnerSlotPadding(FVector2D(24.f, 24.f));
    MainLayout->AddChildToHorizontalBox(ItemGridBox);

    return Super::RebuildWidget();
}

void UInventoryWidget::CreateCategoryButton(const FString& ButtonText, EItemCategory Category)
{
    UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(FText::FromString(ButtonText));
    Btn->AddChild(Text);

    if (Category == EItemCategory::Medicine)
    {
        Btn->OnClicked.AddDynamic(this, &UInventoryWidget::OnMedicineClicked);
    }
    else
    {
        Btn->OnClicked.AddDynamic(this, &UInventoryWidget::OnOtherClicked);
    }

    CategoryBox->AddChildToVerticalBox(Btn);
}


FItemStack UInventoryWidget::MakeItemTemplate(EItemType Type, const FString& Name) const
{
    FItemStack T;
    T.Type = Type;
    T.Name = Name;

    switch (Type)
    {
    case EItemType::HealthPotion:
        T.Category = EItemCategory::Medicine;
        T.CooldownSeconds = 3.f;
        
        T.Icon = LoadTexHard(TEXT("/Game/UI/maxblood"));
        break;

    case EItemType::ManaPotion:
        T.Category = EItemCategory::Medicine;
        T.CooldownSeconds = 3.f;
        T.Icon = LoadTexHard(TEXT("/Game/UI/maxlan"));
        break;

    default:
        T.Category = EItemCategory::Other;
        T.CooldownSeconds = 0.f;
        T.Icon = LoadTexHard(TEXT("/Game/UI/miniblood"));
        break;
    }

    return T;
}

void UInventoryWidget::AddItem(EItemType Type, const FString& Name, int32 Amount)
{
    if (Amount <= 0) return;

    const FItemStack Template = MakeItemTemplate(Type, Name);
    const FString Key = Template.StackKey();

    if (FItemStack* Existing = FindItemByKeyMutable(Key))
    {
        Existing->Quantity += Amount;
    }
    else
    {
        FItemStack NewItem = Template;
        NewItem.Quantity = Amount;
        InventoryData.Add(NewItem);
    }

    // 如果当前页就是它的分类，刷新右侧
    RefreshItemList(CurrentCategory);

    // 如果正在选中这个物品，同步快捷槽数量
    if (SelectedKey == Key && QuickSlot)
    {
        QuickSlot->SetQuickItem(FindItemByKey(SelectedKey));
    }
}

void UInventoryWidget::RefreshItemList(EItemCategory Category)
{
    if (!ItemGridBox) return;

    ItemGridBox->ClearChildren();

    for (const FItemStack& Item : InventoryData)
    {
        if (Item.Category != Category) continue;
        if (Item.Quantity <= 0) continue;

        UInventoryItemWidget* Cell = WidgetTree->ConstructWidget<UInventoryItemWidget>(
            UInventoryItemWidget::StaticClass()
        );

        const bool bSelected = (Item.StackKey() == SelectedKey);
        Cell->SetData(Item, bSelected);

        Cell->OnClicked.BindUObject(this, &UInventoryWidget::HandleItemClicked);

        // WrapBox child
        ItemGridBox->AddChildToWrapBox(Cell);

        Cell->SetCellSize(ItemCellSize);
        Cell->SetData(Item, bSelected);

    }
}

void UInventoryWidget::HandleItemClicked(const FString& Key)
{
    SelectedKey = Key;

    // 刷新当前分类页的选中高亮
    RefreshItemList(CurrentCategory);

    // 选中的是不是消耗品
    if (QuickSlot)
    {
        const FItemStack* Item = FindItemByKey(Key);
        if (Item && Item->IsConsumable())
        {
            QuickSlot->SetQuickItem(Item);

            // 绑定：快捷槽 Use() 时，回到背包扣数量
            QuickSlot->OnUseRequested.Unbind();
            QuickSlot->OnUseRequested.BindUObject(this, &UInventoryWidget::ConsumeSelectedOne);
        }
        else
        {
            // 非消耗品：不显示（留接口后续扩展）
            QuickSlot->SetQuickItem(nullptr);
            QuickSlot->OnUseRequested.Unbind();
        }
    }
}

const FItemStack* UInventoryWidget::FindItemByKey(const FString& Key) const
{
    for (const FItemStack& I : InventoryData)
    {
        if (I.StackKey() == Key) return &I;
    }
    return nullptr;
}

FItemStack* UInventoryWidget::FindItemByKeyMutable(const FString& Key)
{
    for (FItemStack& I : InventoryData)
    {
        if (I.StackKey() == Key) return &I;
    }
    return nullptr;
}

void UInventoryWidget::SetQuickSlot(UQuickItemSlotWidget* InQuickSlot)
{
    QuickSlot = InQuickSlot;
    if (!QuickSlot) return;

    // 如果已经选中了消耗品，立即同步一次
    const FItemStack* Item = FindItemByKey(SelectedKey);
    if (Item && Item->IsConsumable())
    {
        QuickSlot->SetQuickItem(Item);
        QuickSlot->OnUseRequested.Unbind();
        QuickSlot->OnUseRequested.BindUObject(this, &UInventoryWidget::ConsumeSelectedOne);
    }
    else
    {
        QuickSlot->SetQuickItem(nullptr);
    }
}

void UInventoryWidget::ConsumeSelectedOne()
{
    FItemStack* Item = FindItemByKeyMutable(SelectedKey);
    if (!Item) return;
    if (!Item->IsConsumable()) return;
    if (Item->Quantity <= 0) return;


    Item->Quantity -= 1;
    if (Item->Quantity < 0) Item->Quantity = 0;

    // 刷新右侧列表和快捷槽显示
    RefreshItemList(CurrentCategory);

    if (QuickSlot)
    {
        if (Item->Quantity > 0)
        {
            QuickSlot->SetQuickItem(Item);
        }
        else
        {
            QuickSlot->SetQuickItem(nullptr);
        }
    }
}

bool UInventoryWidget::UseSelectedQuickItem()
{
    if (!QuickSlot) return false;
    
    return QuickSlot->Use();
}

void UInventoryWidget::OnMedicineClicked()
{
    CurrentCategory = EItemCategory::Medicine;
    RefreshItemList(CurrentCategory);
}

void UInventoryWidget::OnOtherClicked()
{
    CurrentCategory = EItemCategory::Other;
    RefreshItemList(CurrentCategory);
}
EItemType UInventoryWidget::GetSelectedItemType() const
{
    if (SelectedKey.IsEmpty())
    {
        return EItemType::Other;
    }

    const FItemStack* Item = FindItemByKey(SelectedKey);
    return Item ? Item->Type : EItemType::Other;
}
