#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundBase.h"
#include "MusicPlayerActor.generated.h"

class UAudioComponent;
class AWukongCharacter;
class APawn;

UCLASS(Blueprintable)
class BLACK_MYTH_CPP_API AMusicPlayerActor : public AActor
{
    GENERATED_BODY()

public:
    AMusicPlayerActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // ================= Audio =================
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
    UAudioComponent* AudioComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* WangLinMusic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* Music1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bAutoPlayOnBegin = true;

    // ================= Distance =================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    float DistanceThreshold = 500.0f;

    // ⭐ 关键：用 APawn，而不是 AMyPawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    ACharacter* TargetCubePawn = nullptr;

    // ================= Control =================
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlayMusic(USoundBase* TargetSound);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void StopMusic();

private:
    UPROPERTY()
    USoundBase* CurrentPlayingSound = nullptr;

    UFUNCTION()
    void OnMusicFinishedPlaying();

    AWukongCharacter* GetPlayerWukong();

    // ✅ 不再传 FVector
    void CheckDistanceAndSwitchMusic();
};
