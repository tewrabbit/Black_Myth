// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FengMaoHealthBar.generated.h"

/**
 * Health bar widget for FengMao enemy
 */
UCLASS()
class WUKONGPROJECT_API UFengMaoHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	// 更新血条
	UFUNCTION(BlueprintCallable, Category = "Health")
	void UpdateHealthBar(float CurrentHealth, float MaxHealth);

protected:
	virtual void NativeConstruct() override;

private:
	// 血条进度条 (需要在UMG中设置)
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;
};