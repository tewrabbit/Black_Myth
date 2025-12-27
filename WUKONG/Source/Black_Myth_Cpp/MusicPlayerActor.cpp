#include "MusicPlayerActor.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "WukongCharacter.h"

AMusicPlayerActor::AMusicPlayerActor()
{
    PrimaryActorTick.bCanEverTick = true;

    AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
    RootComponent = AudioComponent;

    AudioComponent->bAutoActivate = false;
    CurrentPlayingSound = nullptr;
}

void AMusicPlayerActor::BeginPlay()
{
    Super::BeginPlay();

    // 校验音乐资源
    if (!Music1 || !WangLinMusic)
    {
        UE_LOG(LogTemp, Error, TEXT("MusicPlayerActor：音乐资源未设置"));
        PrimaryActorTick.bCanEverTick = false;
        return;
    }

    // 播放完成回调（循环）
    AudioComponent->OnAudioFinished.AddDynamic(
        this, &AMusicPlayerActor::OnMusicFinishedPlaying
    );

    // 开局立即根据 num 决定音乐
    if (bAutoPlayOnBegin)
    {
        CheckAndSwitchMusicByNum();
    }
}

void AMusicPlayerActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    CheckAndSwitchMusicByNum();
}

void AMusicPlayerActor::CheckAndSwitchMusicByNum()
{
    if (!AudioComponent)
    {
        return;
    }

    AWukongCharacter* Wukong = GetPlayerWukong();
    if (!Wukong)
    {
        return;
    }

    // =============================
    // ⭐ 核心：根据 num 判断音乐
    // =============================
    USoundBase* TargetSound = nullptr;

    if (Wukong->num == 0)
    {
        TargetSound = Music1;
    }
    else
    {
        TargetSound = WangLinMusic;
    }

    // 音乐发生变化才切换
    if (TargetSound && TargetSound != CurrentPlayingSound)
    {
        PlayMusic(TargetSound);
        CurrentPlayingSound = TargetSound;

        UE_LOG(LogTemp, Log,
            TEXT("切换音乐：num=%llu → %s"),
            Wukong->num,
            *TargetSound->GetName()
        );
    }
}

void AMusicPlayerActor::PlayMusic(USoundBase* TargetSound)
{
    if (!AudioComponent || !TargetSound)
    {
        return;
    }

    if (AudioComponent->IsPlaying())
    {
        AudioComponent->Stop();
    }

    AudioComponent->SetSound(TargetSound);
    AudioComponent->SetVolumeMultiplier(1.0f);
    AudioComponent->Play();
}

void AMusicPlayerActor::StopMusic()
{
    if (AudioComponent && AudioComponent->IsPlaying())
    {
        AudioComponent->Stop();
        CurrentPlayingSound = nullptr;
    }
}

void AMusicPlayerActor::OnMusicFinishedPlaying()
{
    // 循环播放当前音乐
    if (CurrentPlayingSound)
    {
        PlayMusic(CurrentPlayingSound);
    }
}

AWukongCharacter* AMusicPlayerActor::GetPlayerWukong()
{
    return Cast<AWukongCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0)
    );
}