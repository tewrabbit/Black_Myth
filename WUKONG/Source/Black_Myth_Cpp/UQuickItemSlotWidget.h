#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h"
#include "UQuickItemSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UOverlay;

DECLARE_DELEGATE(FOnQuickUseRequested);

UCLASS()
class BLACK_MYTH_CPP_API UQuickItemSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // 外部：设置当前快捷物品（为空则隐藏）
    void SetQuickItem(const FItemStack* ItemOrNull);

    // 外部：请求使用（由外部绑定真正的“扣数量”逻辑）
    void Use();

    // 让外部绑定：当 Use() 被调用时，外部去扣背包数量
    FOnQuickUseRequested OnUseRequested;

private:
    void StartCooldown(float Duration);

private:
    UPROPERTY() UImage* IconImage = nullptr;
    UPROPERTY() UImage* CooldownOverlay = nullptr;
    UPROPERTY() UTextBlock* CooldownText = nullptr;
    UPROPERTY() UTextBlock* QtyText = nullptr;

    bool bIsCoolingDown = false;
    float CooldownTimer = 0.f;

    // 当前显示信息
    bool bHasItem = false;
    int32 CurrentQty = 0;
    float CurrentCooldown = 0.f;
};
