#include "SkillSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"

#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"

static UOverlay* BuildOneSlot(UWidgetTree* WidgetTree, FSkillSlotRuntime& OutSlot)
{
	UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	// 1) Icon
	OutSlot.Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (OutSlot.Icon)
	{
		UOverlaySlot* IconSlot = SlotOverlay->AddChildToOverlay(OutSlot.Icon);
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);

		// 不染色，显示原贴图
		OutSlot.Icon->SetColorAndOpacity(FLinearColor::White);
	}

	// 2) Cooldown Overlay
	OutSlot.CooldownOverlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (OutSlot.CooldownOverlay)
	{
		UOverlaySlot* OverlaySlot = SlotOverlay->AddChildToOverlay(OutSlot.CooldownOverlay);
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);

		OutSlot.CooldownOverlay->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
		OutSlot.CooldownOverlay->SetVisibility(ESlateVisibility::Hidden);
	}

	// 3) Cooldown Text
	OutSlot.CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (OutSlot.CooldownText)
	{
		UOverlaySlot* TextSlot = SlotOverlay->AddChildToOverlay(OutSlot.CooldownText);
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);

		OutSlot.CooldownText->SetVisibility(ESlateVisibility::Hidden);
	}

	return SlotOverlay;
}

USkillSlotWidget::USkillSlotWidget(const FObjectInitializer& ObjectInitializer)
	: UUserWidget(ObjectInitializer)
{


	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> T0(
			TEXT("/Game/UI/skill1")
		);
		if (T0.Succeeded())
		{
			IconTextures[0] = T0.Object;
		}
	}

	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> T1(
			TEXT("/Game/UI/skill2")
		);
		if (T1.Succeeded())
		{
			IconTextures[1] = T1.Object;
		}
	}
}

TSharedRef<SWidget> USkillSlotWidget::RebuildWidget()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SkillRow"));
	if (Row)
	{
		UOverlaySlot* RowSlot = RootOverlay->AddChildToOverlay(Row);
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}

	for (int32 i = 0; i < 2; ++i)
	{
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SizeBox->SetWidthOverride(SlotSize.X);
		SizeBox->SetHeightOverride(SlotSize.Y);

		UOverlay* SlotOverlay = BuildOneSlot(WidgetTree, Slots[i]);
		SizeBox->AddChild(SlotOverlay);

		// 把构造函数加载的贴图刷上去
		if (Slots[i].Icon && IconTextures[i])
		{
			Slots[i].Icon->SetBrushFromTexture(IconTextures[i], true);
			Slots[i].Icon->SetColorAndOpacity(FLinearColor::White);
		}

		if (Row)
		{
			UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SizeBox);
			HSlot->SetPadding(SlotPadding);
			HSlot->SetHorizontalAlignment(HAlign_Center);
			HSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	return Super::RebuildWidget();
}

void USkillSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	for (int32 i = 0; i < 2; ++i)
	{
		FSkillSlotRuntime& S = Slots[i];
		if (!S.bIsCoolingDown) continue;

		S.CooldownTimer -= InDeltaTime;

		if (S.CooldownTimer <= 0.f)
		{
			S.bIsCoolingDown = false;
			S.CooldownTimer = 0.f;
			S.CooldownDuration = 0.f;

			if (S.CooldownOverlay) S.CooldownOverlay->SetVisibility(ESlateVisibility::Hidden);
			if (S.CooldownText)    S.CooldownText->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			if (S.CooldownText)
			{
				S.CooldownText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), S.CooldownTimer)));
			}
		}
	}
}

bool USkillSlotWidget::StartCooldown(int32 SlotIndex, float Duration)
{
	if (SlotIndex < 0 || SlotIndex >= 2) 
		return false;
	if (Duration <= 0.f) 
		return false;

	FSkillSlotRuntime& S = Slots[SlotIndex];

	if (S.bIsCoolingDown)
	{
		// 返回 false，表示技能仍在冷却中
		return false;
	}

	S.bIsCoolingDown = true;
	S.CooldownTimer = Duration;
	S.CooldownDuration = Duration;

	if (S.CooldownOverlay) S.CooldownOverlay->SetVisibility(ESlateVisibility::Visible);
	if (S.CooldownText)    S.CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
	return true;
}

void USkillSlotWidget::SetSkillIconTexture(int32 SlotIndex, UTexture2D* Texture)
{
	if (SlotIndex < 0 || SlotIndex >= 2) return;
	if (!Texture) return;

	IconTextures[SlotIndex] = Texture;

	if (Slots[SlotIndex].Icon)
	{
		Slots[SlotIndex].Icon->SetBrushFromTexture(Texture, true);
		Slots[SlotIndex].Icon->SetColorAndOpacity(FLinearColor::White);
	}
}
