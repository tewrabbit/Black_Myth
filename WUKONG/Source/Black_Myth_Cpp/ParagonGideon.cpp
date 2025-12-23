// ParagonGideon.cpp
#include "ParagonGideon.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Particles/ParticleSystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"
// 包含小怪头文件
#include "ParagonNarbash.h"
#include "ParagonRampage.h"

AParagonGideon::AParagonGideon()
{
    // 设置Boss默认值，继承自ParagonFengMao但具有Boss特性

    // 高血量设计（符合Boss定位）
    MaxHealth = 800.f;  // 远高于普通敌人
    CurrentHealth = MaxHealth;

    // 攻击力随阶段提升（设置近战攻击伤害为4）
    MeleeAttackDamage = 4.f;  // 近战攻击伤害
    SkillDamage = 5.f;   // 仅降低特效伤害

    // 调整索敌和攻击范围
    DetectionRange = 1500.f;  // 索敌范围
    AttackRange = 600.f;      // 攻击范围

    // 移动速度适中（Boss不应过于敏捷）
    PatrolSpeed = 280.f;
    ChaseSpeed = 450.f;

    // 阶段阈值设置
    Phase2Threshold = 0.7f; // 70%血量时进入二阶段
    Phase3Threshold = 0.3f; // 30%血量时进入三阶段

    // 特殊技能参数
    SpecialAbilityCooldown = 15.f; // 特殊技能15秒冷却
    ShieldDuration = 10.f;         // 护盾持续10秒
    MinionsToSummon = 3;           // 每次召唤3个小怪

    // 初始化状态
    bHasShield = false;
    bIsEnraged = false;

    // 初始化Gideon专用攻击冷却时间（普通模式下更长，狂暴模式下为2.0秒）
    GideonAttackCooldown = 2.5f;  // 普通模式攻击冷却时间（加快0.5秒）

    // 初始化上次攻击时间
    LastGideonAttackTime = 0.0f;

    // 初始化动画同步参数
    AttackEffectDelay = 0.5f;  // 攻击特效延迟时间
    DamageDelay = 0.5f;        // 伤害延迟时间

    // 调整特效生成延迟，使其与攻击冷却同步并下降0.5s
    float EffectSpawnDelay = 0.4f;  // 特效生成延迟时间（最大延迟时间）

    // 初始化伤害间隔控制
    LastEffectDamageTime = 0.0f;  // 上次特效伤害时间
    EffectDamageInterval = 0.3f;  // 特效伤害间隔（每0.3秒造成一次伤害）

    // 初始化持续伤害控制
    MaxContinuousDamageTicks = 15;  // 最大持续伤害次数（2.25秒/0.15秒间隔）
    CurrentContinuousDamageTicks = 0;

    // 调整持续伤害间隔，使其同步并下降约0.5s
    float ContinuousDamageInterval = 0.15f;  // 持续伤害间隔时间

    // 加载Gideon特有的动画资源
    static ConstructorHelpers::FObjectFinder<UAnimMontage> GideonAttackAMontageObj(
        TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Primary_Attack_A_Medium_Montage")
    );
    if (GideonAttackAMontageObj.Succeeded())
    {
        AttackMontage = GideonAttackAMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载普通攻击动画成功: %s"), *GideonAttackAMontageObj.Object->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载普通攻击动画失败！"));
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> GideonAttackBMontageObj(
        TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Primary_Attack_B_Medium_Montage")
    );
    if (GideonAttackBMontageObj.Succeeded())
    {
        HeavyAttackMontage = GideonAttackBMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载重击动画成功: %s"), *GideonAttackBMontageObj.Object->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载重击动画失败！"));
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> GideonAttackCMontageObj(
        TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Primary_Attack_C_Medium_Montage")
    );
    if (GideonAttackCMontageObj.Succeeded())
    {
        SkillMontage = GideonAttackCMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载技能动画成功: %s"), *GideonAttackCMontageObj.Object->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载技能动画失败！"));
    }

    // 加载FXVarietyPack特效资源
    static ConstructorHelpers::FClassFinder<AActor> FireEffectObj(
        TEXT("/Game/FXVarietyPack/Blueprints/BP_ky_fireStorm")
    );
    if (FireEffectObj.Succeeded())
    {
        FireSkillEffectClass = FireEffectObj.Class;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载火焰特效成功: %s"), *FireEffectObj.Class->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载火焰特效失败！"));
    }

    // 加载胸前特效资源
    static ConstructorHelpers::FClassFinder<AActor> ChestEffectObj(
        TEXT("/Game/FXVarietyPack/Blueprints/BP_ky_thunderBall")
    );
    if (ChestEffectObj.Succeeded())
    {
        ChestEffectClass = ChestEffectObj.Class;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载胸前特效成功: %s"), *ChestEffectObj.Class->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载胸前特效失败！"));
    }

    static ConstructorHelpers::FClassFinder<AActor> IceEffectObj(
        TEXT("/Game/FXVarietyPack/Blueprints/BP_ky_aquaStorm")
    );
    if (IceEffectObj.Succeeded())
    {
        IceSkillEffectClass = IceEffectObj.Class;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载冰霜特效成功: %s"), *IceEffectObj.Class->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载冰霜特效失败！"));
    }

    static ConstructorHelpers::FClassFinder<AActor> DarkEffectObj(
        TEXT("/Game/FXVarietyPack/Blueprints/BP_ky_darkStorm")
    );
    if (DarkEffectObj.Succeeded())
    {
        DarkSkillEffectClass = DarkEffectObj.Class;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载黑暗特效成功: %s"), *DarkEffectObj.Class->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载黑暗特效失败！"));
    }

    // 加载小怪蓝图资源
    static ConstructorHelpers::FClassFinder<AParagonFengMao> FengMaoMinionObj(
        TEXT("/Game/ParagonFengMao/Characters/Heroes/FengMao/FengMaoPlayerCharacter")
    );
    if (FengMaoMinionObj.Succeeded())
    {
        FengMaoMinionClass = FengMaoMinionObj.Class;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载FengMao小怪蓝图成功: %s"), *FengMaoMinionObj.Class->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载FengMao小怪蓝图失败！"));
    }

    static ConstructorHelpers::FClassFinder<AParagonFengMao> NarbashMinionObj(
        TEXT("/Game/ParagonNarbash/Characters/Heroes/Narbash/NarbashPlayerCharacter")
    );
    if (NarbashMinionObj.Succeeded())
    {
        NarbashMinionClass = NarbashMinionObj.Class;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载Narbash小怪蓝图成功: %s"), *NarbashMinionObj.Class->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载Narbash小怪蓝图失败！"));
    }

    static ConstructorHelpers::FClassFinder<AParagonFengMao> RampageMinionObj(
        TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/RampagePlayerCharacter")
    );
    if (RampageMinionObj.Succeeded())
    {
        RampageMinionClass = RampageMinionObj.Class;
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon加载Rampage小怪蓝图成功: %s"), *RampageMinionObj.Class->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon加载Rampage小怪蓝图失败！"));
    }

    // 注意：死亡动画资源将在蓝图中设置，这里保持父类的默认加载逻辑七位数z
}

