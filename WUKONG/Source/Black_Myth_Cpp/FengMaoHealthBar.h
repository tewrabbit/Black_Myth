// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FengMaoHealthBar.generated.h"

UCLASS()
class BLACK_MYTH_CPP_API UFengMaoHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Health")
	void UpdateHealthBar(float CurrentHealth, float MaxHealth);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;
};