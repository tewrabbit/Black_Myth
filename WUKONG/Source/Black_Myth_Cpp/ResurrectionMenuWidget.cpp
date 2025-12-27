#include "ResurrectionMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "WukongCharacter.h"
#include "GameFramework/PlayerController.h"

void UResurrectionMenuWidget::NativeOnInitialized()
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

	// 4. 创建 "重新开始" 按钮
	RestartButton = CreateButtonWithText(TEXT("Restart"), VBox);
	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UResurrectionMenuWidget::OnRestartClicked);
	}

	// 5. 创建 "返回主界面" 按钮
	MainMenuButton = CreateButtonWithText(TEXT("Main Menu"), VBox);
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UResurrectionMenuWidget::OnMainMenuClicked);
	}
}

UButton* UResurrectionMenuWidget::CreateButtonWithText(const FString& ButtonText, UVerticalBox* ParentBox)
{
	if (!ParentBox) return nullptr;

	// 创建按钮
	UButton* NewBtn = WidgetTree->ConstructWidget<UButton>();

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

void UResurrectionMenuWidget::OnRestartClicked()
{
	// 1. 获取 PlayerController
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		// 2. 恢复玩家到出生点（通过传送玩家到原来的位置）
		APawn* Pawn = PC->GetPawn();
		if (Pawn)
		{
			const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true); // true: 去掉 PIE 前缀
			FVector RespawnLoc;

			if (LevelName == TEXT("LandscapeAutoMaterial_Desert_Example"))
			{
				RespawnLoc = FVector(-64411.f, -73131.f, 2900.f);
			}
			else if (LevelName == TEXT("LandscapeAutoMaterial_lsland_Example"))
			{
				RespawnLoc = FVector(18357.f, 14875.f, -2830.f);
			}
			else if (LevelName == TEXT("LandscapeAutoMaterial_MountainRange_Example"))
			{
				RespawnLoc = FVector(-23444.f, -2760.f, -3700.f);
			}
			else
			{
				// 默认地图/兜底
				RespawnLoc = FVector(0.f, 0.f, 100.f);
			}

			Pawn->SetActorLocation(RespawnLoc);

			AWukongCharacter* MyChar = Cast<AWukongCharacter>(Pawn);
			if (MyChar) { MyChar->setCurrentHealth(200.0f); MyChar->setbIsDead(false); }
		}

		// 注意：这里直接修改 bool 变量，不要用括号()调用
		PC->bShowMouseCursor = false;

		// 设置为“仅游戏”模式
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
	// 3. 关闭菜单
	RemoveFromParent();
}

void UResurrectionMenuWidget::OnMainMenuClicked()
{
	// 确保跳转前清理输入模式（防止鼠标卡在主菜单没法用）
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
	}
	// 1. 跳转回主菜单
	UGameplayStatics::OpenLevel(this, FName("MainMenu")); // 替换为实际的主菜单地图名称
}