void AParagonGideon::BeginPlay()
{
    // 调用父类BeginPlay
    Super::BeginPlay();

    // 初始化Boss阶段
    CurrentPhase = EBossPhase::Phase1;

    // 初始化状态标志
    bHasShield = false;
    bIsEnraged = false;

    // 重置攻击时间
    LastGideonAttackTime = 0.0f;

    // 重置特效伤害时间
    LastEffectDamageTime = 0.0f;

    // 重置持续伤害控制
    CurrentContinuousDamageTicks = 0;

    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon Boss初始化完成，进入第一阶段"));

    // 添加调试信息，确认我们的C++类被正确调用
    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon C++类已正确初始化，类名: %s"), *this->GetClass()->GetName());
}
void AParagonGideon::Tick(float DeltaTime)
{


    // 调用父类Tick
    Super::Tick(DeltaTime);

    // 定期检查阶段转换
    CheckPhaseTransition();
}

void AParagonGideon::CheckPhaseTransition()
{
    // 防止重复转换
    if (bIsDead) return;

    float HealthPercentage = CurrentHealth / MaxHealth;

    // 检查是否进入三阶段
    if (HealthPercentage <= Phase3Threshold && CurrentPhase != EBossPhase::Phase3)
    {
        TransitionToPhase(EBossPhase::Phase3);
    }
    // 检查是否进入二阶段
    else if (HealthPercentage <= Phase2Threshold && CurrentPhase == EBossPhase::Phase1)
    {
        TransitionToPhase(EBossPhase::Phase2);
    }
}

