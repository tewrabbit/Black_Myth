#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h"
#include "InventoryWidget.generated.h"

class UWrapBox;
class UVerticalBox;
class UQuickItemSlotWidget;

UCLASS()
class BLACK_MYTH_CPP_API UInventoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

    // ✅ 你要的接口：添加物品（自动分区、堆叠）
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(EItemType Type, const FString& Name, int32 Amount);

    // 外部把右下角快捷槽注入进来（HUD 创建后调用一次）
    void SetQuickSlot(UQuickItemSlotWidget* InQuickSlot);

    // 外部调用：使用当前选中的快捷物品（例如按键触发）
    void UseSelectedQuickItem();

    UFUNCTION()
    void OnMedicineClicked();

    UFUNCTION()
    void OnOtherClicked();



protected:
    UPROPERTY() UWrapBox* ItemGridBox = nullptr;
    UPROPERTY() UVerticalBox* CategoryBox = nullptr;

private:
    // 数据
    TArray<FItemStack> InventoryData;
    EItemCategory CurrentCategory = EItemCategory::Medicine;

    // 选中
    FString SelectedKey;

    // 右下角快捷槽
    UPROPERTY() UQuickItemSlotWidget* QuickSlot = nullptr;

private:
    void RefreshItemList(EItemCategory Category);
    void CreateCategoryButton(const FString& ButtonText, EItemCategory Category);

    void HandleItemClicked(const FString& Key);
    const FItemStack* FindItemByKey(const FString& Key) const;
    FItemStack* FindItemByKeyMutable(const FString& Key);

    // 根据类型决定：分类/图标/冷却（你可以在这里扩展更多物品）
    FItemStack MakeItemTemplate(EItemType Type, const FString& Name) const;

    // 快捷槽请求使用时：真正扣数量 + 刷新 UI
    void ConsumeSelectedOne();

    FVector2D ItemCellSize = FVector2D(128.f, 128.f);

};
