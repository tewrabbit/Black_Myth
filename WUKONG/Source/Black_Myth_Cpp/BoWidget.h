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
	void SetBossReference(class AParagonGideon* InBoss);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	AParagonGideon* BossActor;
	const float CircleRadius = 1000.0f;
};
