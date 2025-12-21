#include "PauseMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"

void UPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 1. 创建根节点 CanvasPanel (允许绝对定位)
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas; // 重要：设置 WidgetTree 的根

	// 2. 创建垂直盒子 (用来排列按钮)
	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));

	// 3. 将垂直盒子添加到 Canvas 中
	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(VBox);
	if (CanvasSlot)
	{
		// 设置锚点为屏幕中心
		CanvasSlot->SetAnchors(FAnchors(0.5f));
		// 设置对齐为中心 (0.5, 0.5)，这样 UI 就会严格居中
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		// 让盒子根据内容自动调整大小
		CanvasSlot->SetAutoSize(true);
	}

	// 4. 创建 "继续游戏" 按钮
	ResumeButton = CreateButtonWithText(TEXT("Resume"), VBox);
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	}

	// 添加一点间距 (Spacer) 或者仅仅依靠按钮自身的 Padding，这里简单处理，直接加第二个按钮

	// 5. 创建 "结束游戏" 按钮
	QuitButton = CreateButtonWithText(TEXT("Quit"), VBox);
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnQuitClicked);
	}
}

UButton* UPauseMenuWidget::CreateButtonWithText(const FString& ButtonText, UVerticalBox* ParentBox)
{
	if (!ParentBox) return nullptr;

	// 创建按钮
	UButton* NewBtn = WidgetTree->ConstructWidget<UButton>();

	// 设置按钮样式 (可选，这里用默认样式，稍微加点 Padding)
	// NewBtn->WidgetStyle... 

	// 创建文本
	UTextBlock* NewTxt = WidgetTree->ConstructWidget<UTextBlock>();
	NewTxt->SetText(FText::FromString(ButtonText));
	NewTxt->SetJustification(ETextJustify::Center);

	// 将文本设为按钮的子控件
	NewBtn->AddChild(NewTxt);

	// 将按钮添加到垂直盒子
	UVerticalBoxSlot* BoxSlot = ParentBox->AddChildToVerticalBox(NewBtn);
	if (BoxSlot)
	{
		BoxSlot->SetPadding(FMargin(0.f, 10.f)); // 上下间距
		BoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
	}

	return NewBtn;
}

void UPauseMenuWidget::OnResumeClicked()
{
	// 获取 PlayerController
	if (APlayerController* PC = GetOwningPlayer())
	{
		// 1. 移除 UI
		RemoveFromParent();

		// 2. 取消暂停
		UGameplayStatics::SetGamePaused(this, false);

		// 3. 恢复输入模式为游戏
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void UPauseMenuWidget::OnQuitClicked()
{
	// 简单实现：跳转回主菜单 Level
	// 注意：你需要将 "MainMenuMap" 替换为你实际的主菜单地图名称
	UGameplayStatics::OpenLevel(this, FName("MainMenu"));
}