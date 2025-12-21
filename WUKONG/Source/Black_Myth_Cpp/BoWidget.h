// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BloodWidget.h"
#include "BoWidget.generated.h"

/**
 *
 */
UCLASS()
class BLACK_MYTH_CPP_API UBoWidget : public UBloodWidget
{
    GENERATED_BODY()

public:
    // 用于构建 UI 结构的函数（替代蓝图编辑器里的拖拽操作）
    virtual TSharedRef<SWidget> RebuildWidget() override;

public:
    // 传递Boss的引用给血条Widget
    void SetBossReference(class ABoss* InBoss);

protected:
    // 每帧更新血条的显示状态
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    ABoss* BossActor;
    const float CircleRadius = 500.0f; // 圆形范围半径
};
