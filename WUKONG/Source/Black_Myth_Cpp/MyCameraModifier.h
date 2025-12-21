#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "MyCameraModifier.generated.h"

UCLASS()
class BLACK_MYTH_CPP_API UMyCameraModifier : public UCameraModifier
{
    GENERATED_BODY()

public:
    virtual bool ModifyCamera(
        float DeltaTime,
        FMinimalViewInfo& InOutPOV
    ) override;

    void StartShake(float InDuration, float InStrength);

private:
    float TimeRemaining = 0.f;
    float Strength = 0.f;
};
