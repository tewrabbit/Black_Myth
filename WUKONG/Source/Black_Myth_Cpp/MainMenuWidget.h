// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UImage;
class UButton;
class UVerticalBox;
class UTextBlock;

UCLASS()
class BLACK_MYTH_CPP_API UMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()


public:
    virtual TSharedRef<SWidget> RebuildWidget() override;

protected:
    UFUNCTION()
    void OnStartGameClicked();

    UFUNCTION()
    void OnSelectLevelClicked();

    UFUNCTION()
    void OnCreditsClicked();

    UFUNCTION()
    void OnQuitGameClicked();

private:
    // 参数：按钮文字，点击时绑定的函数
    UButton* CreateMenuButton(UVerticalBox* ParentBox, FString ButtonText, FName FunctionName);

    // 保存背景图指针
    UPROPERTY()
    UImage* BackgroundImage;

};