void AParagonGideon::TransitionToPhase(EBossPhase NewPhase)
{
    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon Boss阶段转换：%d -> %d"), static_cast<int32>(CurrentPhase), static_cast<int32>(NewPhase));

    CurrentPhase = NewPhase;

    switch (CurrentPhase)
    {
    case EBossPhase::Phase2:
        // 二阶段属性提升
        MeleeAttackDamage = 7.f;     // 近战攻击力提升
        ChaseSpeed *= 1.2f;      // 移动速度提升20%

        // 使用特殊技能
        UseSpecialAbility();

        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon进入二阶段！属性全面提升"));
        break;

    case EBossPhase::Phase3:
        // 三阶段狂暴化
        MeleeAttackDamage = 10.f;     // 近战攻击力大幅提升
        ChaseSpeed *= 1.2f;      // 移动速度提升20%

        // 进入狂暴状态
        EnterEnrageMode();

        // 使用特殊技能（召唤小怪）
        UseSpecialAbility();

        UE_LOG(LogTemp, Warning, TEXT("👹👹 Gideon进入三阶段！狂暴状态！"));
        break;
    }
}

void AParagonGideon::AttackPlayer()
{
    // 记录攻击行为
    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon执行攻击行为，当前阶段: %d"), static_cast<int32>(CurrentPhase));

    // 调用父类的攻击逻辑，但我们需要完全重写这个函数
    // 检查目标和死亡状态
    if (!TargetPlayer || bIsDead)
    {
        SetAIState(EFengMaoAIState::Patrol);
        return;
    }

    float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());

    // 动态距离管理：根据距离调整行为
    if (Distance > AttackRange * 2.0f)
    {
        // 玩家距离过远，切换到追逐
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon玩家远离 (%.1f米)，切换到追逐"), Distance);
        SetAIState(EFengMaoAIState::Chase);
        return;
    }
    else if (Distance < AttackRange * 0.3f)
    {
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon玩家过近，紧急后退"));
        // 玩家距离过近，后退调整
        if (GetCharacterMovement())
        {
            // 设置紧急后退速度
            GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
            FVector BackwardDirection = -GetActorForwardVector();
            AddMovementInput(BackwardDirection, 1.0f);
        }
    }

    // 面向玩家
    FaceTarget(TargetPlayer, GetWorld()->GetDeltaSeconds());

    // 保持移动能力（攻击时不完全停止移动）
    // 根据距离调整移动策略
    // 保持移动能力（攻击时不完全停止移动）
    // 根据距离调整移动策略
    if (GetCharacterMovement())
    {
        // 设置攻击时的移动速度
        GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed * 0.8f; // 攻击时稍慢一些

        FVector MoveDirection = FVector::ZeroVector;
        bool bShouldMove = true;

        // 如果距离过近，后退调整位置
        if (Distance < AttackRange * 0.6f)
        {
            UE_LOG(LogTemp, Warning, TEXT("👹 Gideon距离过近，后退调整"));
            // 后退调整
            MoveDirection = -GetActorForwardVector();
        }
        // 如果距离适中，侧向移动
        else if (Distance < AttackRange * 1.2f)
        {
            UE_LOG(LogTemp, Warning, TEXT("👹 Gideon距离适中，侧向移动"));
            // 50%概率向左或向右移动
            MoveDirection = (FMath::RandBool() ? GetActorRightVector() : -GetActorRightVector());
        }
        // 如果距离较远，前进接近
        else if (Distance > AttackRange * 1.5f)
        {
            UE_LOG(LogTemp, Warning, TEXT("👹 Gideon距离较远，前进接近"));
            // 前进接近
            MoveDirection = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        }
        else
        {
            // 否则保持当前位置
            bShouldMove = false;
        }

        // 执行移动
        if (bShouldMove)
        {
            AddMovementInput(MoveDirection, 1.0f);
        }
    }

    // 检查攻击冷却（使用成员变量来跟踪）
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float TimeSinceLastAttack = CurrentTime - LastGideonAttackTime;

    // 10%概率进行随机移动（增加行为多样性）
    static float LastRandomMoveTime = 0.f;

    if (FMath::FRand() < 0.1f && CurrentTime - LastRandomMoveTime >= 0.2f) // 添加轻微冷却时间以减少抽搐
    {
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon执行随机移动"));
        LastRandomMoveTime = CurrentTime;

        // 随机选择移动方向
        FVector RandomDirection = FVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), 0.f).GetSafeNormal();

        if (GetCharacterMovement())
        {
            // 设置随机移动速度
            GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed * 0.7f;
            AddMovementInput(RandomDirection, 1.0f);
        }
    }

    // 5%概率进行环绕移动（围绕玩家）
    static float LastCircleMoveTime = 0.f;

    if (FMath::FRand() < 0.05f && CurrentTime - LastCircleMoveTime >= 0.3f) // 添加轻微冷却时间以减少抽搐
    {
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon执行环绕移动"));
        LastCircleMoveTime = CurrentTime;

        // 计算围绕玩家的移动方向
        FVector ToPlayer = TargetPlayer->GetActorLocation() - GetActorLocation();
        FVector RightVector = FVector(-ToPlayer.Y, ToPlayer.X, 0.f).GetSafeNormal();

        // 随机选择顺时针或逆时针
        if (FMath::RandBool())
        {
            RightVector = -RightVector;
        }

        if (GetCharacterMovement())
        {
            // 设置环绕移动速度
            GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed * 0.6f;
            AddMovementInput(RightVector, 1.0f);
        }
    }

    // 使用Gideon专用的攻击冷却时间
    // 添加随机因素使攻击节奏更自然
    float RandomizedCooldown = GideonAttackCooldown * FMath::FRandRange(0.8f, 1.2f);
    if (TimeSinceLastAttack >= RandomizedCooldown)
    {
        LastGideonAttackTime = CurrentTime;

        // 根据当前阶段调整攻击行为
        switch (CurrentPhase)
        {
        case EBossPhase::Phase1:
            // 第一阶段使用三种攻击动作，都附带黑暗特效
            UE_LOG(LogTemp, Warning, TEXT("👹 Gideon第一阶段攻击"));
            {
                // 随机选择三种攻击动作之一
                int32 AttackType = FMath::RandRange(0, 2);
                switch (AttackType)
                {
                case 0:
                    // 使用攻击A动画
                    if (AttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
                    {
                        GetMesh()->GetAnimInstance()->Montage_Play(AttackMontage, 1.0f);
                        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon播放攻击A动画"));
                    }
                    break;
                case 1:
                    // 使用攻击B动画
                    if (HeavyAttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
                    {
                        GetMesh()->GetAnimInstance()->Montage_Play(HeavyAttackMontage, 1.0f);
                        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon播放攻击B动画"));
                    }
                    break;
                case 2:
                    // 使用攻击C动画
                    if (SkillMontage && GetMesh() && GetMesh()->GetAnimInstance())
                    {
                        GetMesh()->GetAnimInstance()->Montage_Play(SkillMontage, 1.0f);
                        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon播放攻击C动画"));
                    }
                    break;
                }
                // 在施法瞬间记录玩家位置
                FVector SpellCastLocation = GetPlayerLocation();

                // 生成胸前特效
                SpawnChestEffect();

                // 延迟0.1-0.4秒后同时生成视觉特效和开始持续伤害判定
                float RandomDelay = FMath::FRandRange(0.1f, 0.4f);
                FTimerHandle EffectAndDamageDelayHandle;
                GetWorldTimerManager().SetTimer(EffectAndDamageDelayHandle, [this, SpellCastLocation]() {
                    // 生成视觉特效
                    SpawnAttackEffectAtLocation(ESkillType::Dark, SpellCastLocation);
                    // 开始持续伤害判定
                    StartContinuousDamage(ESkillType::Dark, SpellCastLocation);
                    }, RandomDelay, false);

                // 应用近战伤害
                ApplyMeleeDamage();
            }
            break;

        case EBossPhase::Phase2:
            // 第二阶段使用三种攻击动作，附带黑暗或火焰特效
            UE_LOG(LogTemp, Warning, TEXT("👹 Gideon第二阶段攻击"));
            {
                // 随机选择三种攻击动作之一
                int32 AttackType = FMath::RandRange(0, 2);
                switch (AttackType)
                {
                case 0:
                    // 使用攻击A动画
                    if (AttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
                    {
                        GetMesh()->GetAnimInstance()->Montage_Play(AttackMontage, 1.0f);
                        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon播放攻击A动画"));
                    }
                    break;
                case 1:
                    // 使用攻击B动画
                    if (HeavyAttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
                    {
                        GetMesh()->GetAnimInstance()->Montage_Play(HeavyAttackMontage, 1.0f);
                        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon播放攻击B动画"));
                    }
                    break;
                case 2:
                    // 使用攻击C动画
                    if (SkillMontage && GetMesh() && GetMesh()->GetAnimInstance())
                    {
                        GetMesh()->GetAnimInstance()->Montage_Play(SkillMontage, 1.0f);
                        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon播放攻击C动画"));
                    }
                    break;
                }

                // 在施法瞬间记录玩家位置
                ESkillType SkillType = (FMath::RandBool()) ? ESkillType::Dark : ESkillType::Fire;
                FVector SpellCastLocation = GetPlayerLocation();

                // 生成胸前特效
                SpawnChestEffect();

                // 延迟0.1-0.4秒后同时生成视觉特效和开始持续伤害判定
                float RandomDelay = FMath::FRandRange(0.1f, 0.4f);
                FTimerHandle EffectAndDamageDelayHandle;
                GetWorldTimerManager().SetTimer(EffectAndDamageDelayHandle, [this, SkillType, SpellCastLocation]() {
                    // 生成视觉特效
                    SpawnAttackEffectAtLocation(SkillType, SpellCastLocation);
                    // 开始持续伤害判定
                    StartContinuousDamage(SkillType, SpellCastLocation);
                    }, RandomDelay, false);

                // 应用近战伤害
                ApplyMeleeDamage();
            }
            break;

        case EBossPhase::Phase3:
            // 第三阶段使用三种攻击动作，附带火焰或冰霜特效
            UE_LOG(LogTemp, Warning, TEXT("👹 Gideon第三阶段攻击"));
            {
                // 随机选择三种攻击动作之一
                int32 AttackType = FMath::RandRange(0, 2);
                switch (AttackType)
                {
                case 0:
                    // 使用攻击A动画
                    if (AttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
                    {
                        GetMesh()->GetAnimInstance()->Montage_Play(AttackMontage, 1.0f);
                        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon播放攻击A动画"));
                    }
                    break;
                case 1:
                    // 使用攻击B动画
                    if (HeavyAttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
                    {
                        GetMesh()->GetAnimInstance()->Montage_Play(HeavyAttackMontage, 1.0f);
                        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon播放攻击B动画"));
                    }
                    break;
                case 2:
                    // 使用攻击C动画
                    if (SkillMontage && GetMesh() && GetMesh()->GetAnimInstance())
                    {
                        GetMesh()->GetAnimInstance()->Montage_Play(SkillMontage, 1.0f);
                        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon播放攻击C动画"));
                    }
                    break;
                }

                // 在施法瞬间记录玩家位置
                ESkillType SkillType = (FMath::RandBool()) ? ESkillType::Fire : ESkillType::Ice;
                FVector SpellCastLocation = GetPlayerLocation();

                // 生成胸前特效
                SpawnChestEffect();

                // 延迟0.1-0.4秒后同时生成视觉特效和开始持续伤害判定
                float RandomDelay = FMath::FRandRange(0.1f, 0.4f);
                FTimerHandle EffectAndDamageDelayHandle;
                GetWorldTimerManager().SetTimer(EffectAndDamageDelayHandle, [this, SkillType, SpellCastLocation]() {
                    // 生成视觉特效
                    SpawnAttackEffectAtLocation(SkillType, SpellCastLocation);
                    // 开始持续伤害判定
                    StartContinuousDamage(SkillType, SpellCastLocation);
                    }, RandomDelay, false);

                // 应用近战伤害
                ApplyMeleeDamage();
            }
            break;
        }

        // 0.5秒后重置攻击状态（使用定时器）
        FTimerHandle AttackResetHandle;
        GetWorldTimerManager().SetTimer(AttackResetHandle, [this]()
            {
                // 这里我们不需要重置bIsAttacking，因为我们没有使用它
            }, 0.5f, false);
    }
}



void AParagonGideon::StartContinuousDamage(ESkillType SkillType, FVector Location)
{
    if (bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon无法开始持续伤害，Boss已死亡"));
        return;
    }

    // 保存当前特效信息
    CurrentEffectLocation = Location;
    CurrentEffectSkillType = SkillType;
    CurrentContinuousDamageTicks = 0;

    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon开始持续伤害，技能类型: %s"),
        SkillType == ESkillType::Fire ? TEXT("火焰") :
        SkillType == ESkillType::Ice ? TEXT("冰霜") : TEXT("黑暗"));

    // 启动每0.15秒执行一次的循环定时器，持续2.25秒（共15次）
    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon设置持续伤害定时器：间隔0.15秒，总次数%d次"), MaxContinuousDamageTicks);
    GetWorldTimerManager().SetTimer(ContinuousDamageTimerHandle, this, &AParagonGideon::ApplyContinuousDamage, 0.15f, true);
}

void AParagonGideon::ApplyContinuousDamage()
{
    if (bIsDead)
    {
        // 清除定时器
        GetWorldTimerManager().ClearTimer(ContinuousDamageTimerHandle);
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon持续伤害结束，Boss已死亡"));
        return;
    }

    // 增加计数
    CurrentContinuousDamageTicks++;

    // 检查是否达到最大次数
    if (CurrentContinuousDamageTicks >= MaxContinuousDamageTicks)
    {
        // 清除定时器
        GetWorldTimerManager().ClearTimer(ContinuousDamageTimerHandle);
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon持续伤害结束，已达最大次数%d次"), MaxContinuousDamageTicks);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon执行第%d次持续伤害判定 (间隔0.15秒)"), CurrentContinuousDamageTicks);

    // 应用伤害
    ApplyAttackEffectDamage(CurrentEffectSkillType, CurrentEffectLocation);
}

ESkillType AParagonGideon::GetRandomSkillTypeForPhase()
{
    switch (CurrentPhase)
    {
    case EBossPhase::Phase1:
        // 第一阶段只使用黑暗技能
        return ESkillType::Dark;

    case EBossPhase::Phase2:
        // 第二阶段随机选择技能类型
    {
        float RandValue = FMath::FRand();
        if (RandValue < 0.33f)
        {
            return ESkillType::Fire;
        }
        else if (RandValue < 0.66f)
        {
            return ESkillType::Ice;
        }
        else
        {
            return ESkillType::Dark;
        }
    }

    case EBossPhase::Phase3:
        // 第三阶段只使用火焰技能
        return ESkillType::Fire;

    default:
        return ESkillType::Dark;
    }
}

void AParagonGideon::SpawnAttackEffectAtLocation(ESkillType SkillType, FVector Location)
{
    if (bIsDead) return;

    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon准备生成攻击特效: %s 在位置 X=%.2f, Y=%.2f, Z=%.2f"),
        SkillType == ESkillType::Fire ? TEXT("火焰") :
        SkillType == ESkillType::Ice ? TEXT("冰霜") : TEXT("黑暗"),
        Location.X, Location.Y, Location.Z);

    // 稍微降低一点位置，确保特效在地面上
    FVector SpawnLocation = Location;
    SpawnLocation.Z -= 50.0f;

    // 根据技能类型选择对应的蓝图类
    TSubclassOf<AActor> EffectClass = nullptr;
    switch (SkillType)
    {
    case ESkillType::Fire:
        EffectClass = FireSkillEffectClass;
        break;
    case ESkillType::Ice:
        EffectClass = IceSkillEffectClass;
        break;
    case ESkillType::Dark:
        EffectClass = DarkSkillEffectClass;
        break;
    }

    // 检查特效类是否有效
    if (!EffectClass)
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon特效类为空，无法生成特效: %s"),
            SkillType == ESkillType::Fire ? TEXT("火焰") :
            SkillType == ESkillType::Ice ? TEXT("冰霜") : TEXT("黑暗"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon准备使用特效类: %s"), *EffectClass->GetName());

    // 生成特效
    if (GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon世界对象有效，准备生成特效"));
        AActor* SpawnedEffect = GetWorld()->SpawnActor<AActor>(EffectClass, SpawnLocation, FRotator::ZeroRotator);
        if (SpawnedEffect)
        {
            UE_LOG(LogTemp, Warning, TEXT("👹 Gideon成功生成%s攻击特效！地址: %p"),
                SkillType == ESkillType::Fire ? TEXT("火焰") :
                SkillType == ESkillType::Ice ? TEXT("冰霜") : TEXT("黑暗"),
                SpawnedEffect);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("👹 Gideon生成%s攻击特效失败！"),
                SkillType == ESkillType::Fire ? TEXT("火焰") :
                SkillType == ESkillType::Ice ? TEXT("冰霜") : TEXT("黑暗"));
            UE_LOG(LogTemp, Error, TEXT("👹 Gideon特效类详情: %s"), *EffectClass->GetFullName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("👹 Gideon世界对象为空，无法生成特效"));
    }
}

void AParagonGideon::SpawnChestEffect()
{
    if (bIsDead || !ChestEffectClass) return;

    // 获取胸前位置（使用骨骼或相对位置）
    FVector ChestLocation = GetActorLocation() + FVector(0, 0, 80); // 简单的胸部偏移

    // 生成胸前特效
    if (GetWorld())
    {
        AActor* ChestEffect = GetWorld()->SpawnActor<AActor>(ChestEffectClass, ChestLocation, FRotator::ZeroRotator);
        if (ChestEffect)
        {
            UE_LOG(LogTemp, Warning, TEXT("👹 Gideon生成胸前特效成功"));

            // 0.3秒后销毁特效
            FTimerHandle ChestEffectTimerHandle;
            GetWorldTimerManager().SetTimer(ChestEffectTimerHandle, [this, ChestEffect]() {
                if (ChestEffect && !ChestEffect->IsPendingKillPending())
                {
                    ChestEffect->Destroy();
                    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon胸前特效销毁"));
                }
                }, 0.3f, false);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("👹 Gideon生成胸前特效失败"));
        }
    }
}

void AParagonGideon::ApplyAttackEffectDamage(ESkillType SkillType, FVector Location)
{
    if (bIsDead) return;

    // 根据技能类型设置伤害值和范围
    float DamageAmount = 0.0f;
    float DamageRadius = 100.0f;  // 范围技能作用区间改为100

    switch (SkillType)
    {
    case ESkillType::Fire:
        DamageAmount = SkillDamage * 1.2f; // 火焰伤害较高
        break;
    case ESkillType::Ice:
        DamageAmount = SkillDamage * 1.0f; // 冰霜伤害中等
        break;
    case ESkillType::Dark:
        DamageAmount = SkillDamage * 0.8f; // 黑暗伤害较低
        break;
    }

    // 确保特效伤害不会太高
    DamageAmount = FMath::Min(DamageAmount, 5.0f);

    // 使用球形检测查找范围内的玩家
    TArray<FHitResult> HitResults;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(DamageRadius);

    if (GetWorld()->SweepMultiByObjectType(
        HitResults,
        Location,
        Location,
        FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn),
        CollisionShape))
    {
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon检测到%d个命中目标"), HitResults.Num());

        // 用于跟踪已经受伤的Actor，避免同一Actor在同一 tick 内多次受伤
        TSet<AActor*> DamagedActors;
        int32 AppliedDamageCount = 0;  // 记录实际造成伤害的次数

        // 遍历所有命中的Actor
        for (const FHitResult& Hit : HitResults)
        {
            // 检查是否是玩家角色
            AActor* HitActor = Hit.GetActor();
            if (HitActor && HitActor != this)
            {
                // 检查是否已经对该Actor造成了伤害
                if (DamagedActors.Contains(HitActor))
                {
                    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon跳过重复伤害目标: %s"), *HitActor->GetName());
                    continue;
                }

                // 记录已受伤的Actor
                DamagedActors.Add(HitActor);
                AppliedDamageCount++;

                // 对玩家造成伤害
                FDamageEvent DamageEvent;
                HitActor->TakeDamage(
                    DamageAmount,
                    DamageEvent,
                    GetController(),
                    this
                );

                UE_LOG(LogTemp, Warning, TEXT("👹 Gideon的%s技能对%s造成%.1f点伤害！"),
                    SkillType == ESkillType::Fire ? TEXT("火焰") :
                    SkillType == ESkillType::Ice ? TEXT("冰霜") : TEXT("黑暗"),
                    *HitActor->GetName(), DamageAmount);
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon本次伤害判定实际对%d个目标造成了伤害"), AppliedDamageCount);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon未检测到任何目标"));
    }
}

void AParagonGideon::UseSpecialAbility()
{
    // 根据阶段选择不同技能
    switch (CurrentPhase)
    {
    case EBossPhase::Phase2:
        if (!bHasShield)
        {
            ActivateShield(); // 二阶段使用护盾
        }
        break;

    case EBossPhase::Phase3:
        SummonMinions(); // 三阶段召唤小怪
        break;

    default:
        // 默认不执行特殊技能
        break;
    }
}

void AParagonGideon::SummonMinions()
{
    if (bIsDead) return;

    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon召唤小怪！"));

    // 获取世界指针
    UWorld* World = GetWorld();
    if (!World) return;

    // 获取Boss的位置和旋转
    FVector BossLocation = GetActorLocation();
    FRotator BossRotation = GetActorRotation();

    // 在Boss周围生成三个削弱版的小怪
    for (int32 i = 0; i < 3; i++)
    {
        // 计算小怪生成位置（围绕Boss分布，三个方向）
        FVector SpawnOffset;
        switch (i)
        {
        case 0: // 前方
            SpawnOffset = FVector(1000.f, 0.f, 0.f);
            break;
        case 1: // 左侧
            SpawnOffset = FVector(0.f, -1000.f, 0.f);
            break;
        case 2: // 右侧
        default:
            SpawnOffset = FVector(0.f, 1000.f, 0.f);
            break;
        }

        // 应用Boss的旋转，使方向相对于Boss朝向
        SpawnOffset = BossRotation.RotateVector(SpawnOffset);
        FVector SpawnLocation = BossLocation + SpawnOffset;

        // 选择一种小怪类型
        TSubclassOf<AParagonFengMao> MinionClass;
        switch (i)
        {
        case 0:
            if (NarbashMinionClass.Get() != nullptr)
                MinionClass = NarbashMinionClass;
            else
                MinionClass = AParagonNarbash::StaticClass();
            break;
        case 1:
            if (RampageMinionClass.Get() != nullptr)
                MinionClass = RampageMinionClass;
            else
                MinionClass = AParagonRampage::StaticClass();
            break;
        case 2:
        default:
            if (FengMaoMinionClass.Get() != nullptr)
                MinionClass = FengMaoMinionClass;
            else
                MinionClass = AParagonFengMao::StaticClass();
            break;
        }

        // 生成小怪
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        // 设置生成参数，确保AI控制器能正确初始化
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AParagonFengMao* Minion = World->SpawnActor<AParagonFengMao>(MinionClass, SpawnLocation, BossRotation, SpawnParams);
        if (Minion)
        {
            // 确保小怪有正确的AI控制器类
            Minion->AIControllerClass = AFengMaoAIController::StaticClass();

            // 初始化AI控制器
            Minion->SpawnDefaultController();

            // 削弱小怪能力（降至50%）
            Minion->MaxHealth *= 0.5f;
            Minion->CurrentHealth = Minion->MaxHealth;
            // 保持小怪原有的攻击伤害设定，不使用Boss的伤害值
            Minion->AttackDamage = 25.f * 0.5f;  // 普通攻击伤害削弱至50%
            Minion->HeavyAttackDamage = 50.f * 0.5f;  // 重击伤害削弱至50%
            Minion->PatrolSpeed *= 0.7f;
            Minion->ChaseSpeed *= 0.7f;

            // 初始化小怪的AI状态
            Minion->SetAIState(EFengMaoAIState::Patrol);

            UE_LOG(LogTemp, Warning, TEXT("👹 召唤了削弱版小怪: %s"), *MinionClass->GetName());

            // 确保小怪能够感知到玩家，并且不会攻击Boss本身
            Minion->TargetPlayer = TargetPlayer;

            // 设置召唤者引用，用于友方识别
            Minion->Summoner = this;

            // 设置小怪的AI控制器
            if (AFengMaoAIController* MinionAI = Cast<AFengMaoAIController>(Minion->GetController()))
            {
                UE_LOG(LogTemp, Warning, TEXT("👹 小怪AI控制器已设置"));
            }
        }
    }
}

void AParagonGideon::ActivateShield()
{
    if (bIsDead || bHasShield) return;

    bHasShield = true;
    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon激活护盾！"));

    // 启动护盾定时器
    if (GetWorld())
    {
        GetWorldTimerManager().SetTimer(ShieldTimerHandle, this, &AParagonGideon::DeactivateShield, ShieldDuration, false);
    }
}

void AParagonGideon::DeactivateShield()
{
    bHasShield = false;
    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon护盾消失！"));
}

void AParagonGideon::EnterEnrageMode()
{
    if (bIsDead || bIsEnraged) return;

    bIsEnraged = true;
    UE_LOG(LogTemp, Warning, TEXT("👹👹 Gideon进入狂暴状态！"));

    // 狂暴状态下属性进一步提升
    MeleeAttackDamage *= 1.2f;
    ChaseSpeed *= 0.9f;

    // 狂暴状态下加快攻击速度
    GideonAttackCooldown = 2.0f;  // 狂暴模式攻击冷却时间（加快0.5秒）

    // 狂暴状态持续一段时间后自动解除
    if (GetWorld())
    {
        GetWorldTimerManager().SetTimer(EnrageTimerHandle, [this]() {
            bIsEnraged = false;
            // 恢复普通模式的攻击冷却时间
            GideonAttackCooldown = 2.5f;  // 普通模式攻击冷却时间（加快0.5秒）
            UE_LOG(LogTemp, Warning, TEXT("👹👹 Gideon狂暴状态结束"));
            }, 20.0f, false);
    }
}

void AParagonGideon::ApplyMeleeDamage()
{
    if (!TargetPlayer || bIsDead) return;

    // 检查目标是否在扇形判定范围内（前方90度，距离150）
    if (!IsTargetInFrontSector(TargetPlayer, 90.f, 150.f))
    {
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon近战攻击未命中！目标不在扇形判定范围内"));
        return;
    }

    // 创建伤害事件并应用伤害
    FDamageEvent DamageEvent;
    FHitResult HitResult;
    FPointDamageEvent PointDamageEvent(MeleeAttackDamage, HitResult, GetActorForwardVector(), nullptr);

    float ActualDamage = TargetPlayer->TakeDamage(MeleeAttackDamage, PointDamageEvent, nullptr, this);

    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon近战攻击成功！对 %s 造成 %.1f 伤害 (实际: %.1f)"),
        *TargetPlayer->GetName(), MeleeAttackDamage, ActualDamage);
}

void AParagonGideon::Die()
{
    if (bIsDead) return;

    // 调用父类死亡函数
    Super::Die();

    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon Boss被击败！"));

    // 清理所有定时器
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(SpecialAbilityTimerHandle);
        GetWorldTimerManager().ClearTimer(ShieldTimerHandle);
        GetWorldTimerManager().ClearTimer(EnrageTimerHandle);
    }
}