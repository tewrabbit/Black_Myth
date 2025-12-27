#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundBase.h"
#include "MusicPlayerActor.generated.h"

class UAudioComponent;
class AWukongCharacter;

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

    // 受伤后播放
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* WangLinMusic;

    // 正常状态播放
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* Music1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bAutoPlayOnBegin = true;

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

    // ⭐ 根据 num 判断并切歌
    void CheckAndSwitchMusicByNum();
};