#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h"
#include "InventoryItemWidget.generated.h"

class UImage;
class UTextBlock;
class UOverlay;
class UButton;

DECLARE_DELEGATE_OneParam(FOnItemClicked, const FString& /*StackKey*/);

UCLASS()
class BLACK_MYTH_CPP_API UInventoryItemWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;

    void SetData(const FItemStack& Item, bool bSelected);
    FString GetKey() const { return Key; }

    FOnItemClicked OnClicked;

    void SetCellSize(FVector2D InSize) { CellSize = InSize; }

private:
    UFUNCTION()
    void HandleClicked();

private:
    UPROPERTY() UButton* ClickButton = nullptr;
    UPROPERTY() UImage* IconImage = nullptr;
    UPROPERTY() UImage* SelectedOverlay = nullptr;
    UPROPERTY() UTextBlock* QtyText = nullptr;

    FString Key;
    FVector2D CellSize = FVector2D(96.f, 96.f);
};

