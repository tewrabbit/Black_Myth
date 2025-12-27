#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UOverlay;
class UTexture2D;

USTRUCT()
struct FSkillSlotRuntime
{
	GENERATED_BODY()

	UPROPERTY() UImage* Icon = nullptr;
	UPROPERTY() UImage* CooldownOverlay = nullptr;
	UPROPERTY() UTextBlock* CooldownText = nullptr;

	bool  bIsCoolingDown = false;
	float CooldownTimer = 0.f;
	float CooldownDuration = 0.f;
};

UCLASS()
class BLACK_MYTH_CPP_API USkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USkillSlotWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// SlotIndex: 0 or 1
	UFUNCTION(BlueprintCallable)
	bool StartCooldown(int32 SlotIndex, float Duration);


	UFUNCTION(BlueprintCallable)
	void SetSkillIconTexture(int32 SlotIndex, UTexture2D* Texture);

protected:
	UPROPERTY() FSkillSlotRuntime Slots[2];

	// 构造函数里用路径加载的两张图
	UPROPERTY() UTexture2D* IconTextures[2] = { nullptr, nullptr };

	UPROPERTY(EditAnywhere, Category = "SkillSlot")
	FVector2D SlotSize = FVector2D(128.f, 128.f);

	UPROPERTY(EditAnywhere, Category = "SkillSlot")
	FMargin SlotPadding = FMargin(6.f, 0.f);
};
