#include "MyCameraModifier.h"

bool UMyCameraModifier::ModifyCamera(
    float DeltaTime,
    FMinimalViewInfo& InOutPOV
)
{
    if (TimeRemaining <= 0.f)
    {
        return false;
    }

    TimeRemaining -= DeltaTime;

    float LocalAlpha = TimeRemaining * Strength;

    // 简单随机抖动（第三人称非常明显）
    InOutPOV.Rotation.Pitch += FMath::FRandRange(-1.f, 1.f) * LocalAlpha;
    InOutPOV.Rotation.Yaw += FMath::FRandRange(-1.f, 1.f) * LocalAlpha;

    return false;
}

void UMyCameraModifier::StartShake(float InDuration, float InStrength)
{
    TimeRemaining = InDuration;
    Strength = InStrength;
}
