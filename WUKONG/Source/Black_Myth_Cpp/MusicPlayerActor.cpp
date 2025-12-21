#include "MusicPlayerActor.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Pawn.h"
#include "WukongCharacter.h"

AMusicPlayerActor::AMusicPlayerActor()
{
    PrimaryActorTick.bCanEverTick = true;

    AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
    RootComponent = AudioComponent;

    AudioComponent->bAutoActivate = false;

    CurrentPlayingSound = nullptr;
    TargetCubePawn = nullptr;
}

void AMusicPlayerActor::BeginPlay()
{
    Super::BeginPlay();

    // =============================
    // 1. 只在指定关卡生效（可删）
    // =============================

    FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
    UE_LOG(LogTemp, Log, TEXT("MusicPlayerActor 当前关卡：%s"), *LevelName);

    if (!LevelName.Contains(TEXT("LandscapeAutoMaterial_MountainRange_Example"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Log, TEXT("非目标关卡，关闭 Tick"));
        PrimaryActorTick.bCanEverTick = false;
        return;
    }

    // =============================
    // 2. 若编辑器未拖 TargetCubePawn，则自动查找
    // =============================
    if (!TargetCubePawn)
    {
        TArray<AActor*> Pawns;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Pawns);

        for (AActor* Actor : Pawns)
        {
            if (Actor && Actor->GetName().Contains(TEXT("ParagonFengMao")))
            {
                TargetCubePawn = Cast<ACharacter>(Actor);
                UE_LOG(LogTemp, Log, TEXT("自动找到目标方块 Pawn：%s"), *Actor->GetName());
                break;
            }
        }
    }

    if (!TargetCubePawn)
    {
        UE_LOG(LogTemp, Error, TEXT("未找到目标方块 Pawn，音乐距离检测将失效"));
        PrimaryActorTick.bCanEverTick = false;
        return;
    }

    // =============================
    // 3. 校验音乐资源
    // =============================
    if (!WangLinMusic || !Music1)
    {
        UE_LOG(LogTemp, Error, TEXT("音乐资源未设置（WangLinMusic / Music1）"));
        PrimaryActorTick.bCanEverTick = false;
        return;
    }

    // =============================
    // 4. 绑定播放完成回调（伪循环）
    // =============================
    AudioComponent->OnAudioFinished.AddDynamic(
        this, &AMusicPlayerActor::OnMusicFinishedPlaying);

    // =============================
    // 5. 开局立即检测一次
    // =============================
    if (bAutoPlayOnBegin)
    {
        CheckDistanceAndSwitchMusic();
    }
}

void AMusicPlayerActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    CheckDistanceAndSwitchMusic();
}

void AMusicPlayerActor::CheckDistanceAndSwitchMusic()
{
    if (!AudioComponent || !TargetCubePawn)
    {
        return;
    }

    AWukongCharacter* Wukong = GetPlayerWukong();
    if (!Wukong)
    {
        return;
    }

    // =============================
    // 距离计算（正确用法）
    // =============================
    float Distance = FVector::Distance(
        Wukong->GetActorLocation(),
        TargetCubePawn->GetActorLocation()
    );

    // =============================
    // 根据距离决定音乐
    // =============================
    USoundBase* TargetSound =
        (Distance <= DistanceThreshold) ? WangLinMusic : Music1;

    if (TargetSound && TargetSound != CurrentPlayingSound)
    {
        PlayMusic(TargetSound);
        CurrentPlayingSound = TargetSound;
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

    UE_LOG(LogTemp, Log, TEXT("开始播放音乐：%s"), *TargetSound->GetName());
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
    // 简单循环播放当前音乐
    if (CurrentPlayingSound)
    {
        PlayMusic(CurrentPlayingSound);
    }
}

AWukongCharacter* AMusicPlayerActor::GetPlayerWukong()
{
    return Cast<AWukongCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0));
}
