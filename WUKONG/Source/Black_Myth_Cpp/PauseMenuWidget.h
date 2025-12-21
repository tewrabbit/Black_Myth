#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

/**
 * 纯 C++ 实现的暂停菜单
 */
UCLASS()
class BLACK_MYTH_CPP_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化 Widget 树
	virtual void NativeOnInitialized() override;

protected:
	// 按钮点击回调
	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnQuitClicked();

private:
	// 辅助函数：创建一个带有文本的按钮
	UButton* CreateButtonWithText(const FString& ButtonText, UVerticalBox* ParentBox);

	// 保存按钮引用（可选，如果后续需要动态修改样式）
	UPROPERTY()
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY()
	TObjectPtr<UButton> QuitButton;
};