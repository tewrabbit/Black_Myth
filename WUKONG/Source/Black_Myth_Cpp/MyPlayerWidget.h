// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BloodWidget.h"
#include "MyPlayerWidget.generated.h"

class USkillSlotWidget;
class UProgressBar;
class UInventoryWidget;
class UQuickItemSlotWidget;

UCLASS()
class BLACK_MYTH_CPP_API UMyPlayerWidget : public UBloodWidget
{
    GENERATED_BODY()

public:

    virtual TSharedRef<SWidget> RebuildWidget() override;

    // 更新蓝量
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateMana(float CurrentMana, float MaxMana);

    void TriggerSkillCooldown(float Duration);

    USkillSlotWidget* getSillSlot() { return SkillSlot; }

    UInventoryWidget* GetInventoryWidget() { return InventoryWidget; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    void UseQuickItem();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleInventory();

protected:
    // 蓝条组件 (不需要 BindWidget，因为是手动创建)
    UPROPERTY()
    class UProgressBar* ManaBar;

    UPROPERTY()
    USkillSlotWidget* SkillSlot;

    UPROPERTY()
    UInventoryWidget* InventoryWidget = nullptr;

    UPROPERTY()
    UQuickItemSlotWidget* QuickSlotWidget = nullptr;
};
