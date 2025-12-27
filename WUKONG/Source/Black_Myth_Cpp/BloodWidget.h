// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BloodWidget.generated.h"

class UProgressBar;
class UCanvasPanel;

UCLASS()
class BLACK_MYTH_CPP_API UBloodWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    // 用于构建 UI 结构的函数
    virtual TSharedRef<SWidget> RebuildWidget() override;

    // 更新血量的接口
    void UpdateHealth(float Current, float Max);

protected:

    UPROPERTY()
    UProgressBar* HealthProgressBar;

    UPROPERTY()
    UCanvasPanel* RootCanvasPanel;
};