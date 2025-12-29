
#include "FengMaoHealthBar.h"
#include "Components/ProgressBar.h"

void UFengMaoHealthBar::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateHealthBar(1.f, 1.f);
}

void UFengMaoHealthBar::UpdateHealthBar(float CurrentHealth, float MaxHealth)
{
	if (HealthBar && MaxHealth > 0.f)
	{
		float HealthPercent = CurrentHealth / MaxHealth;
		HealthBar->SetPercent(HealthPercent);

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