// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"


UENUM()
enum class EItemCategory : uint8
{
    Medicine,
    Other
};

UENUM()
enum class EItemType : uint8
{
    HealthPotion,
    ManaPotion,
    Other
};

struct FItemStack
{
    EItemType Type = EItemType::Other;
    EItemCategory Category = EItemCategory::Other;

    FString Name;
    int32 Quantity = 0;

    // 图标（纯 C++ 直接存指针）
    TObjectPtr<class UTexture2D> Icon = nullptr;

    // 消耗品冷却（非消耗品可为 0）
    float CooldownSeconds = 0.f;

    // 堆叠判定 key：同 Type + 同 Name 视为同类物品
    FString StackKey() const
    {
        return FString::Printf(TEXT("%d|%s"), (int32)Type, *Name);
    }

    bool IsConsumable() const
    {
        return Type == EItemType::HealthPotion || Type == EItemType::ManaPotion;
    }
};

class BLACK_MYTH_CPP_API ItemTypes
{
public:
    ItemTypes();
    ~ItemTypes();
};
