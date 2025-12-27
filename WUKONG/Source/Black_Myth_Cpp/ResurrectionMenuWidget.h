#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResurrectionMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

/**
 * 
 */
UCLASS()
class BLACK_MYTH_CPP_API UResurrectionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

protected:
	// 按钮点击回调
	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnMainMenuClicked();

private:
	// 辅助函数：创建一个带有文本的按钮
	UButton* CreateButtonWithText(const FString& ButtonText, UVerticalBox* ParentBox);

	// 保存按钮引用（可选，如果后续需要动态修改样式）
	UPROPERTY()
	TObjectPtr<UButton> RestartButton;

	UPROPERTY()
	TObjectPtr<UButton> MainMenuButton;
};
