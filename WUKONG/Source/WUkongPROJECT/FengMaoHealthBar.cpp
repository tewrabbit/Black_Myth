// Fill out your copyright notice in the Description page of Project Settings.

#include "FengMaoHealthBar.h"
#include "Components/ProgressBar.h"

void UFengMaoHealthBar::NativeConstruct()
{
	Super::NativeConstruct();

	// 初始化血条
	UpdateHealthBar(1.f, 1.f);
}

void UFengMaoHealthBar::UpdateHealthBar(float CurrentHealth, float MaxHealth)
{
	if (HealthBar && MaxHealth > 0.f)
	{
		float HealthPercent = CurrentHealth / MaxHealth;
		HealthBar->SetPercent(HealthPercent);

		// 根据血量设置颜色
		if (HealthPercent > 0.6f)
		{
			HealthBar->SetFillColorAndOpacity(FLinearColor::Green);
		}
		else if (HealthPercent > 0.3f)
		{
			HealthBar->SetFillColorAndOpacity(FLinearColor::Yellow);
		}
		else
		{
			HealthBar->SetFillColorAndOpacity(FLinearColor::Red);
		}
	}
}