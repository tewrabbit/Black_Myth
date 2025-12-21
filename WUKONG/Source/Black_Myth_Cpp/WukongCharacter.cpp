#include "WukongCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Controller.h"
#include "InputCoreTypes.h"
#include "Engine/LocalPlayer.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "InventoryWidget.h"
#include "SkillSlotWidget.h"
#include "PauseMenuWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Engine/World.h" // 用于射线检测
#include "Kismet/GameplayStatics.h" // 用于GameplayStatics工具函数
#include "ParagonFengMao.h" // 敌人角色类
#include "GameFramework/CharacterMovementComponent.h" // 角色移动组件
#include "Components/SkeletalMeshComponent.h" // 骨骼网格体组件（用于物理模拟）
#include "DrawDebugHelpers.h" // 🆕 添加调试绘制头文件
#include "Camera/PlayerCameraManager.h"
#include "MyCameraModifier.h"
#include "TimerManager.h"
#include "Engine/World.h"
/*
===============================================================================
    🏮 悟空角色系统 - 实现文件
    包含所有战斗逻辑、移动控制、动画播放的具体实现
===============================================================================
*/

// 🏗️ 构造函数 - 角色出生时创建所有组件和设置默认值
AWukongCharacter::AWukongCharacter() {
    PrimaryActorTick.bCanEverTick = true;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 540, 0);
    GetCharacterMovement()->bUseControllerDesiredRotation = true;
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 🎯 创建输入动作对象
    MoveAction = CreateDefaultSubobject<UInputAction>(TEXT("MoveAction"));
    MoveAction->ValueType = EInputActionValueType::Axis2D;

    LookAction = CreateDefaultSubobject<UInputAction>(TEXT("LookAction"));
    LookAction->ValueType = EInputActionValueType::Axis2D;

    JumpAction = CreateDefaultSubobject<UInputAction>(TEXT("JumpAction"));
    JumpAction->ValueType = EInputActionValueType::Boolean;

    LightAttackAction = CreateDefaultSubobject<UInputAction>(TEXT("LightAttackAction"));
    LightAttackAction->ValueType = EInputActionValueType::Boolean;

    SprintAction = CreateDefaultSubobject<UInputAction>(TEXT("SprintAction"));
    SprintAction->ValueType = EInputActionValueType::Boolean;

    DodgeAction = CreateDefaultSubobject<UInputAction>(TEXT("DodgeAction"));
    DodgeAction->ValueType = EInputActionValueType::Boolean;

    HeavyAttackAction = CreateDefaultSubobject<UInputAction>(TEXT("HeavyAttackAction"));
    HeavyAttackAction->ValueType = EInputActionValueType::Boolean;

    // 新增的输入动作
    StunSkillAction = CreateDefaultSubobject<UInputAction>(TEXT("StunSkillAction"));
    StunSkillAction->ValueType = EInputActionValueType::Boolean;   // 布尔值（Q键）

    DrinkPotionAction = CreateDefaultSubobject<UInputAction>(TEXT("DrinkPotionAction"));
    DrinkPotionAction->ValueType = EInputActionValueType::Boolean; // 布尔值（E键）

    // 🎭 隐身动作
    ToggleInvisibilityAction = CreateDefaultSubobject<UInputAction>(TEXT("ToggleInvisibilityAction"));
    ToggleInvisibilityAction->ValueType = EInputActionValueType::Boolean;

    // 🌀 变身动作
    TransformAction = CreateDefaultSubobject<UInputAction>(TEXT("TransformAction"));
    TransformAction->ValueType = EInputActionValueType::Boolean;

    // 测试功能按键
    TestDamageAction = CreateDefaultSubobject<UInputAction>(TEXT("TestDamageAction"));
    TestDamageAction->ValueType = EInputActionValueType::Boolean;

    TestDeathAction = CreateDefaultSubobject<UInputAction>(TEXT("TestDeathAction"));
    TestDeathAction->ValueType = EInputActionValueType::Boolean;

    TestRespawnAction = CreateDefaultSubobject<UInputAction>(TEXT("TestRespawnAction"));
    TestRespawnAction->ValueType = EInputActionValueType::Boolean;

    TestDetectAction = CreateDefaultSubobject<UInputAction>(TEXT("TestDetectAction"));
    TestDetectAction->ValueType = EInputActionValueType::Boolean;

    // 🆕 测试碰撞检测按键
    TestCollisionAction = CreateDefaultSubobject<UInputAction>(TEXT("TestCollisionAction"));
    TestCollisionAction->ValueType = EInputActionValueType::Boolean;

    // 🎪 创建输入映射上下文
    InputMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("InputMappingContext"));

    // 🗺️ 配置按键映射
    InputMappingContext->MapKey(MoveAction, EKeys::D);
    {
        auto& AMap = InputMappingContext->MapKey(MoveAction, EKeys::A);
        AMap.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("MoveNegateA")));
    }
    {
        auto& WMap = InputMappingContext->MapKey(MoveAction, EKeys::W);
        auto* SwizzleW = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("MoveSwizzleW"));
        SwizzleW->Order = EInputAxisSwizzle::YXZ;
        WMap.Modifiers.Add(SwizzleW);
    }
    {
        auto& SMap = InputMappingContext->MapKey(MoveAction, EKeys::S);
        auto* SwizzleS = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("MoveSwizzleS"));
        SwizzleS->Order = EInputAxisSwizzle::YXZ;
        SMap.Modifiers.Add(SwizzleS);
        SMap.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("MoveNegateS")));
    }

    // 👀 视角控制映射
    InputMappingContext->MapKey(LookAction, EKeys::MouseX);
    {
        auto& MY = InputMappingContext->MapKey(LookAction, EKeys::MouseY);
        auto* SwizzleY = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("LookSwizzleY"));
        SwizzleY->Order = EInputAxisSwizzle::YXZ;
        MY.Modifiers.Add(SwizzleY);
    }

    // 🎯 其他动作按键映射
    InputMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
    InputMappingContext->MapKey(LightAttackAction, EKeys::LeftMouseButton);
    InputMappingContext->MapKey(SprintAction, EKeys::LeftShift);
    InputMappingContext->MapKey(DodgeAction, EKeys::F);
    InputMappingContext->MapKey(HeavyAttackAction, EKeys::RightMouseButton);
    InputMappingContext->MapKey(ToggleInvisibilityAction, EKeys::V);
    InputMappingContext->MapKey(TransformAction, EKeys::R);    // 🌀 变身按键
    InputMappingContext->MapKey(StunSkillAction, EKeys::Q);             // 定身技能
    InputMappingContext->MapKey(DrinkPotionAction, EKeys::E);           // 喝药
    // 测试功能按键映射
    InputMappingContext->MapKey(TestDamageAction, EKeys::T);
    InputMappingContext->MapKey(TestDeathAction, EKeys::Y);
    InputMappingContext->MapKey(TestRespawnAction, EKeys::U);
    InputMappingContext->MapKey(TestDetectAction, EKeys::G);
    // 🆕 测试碰撞检测按键映射
    InputMappingContext->MapKey(TestCollisionAction, EKeys::H);

    UE_LOG(LogTemp, Warning, TEXT("Constructor IMC Mapping count=%d"), InputMappingContext->GetMappings().Num());

    // ==========================================
    // 📦 初始化角色属性和加载动画资源
    // ==========================================

    // 🎯 初始化战斗状态变量
    AtttackMyCount = 0;
    bMyIsAttacking = false;
    CurrentActionState = EWukongActionState::Idle;

    // 🎬 加载轻攻击连击动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> LightMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/PrimaryMelee_MasterMontage")
    );

    if (LightMontageObj.Succeeded())
    {
        PrimaryMeleeMontage = LightMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ PrimaryMeleeMontage Loaded OK: %s"), *GetNameSafe(PrimaryMeleeMontage));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PrimaryMeleeMontage FAILED TO LOAD!"));
    }

    // 🎯 加载独立的攻击动画文件
    static ConstructorHelpers::FObjectFinder<UAnimMontage> Attack1Obj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_A_Slow_Montage")
    );
    if (Attack1Obj.Succeeded()) {
        Attack1Montage = Attack1Obj.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> Attack2Obj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_B_Slow_Montage")
    );
    if (Attack2Obj.Succeeded()) {
        Attack2Montage = Attack2Obj.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> Attack3Obj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_C_Slow_Montage")
    );
    if (Attack3Obj.Succeeded()) {
        Attack3Montage = Attack3Obj.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> Attack4Obj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_D_Slow_Montage")
    );
    if (Attack4Obj.Succeeded()) {
        Attack4Montage = Attack4Obj.Object;
    }

    // 💥 加载重攻击动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> HeavyMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_D_Slow_Montage")
    );
    if (HeavyMontageObj.Succeeded()) {
        HeavyAttackMontage = HeavyMontageObj.Object;
    }
    else {
        HeavyAttackMontage = Attack1Montage;
    }

    // 💨 加载闪避动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> DodgeMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Evade1")
    );
    if (DodgeMontageObj.Succeeded())
    {
        DodgeMontage = DodgeMontageObj.Object;
    }

    // 🔧 初始化所有游戏属性
    CurrentActionState = EWukongActionState::Idle;

    // 🔋 体力系统初始化
    CurrentStamina = MaxStamina = 100.f;
    StaminaRecoveryRate = 10.f;
    LightAttackStaminaCost = 10.f;
    DodgeStaminaCost = 20.f;
    HeavyAttackStaminaCost = 30.f;
    HeavyAttackDistance = 500.f;
    HeavyAttackDuration = 0.4f;
    StunSkillStaminaCost = 25.f;

    // ❤️ 生命值系统初始化
    CurrentHealth = MaxHealth = 200.f;
    HealthRecoveryRate = 7.f;
    bIsDead = false;

    // ⚔️ 伤害系统初始化
    LightAttackDamage = 20.f;
    HeavyAttackBaseDamage = 50.f;
    StunSkillDamage = 30.f;

    // 🔋 蓄力系统初始化
    CurrentChargeTime = 0.f;
    MaxChargeTime = 2.f;
    MinChargeDamageMultiplier = 1.f;
    MaxChargeDamageMultiplier = 3.f;
    bIsCharging = false;

    // ⚔️ 战斗系统初始化
    LightComboBufferWindow = 1.0f;
    CurrentLightComboIndex = 0;
    bLightAttackQueued = false;
    bIsInvincible = false;

    // 💨 闪避系统初始化
    DodgeDistance = 300.f;
    DodgeSpeed = 1000.f;
    DodgeDuration = 0.5f;
    DodgeCooldown = 0.5f;
    bCanDodge = true;
    LastMovementInput = FVector2D::ZeroVector;

    // 💫 定身技能初始化
    StunDuration = 3.0f;                          // 定身持续3秒
    StunSkillRange = 300.f;

    // 🎭 隐身系统初始化
    bIsInvisible = false;
    bCanToggleInvisibility = true;
    InvisibilityCooldown = 0.5f;

    // 🌀 变身系统初始化
    bIsTransformed = false;
    TransformScaleMultiplier = 1.5f;     // 放大1.5倍
    TransformDuration = 10.0f;

    // 🧪 喝药系统初始化
    PotionCount = 0;                              // 初始没有瓶药水
    InstantHealAmount = 20.f;                     // 瞬间回复10点生命值
    OverTimeHealAmount = 30.f;                    // 持续回复30点生命值
    OverTimeHealDuration = 2.5f;                  // 持续2.5秒
    OverTimeHealInterval = 1.0f;  // 持续1秒

    // 🆕 碰撞检测系统初始化
    bShowDebugVisuals = true;
    LastDetectedEnemies.Empty();
    DamageNumberLocations.Empty();
    DamageNumberValues.Empty();
    DamageNumberIsCritical.Empty();

    //变身效果的初始化
    TransformStaminaCost = 10.0f;
}
// 🔄 Tick函数 - 每帧执行的核心逻辑
void AWukongCharacter::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

    // 🔍 动画状态调试
    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance()) {
        UAnimMontage* CurrentPlayingMontage = nullptr;
        FName CurrentSection = NAME_None;
        float CurrentPos = 0.0f;

        if (Attack1Montage && AnimInst->Montage_IsPlaying(Attack1Montage)) {
            CurrentPlayingMontage = Attack1Montage;
            CurrentSection = FName(TEXT("Attack1"));
            CurrentPos = AnimInst->Montage_GetPosition(Attack1Montage);
        }
        else if (Attack2Montage && AnimInst->Montage_IsPlaying(Attack2Montage)) {
            CurrentPlayingMontage = Attack2Montage;
            CurrentSection = FName(TEXT("Attack2"));
            CurrentPos = AnimInst->Montage_GetPosition(Attack2Montage);
        }
        else if (Attack3Montage && AnimInst->Montage_IsPlaying(Attack3Montage)) {
            CurrentPlayingMontage = Attack3Montage;
            CurrentSection = FName(TEXT("Attack3"));
            CurrentPos = AnimInst->Montage_GetPosition(Attack3Montage);
        }
        else if (Attack4Montage && AnimInst->Montage_IsPlaying(Attack4Montage)) {
            CurrentPlayingMontage = Attack4Montage;
            CurrentSection = FName(TEXT("Attack4"));
            CurrentPos = AnimInst->Montage_GetPosition(Attack4Montage);
        }
        else if (HeavyAttackMontage && AnimInst->Montage_IsPlaying(HeavyAttackMontage)) {
            CurrentPlayingMontage = HeavyAttackMontage;
            CurrentSection = FName(TEXT("HeavyAttack"));
            CurrentPos = AnimInst->Montage_GetPosition(HeavyAttackMontage);
        }
        else if (PrimaryMeleeMontage && AnimInst->Montage_IsPlaying(PrimaryMeleeMontage)) {
            CurrentPlayingMontage = PrimaryMeleeMontage;
            CurrentSection = AnimInst->Montage_GetCurrentSection(PrimaryMeleeMontage);
            CurrentPos = AnimInst->Montage_GetPosition(PrimaryMeleeMontage);
        }

        if (CurrentPlayingMontage) {
            static float LastPos = 0.0f;
            if (FMath::Abs(CurrentPos - LastPos) > 0.1f) {
                float TotalLength = CurrentPlayingMontage->GetPlayLength();
                float Progress = TotalLength > 0.0f ? (CurrentPos / TotalLength) * 100.0f : 0.0f;
                float Remaining = TotalLength - CurrentPos;

                UE_LOG(LogTemp, Warning, TEXT("🎭 === 动画状态 === 段落:%s | 计数:%d | 进度:%.1f%% | 位置:%.2fs/%.2fs | 剩余:%.2fs | 攻击中:%d | 缓冲:%d"),
                    *CurrentSection.ToString(), AtttackMyCount, Progress,
                    CurrentPos, TotalLength, Remaining,
                    bMyIsAttacking, bLightAttackQueued);
                LastPos = CurrentPos;
            }
        }
    }

    //// 🔋 体力恢复逻辑
    //if (CurrentActionState != EWukongActionState::Sprint) {
    //    CurrentStamina += StaminaRecoveryRate * DeltaTime;
    //    CurrentStamina = FMath::Min(CurrentStamina, MaxStamina);
    //}

    //// ❤️ 生命值恢复逻辑
    //if (!bIsDead && CurrentHealth < MaxHealth) {
    //    CurrentHealth += HealthRecoveryRate * DeltaTime;
    //    CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
    //}

    static float DebugTimer = 0.f;
    DebugTimer += DeltaTime;
    if (DebugTimer >= 1.f)
    {
        DebugTimer = 0.f;
        UE_LOG(LogTemp, Warning, TEXT("📊 状态: HP=%.1f/%.1f, SP=%.1f/%.1f, 死亡=%d, 蓄力=%d"),
            CurrentHealth, MaxHealth, CurrentStamina, MaxStamina, bIsDead, bIsCharging);

        // 检查是否有敌人 - 使用更灵活的方式查找
        TArray<AActor*> FoundEnemies;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AParagonFengMao::StaticClass(), FoundEnemies);

        // 如果没找到，尝试查找所有Character，看看是否有敌人
        if (FoundEnemies.Num() == 0)
        {
            TArray<AActor*> AllCharacters;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllCharacters);

            UE_LOG(LogTemp, Warning, TEXT("🔍 场景中总共有 %d 个Character角色"), AllCharacters.Num());

            // 列出所有Character的信息
            for (AActor* Char : AllCharacters)
            {
                if (Char && Char != this)
                {
                    FString ClassName = Char->GetClass()->GetName();
                    FString ActorName = Char->GetName();
                    UClass* CharClass = Char->GetClass();

                    // 获取父类信息
                    FString ParentClassName = TEXT("无");
                    if (CharClass && CharClass->GetSuperClass())
                    {
                        ParentClassName = CharClass->GetSuperClass()->GetName();
                    }

                    UE_LOG(LogTemp, Warning, TEXT("  📋 Character: %s (类名: %s, 父类: %s)"),
                        *ActorName, *ClassName, *ParentClassName);

                    // 检查是否是ParagonFengMao或其子类
                    bool bIsEnemy = false;
                    if (Char->IsA(AParagonFengMao::StaticClass()))
                    {
                        bIsEnemy = true;
                        UE_LOG(LogTemp, Warning, TEXT("    ✅ 这是封魔敌人（通过IsA检查）！"));
                    }
                    // 检查类名是否包含FengMao（可能是蓝图类）
                    else if (ClassName.Contains(TEXT("FengMao")) || ClassName.Contains(TEXT("Paragon")))
                    {
                        // 检查父类是否是AParagonFengMao
                        UClass* CurrentClass = CharClass;
                        while (CurrentClass)
                        {
                            if (CurrentClass == AParagonFengMao::StaticClass())
                            {
                                bIsEnemy = true;
                                UE_LOG(LogTemp, Warning, TEXT("    ✅ 这是封魔敌人（通过父类检查：%s）！"), *CurrentClass->GetName());
                                break;
                            }
                            CurrentClass = CurrentClass->GetSuperClass();
                        }

                        if (!bIsEnemy)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("    ⚠️ 类名包含FengMao但不是AParagonFengMao的子类！"));
                            UE_LOG(LogTemp, Warning, TEXT("    💡 请确保蓝图类的Parent Class是ParagonFengMao"));
                        }
                    }

                    if (bIsEnemy)
                    {
                        FoundEnemies.Add(Char);
                    }
                }
            }
        }

        if (FoundEnemies.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ 警告: 场景中没有找到任何封魔敌人(AParagonFengMao)！"));
            UE_LOG(LogTemp, Warning, TEXT("💡 提示: 请确保放置的是ParagonFengMao类或其蓝图子类"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("✅ 场景中有 %d 个封魔敌人"), FoundEnemies.Num());
            for (int32 i = 0; i < FoundEnemies.Num(); i++)
            {
                if (FoundEnemies[i])
                {
                    UE_LOG(LogTemp, Warning, TEXT("  🐺 敌人%d: %s (类: %s)"),
                        i + 1, *FoundEnemies[i]->GetName(), *FoundEnemies[i]->GetClass()->GetName());
                }
            }
        }
    }

    // 🔋 蓄力计时器逻辑
    if (bIsCharging && CurrentActionState == EWukongActionState::HeavyCharge) {
        CurrentChargeTime += DeltaTime;
        CurrentChargeTime = FMath::Min(CurrentChargeTime, MaxChargeTime);
    }
    else if (bIsCharging && CurrentActionState != EWukongActionState::HeavyCharge) {
        CancelHeavyCharge();
    }

    // 🛡️ 动画防护措施
    PreventAnimationBlueprintDivisionByZero();
    FixAnimationBlueprintVariables();

    // 🆕 更新伤害数字显示
    static float DamageNumberTimer = 0.f;
    DamageNumberTimer += DeltaTime;
    if (DamageNumberTimer >= 0.5f && DamageNumberValues.Num() > 0)
    {
        DamageNumberTimer = 0.f;
        // 清除过期的伤害数字
        DamageNumberValues.Empty();
        DamageNumberLocations.Empty();
        DamageNumberIsCritical.Empty();
    }
}

// ==========================================
// C++ 逻辑实现：替代蓝图 Switch 逻辑
// ==========================================

void AWukongCharacter::ExecuteAttackLogic(int32 NewAtttackMyCount, FName SectionName)
{
    UE_LOG(LogTemp, Warning, TEXT("⚡ ===== EXECUTE ATTACK LOGIC ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 参数: AttackCount=%d, Section=%s"), NewAtttackMyCount, *SectionName.ToString());

    bool OldAttacking = bMyIsAttacking;
    bMyIsAttacking = true;
    UE_LOG(LogTemp, Warning, TEXT("📊 攻击状态更新: %d → %d"), OldAttacking, bMyIsAttacking);

    int32 OldCount = AtttackMyCount;
    AtttackMyCount = NewAtttackMyCount;
    UE_LOG(LogTemp, Warning, TEXT("🔢 连击计数更新: %d → %d"), OldCount, AtttackMyCount);

    UAnimMontage* MontageToPlay = nullptr;

    switch (AtttackMyCount) {
    case 1:
        MontageToPlay = Attack1Montage;
        UE_LOG(LogTemp, Warning, TEXT("🎯 选择播放Attack1动画"));
        break;
    case 2:
        MontageToPlay = Attack2Montage;
        UE_LOG(LogTemp, Warning, TEXT("🎯 选择播放Attack2动画"));
        break;
    case 3:
        MontageToPlay = Attack3Montage;
        UE_LOG(LogTemp, Warning, TEXT("🎯 选择播放Attack3动画"));
        break;
    case 4:
        MontageToPlay = Attack4Montage;
        UE_LOG(LogTemp, Warning, TEXT("🎯 选择播放Attack4动画"));
        break;
    case 0:
        UE_LOG(LogTemp, Warning, TEXT("🔄 连击重置，不播放动画"));
        MontageToPlay = nullptr;
        break;
    default:
        UE_LOG(LogTemp, Error, TEXT("❌ 无效的AttackCount: %d"), AtttackMyCount);
        MontageToPlay = nullptr;
        break;
    }

    if (MontageToPlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎬 播放动画: %s"), *GetNameSafe(MontageToPlay));

        if (PlayMontageSafe(MontageToPlay, 1.5f, SectionName))
        {
            UE_LOG(LogTemp, Warning, TEXT("✅ 动画播放成功"));

            GetWorldTimerManager().SetTimer(
                DodgeTimerHandle,
                [this]() {
                    // 🆕 使用新的碰撞检测系统
                    TArray<AActor*> Enemies = DetectEnemiesInRange(DefaultAttackRadius, EAttackDetectionType::Sector);
                    if (Enemies.Num() > 0)
                    {
                        float Damage = LightAttackDamage;
                        bool bIsCritical = (FMath::FRand() < CriticalHitChance);
                        if (bIsCritical)
                        {
                            Damage *= CriticalHitMultiplier;
                            UE_LOG(LogTemp, Warning, TEXT("💥 暴击！伤害倍率: %.1f"), CriticalHitMultiplier);
                        }
                        ApplyDamageToDetectedEnemies(Enemies, Damage, false);
                    }
                },
                0.3f,
                false
            );

            float ComboWindowDuration = 0.5f;
            GetWorldTimerManager().SetTimer(
                ComboWindowTimerHandle,
                this,
                &AWukongCharacter::ResetAttackState,
                ComboWindowDuration,
                false
            );

            UE_LOG(LogTemp, Warning, TEXT("⏰ 缓冲窗口设置: %.1f秒"), ComboWindowDuration);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ 动画播放失败"));
        }
    }
    else if (AtttackMyCount != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ 没有找到对应的动画文件"));
    }

    UE_LOG(LogTemp, Warning, TEXT("⚡ ===== ATTACK LOGIC EXECUTED =====\n"));
}

void AWukongCharacter::ResetAttackState()
{
    if (PrimaryMeleeMontage)
    {
        UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
        if (AnimInst && AnimInst->Montage_IsPlaying(PrimaryMeleeMontage))
            return;
    }

    if (bLightAttackQueued)
        return;

    bMyIsAttacking = false;
    AtttackMyCount = 0;
    GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle);
}

FName AWukongCharacter::GetSectionNameForComboIndex(int32 ComboIndex)
{
    switch (ComboIndex)
    {
    case 0: return FName(TEXT("Attack1"));
    case 1: return FName(TEXT("Attack2"));
    case 2: return FName(TEXT("Attack3"));
    case 3: return FName(TEXT("Attack4"));
    default: return FName(TEXT("Attack1"));
    }
}

void AWukongCharacter::PlayLightAttackMontage(int32 ComboIndex)
{
    UE_LOG(LogTemp, Warning, TEXT("🎬 ===== PLAY ATTACK MONTAGE ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 参数: ComboIndex=%d"), ComboIndex);

    UAnimMontage* SelectedMontage = nullptr;
    int32 AttackCount = ComboIndex + 1;

    switch (ComboIndex) {
    case 0:
        SelectedMontage = Attack1Montage;
        UE_LOG(LogTemp, Warning, TEXT("🎯 选择Attack1动画文件"));
        break;
    case 1:
        SelectedMontage = Attack2Montage;
        UE_LOG(LogTemp, Warning, TEXT("🎯 选择Attack2动画文件"));
        break;
    case 2:
        SelectedMontage = Attack3Montage;
        UE_LOG(LogTemp, Warning, TEXT("🎯 选择Attack3动画文件"));
        break;
    case 3:
        SelectedMontage = Attack4Montage;
        AttackCount = 4;
        UE_LOG(LogTemp, Warning, TEXT("🎯 选择Attack4动画文件（终结技）"));
        break;
    }

    if (SelectedMontage) {
        ExecuteAttackLogic(AttackCount, FName(TEXT("Default")));
        UE_LOG(LogTemp, Warning, TEXT("✅ 动画文件选择成功"));
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("❌ 找不到对应的动画文件！ComboIndex=%d"), ComboIndex);
    }

    UE_LOG(LogTemp, Warning, TEXT("🎬 ===== MONTAGE PLAY REQUESTED =====\n"));
}
// ==========================================
// 🎮 输入系统实现 - 处理玩家的按键输入
// ==========================================

void AWukongCharacter::Move(const FInputActionValue& Value) {
    FVector2D MovementVector = Value.Get<FVector2D>();
    LastMovementInput = MovementVector;

    if (Controller) {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AWukongCharacter::Look(const FInputActionValue& Value) {
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller) {
        AddControllerYawInput(LookAxisVector.X);
        FRotator CurrentRotation = Controller->GetControlRotation();
        float NewPitch = CurrentRotation.Pitch + LookAxisVector.Y;
        NewPitch = FMath::ClampAngle(NewPitch, -80.0f, 45.0f);
        FRotator NewRotation = CurrentRotation;
        NewRotation.Pitch = NewPitch;
        Controller->SetControlRotation(NewRotation);
    }
}

// ==========================================
// ⚔️ 战斗与动作逻辑 - 冲刺、闪避、重攻击等功能
// ==========================================

void AWukongCharacter::SprintStart() {
    UE_LOG(LogTemp, Warning, TEXT("SprintStart Triggered"));
    CurrentActionState = EWukongActionState::Sprint;
    GetCharacterMovement()->MaxWalkSpeed = 1000.0f;
}

void AWukongCharacter::SprintStop() {
    UE_LOG(LogTemp, Warning, TEXT("SprintStop Triggered"));
    CurrentActionState = EWukongActionState::Idle;
    GetCharacterMovement()->MaxWalkSpeed = 500.0f;
}

void AWukongCharacter::Dodge() {
    if (CurrentActionState == EWukongActionState::Dodge || !bCanDodge) {
        UE_LOG(LogTemp, Warning, TEXT("Dodge blocked: already dodging or cooling down"));
        return;
    }

    if (CurrentStamina < DodgeStaminaCost) {
        UE_LOG(LogTemp, Warning, TEXT("Dodge blocked: stamina low"));
        return;
    }

    bool bIsInAir = GetCharacterMovement()->IsFalling();
    float ActualDodgeDistance = DodgeDistance;
    if (bIsInAir) {
        ActualDodgeDistance *= 0.5f;
        UE_LOG(LogTemp, Warning, TEXT("Dodge in air: distance reduced to %.1f"), ActualDodgeDistance);
    }
    UseSkill(DodgeStaminaCost);
    //CurrentStamina -= DodgeStaminaCost;
    if (CurrentStamina < DodgeStaminaCost) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 闪避被阻止: 体力不足"));
        return;
    }
    if (GetMesh() && GetMesh()->GetAnimInstance()) {
        UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
        AnimInst->StopAllMontages(0.1f);
        GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle);
        GetWorldTimerManager().ClearTimer(DodgeTimerHandle);
        GetWorldTimerManager().ClearTimer(DodgeCooldownTimerHandle);
    }

    bMyIsAttacking = false;
    bLightAttackQueued = false;
    CurrentLightComboIndex = 0;
    AtttackMyCount = 0;

    CurrentActionState = EWukongActionState::Dodge;
    bIsInvincible = true;
    bCanDodge = false;

    FVector DodgeDirection = GetActorForwardVector();

    if (!LastMovementInput.IsNearlyZero()) {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        DodgeDirection = (ForwardDirection * LastMovementInput.Y + RightDirection * LastMovementInput.X).GetSafeNormal();
    }

    if (DodgeMontage) {
        PlayMontageSafe(DodgeMontage, 1.5f);

        if (DodgeMontage->HasRootMotion()) {
            GetCharacterMovement()->SetMovementMode(MOVE_None);
        }
        else {
            FVector CurrentLocation = GetActorLocation();
            FVector DodgeTargetLocation = CurrentLocation + (DodgeDirection * ActualDodgeDistance);
            float DodgeMoveTime = 0.3f;
            FVector RequiredVelocity = (DodgeTargetLocation - CurrentLocation) / DodgeMoveTime;
            LaunchCharacter(RequiredVelocity, true, false);
        }
    }
    else {
        FVector CurrentLocation = GetActorLocation();
        FVector DodgeTargetLocation = CurrentLocation + (DodgeDirection * ActualDodgeDistance);
        float DodgeMoveTime = 0.07f;
        FVector RequiredVelocity = (DodgeTargetLocation - CurrentLocation) / DodgeMoveTime;
        LaunchCharacter(RequiredVelocity, true, false);
        UE_LOG(LogTemp, Warning, TEXT("Dodge without animation: Controlled move to target"));
    }

    GetWorldTimerManager().SetTimer(
        DodgeTimerHandle,
        [this]() {
            bIsInvincible = false;
            CurrentActionState = EWukongActionState::Idle;
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            UE_LOG(LogTemp, Warning, TEXT("Dodge ended, back to Idle"));
        },
        DodgeDuration,
        false
    );

    GetWorldTimerManager().SetTimer(
        DodgeCooldownTimerHandle,
        [this]() {
            bCanDodge = true;
            UE_LOG(LogTemp, Warning, TEXT("Dodge cooldown ended, can dodge again"));
        },
        DodgeCooldown,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("Dodge executed: HasAnimation=%s Direction=%.2f,%.2f,%.2f Stamina=%.1f"),
        DodgeMontage ? TEXT("Yes") : TEXT("No"),
        DodgeDirection.X, DodgeDirection.Y, DodgeDirection.Z, CurrentStamina);
}

// ==========================================
// 战斗系统 - 伤害、死亡、重生
// ==========================================

float AWukongCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead)
    {
        return 0.f;
    }

    float ActualDamage = DamageAmount;
    ReceiveDamage(ActualDamage);
    CurrentHealth = FMath::Max(CurrentHealth, 0.f);

    UE_LOG(LogTemp, Warning, TEXT("💥 受到伤害: %.1f, 剩余生命值: %.1f/%.1f"),
        ActualDamage, CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.f && !bIsDead)
    {
        Die();
    }

    return ActualDamage;
}

void AWukongCharacter::Die()
{
    if (bIsDead)
    {
        return;
    }

    bIsDead = true;
    CurrentActionState = EWukongActionState::Idle;

    UE_LOG(LogTemp, Warning, TEXT("💀 角色死亡！生命值: %.1f"), CurrentHealth);

    if (GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->StopAllMontages(0.1f);
    }

    GetCharacterMovement()->DisableMovement();

    GetWorldTimerManager().SetTimer(
        DodgeTimerHandle,
        [this]() {
            Respawn();
        },
        3.f,
        false
    );
}

void AWukongCharacter::Respawn()
{
    UE_LOG(LogTemp, Warning, TEXT("🔄 角色重生！"));

    bIsDead = false;
    CurrentHealth = MaxHealth;
    CurrentStamina = MaxStamina;
    CurrentActionState = EWukongActionState::Idle;

    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    SetActorLocation(FVector(0.f, 0.f, 100.f));

    SetActorRotation(FRotator::ZeroRotator);

    UE_LOG(LogTemp, Warning, TEXT("✅ 重生完成，生命值: %.1f/%.1f"), CurrentHealth, MaxHealth);
}

void AWukongCharacter::ApplyDamageToTarget(float Damage, AActor* Target)
{
    if (!Target)
    {
        return;
    }

    FDamageEvent DamageEvent;
    FHitResult HitResult;
    FPointDamageEvent PointDamageEvent(Damage, HitResult, GetActorForwardVector(), nullptr);

    Target->TakeDamage(Damage, PointDamageEvent, GetController(), this);

    UE_LOG(LogTemp, Warning, TEXT("⚔️ 对目标造成伤害: %.1f"), Damage);
}

void AWukongCharacter::PerformHeavyAttackDamageDetection(float Damage, FVector AttackDirection)
{
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== 重攻击伤害检测开始 ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 伤害数值: %.1f"), Damage);

    // 🆕 使用新的碰撞检测系统
    TArray<AActor*> Enemies = DetectEnemiesInRange(
        DefaultAttackRadius * HeavyAttackRadiusMultiplier,
        EAttackDetectionType::Circle
    );

    if (Enemies.Num() > 0)
    {
        bool bIsCritical = (FMath::FRand() < CriticalHitChance);
        float FinalDamage = Damage;
        if (bIsCritical)
        {
            FinalDamage *= CriticalHitMultiplier;
            UE_LOG(LogTemp, Warning, TEXT("💥 重攻击暴击！伤害倍率: %.1f"), CriticalHitMultiplier);
        }
        ApplyDamageToDetectedEnemies(Enemies, FinalDamage, true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ ❌ 前方没有目标被打中"));
    }

    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== 重攻击伤害检测结束 =====\n"));
}

void AWukongCharacter::PerformLightAttackDamageDetection()
{
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== 轻攻击伤害检测开始 ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 伤害数值: %.1f"), LightAttackDamage);

    // 🆕 使用新的碰撞检测系统
    TArray<AActor*> Enemies = DetectEnemiesInRange(DefaultAttackRadius, EAttackDetectionType::Sector);

    if (Enemies.Num() > 0)
    {
        bool bIsCritical = (FMath::FRand() < CriticalHitChance);
        float FinalDamage = LightAttackDamage;
        if (bIsCritical)
        {
            FinalDamage *= CriticalHitMultiplier;
            UE_LOG(LogTemp, Warning, TEXT("💥 轻攻击暴击！伤害倍率: %.1f"), CriticalHitMultiplier);
        }
        ApplyDamageToDetectedEnemies(Enemies, FinalDamage, false);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ ❌ 前方没有目标被打中"));
    }

    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== 轻攻击伤害检测结束 =====\n"));
}
// ==========================================
// 测试功能 - 调试和测试用
// ==========================================

void AWukongCharacter::TestTakeDamage()
{
    UE_LOG(LogTemp, Warning, TEXT("🩹 ===== TEST TAKE DAMAGE TRIGGERED ====="));
    UE_LOG(LogTemp, Warning, TEXT("🔍 函数被调用了！时间: %.2f"), GetWorld()->GetTimeSeconds());

    float TestDamage = 30.f;
    FDamageEvent DamageEvent;
    FHitResult HitResult;
    FPointDamageEvent PointDamageEvent(TestDamage, HitResult, GetActorForwardVector(), nullptr);

    TakeDamage(TestDamage, PointDamageEvent, nullptr, nullptr);

    UE_LOG(LogTemp, Warning, TEXT("✅ 测试伤害应用完成: %.1f 伤害"), TestDamage);
}

void AWukongCharacter::TestDie()
{
    UE_LOG(LogTemp, Warning, TEXT("💀 ===== TEST DIE ====="));

    if (!bIsDead)
    {
        CurrentHealth = 0.f;
        Die();
        UE_LOG(LogTemp, Warning, TEXT("✅ 测试死亡触发"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 已经死亡，无法再次测试死亡"));
    }
}

void AWukongCharacter::TestRespawn()
{
    UE_LOG(LogTemp, Warning, TEXT("🔄 ===== TEST RESPAWN ====="));

    if (bIsDead)
    {
        GetWorldTimerManager().ClearTimer(DodgeTimerHandle);
        Respawn();
        UE_LOG(LogTemp, Warning, TEXT("✅ 测试重生触发"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 未死亡，无法测试重生"));
    }
}

void AWukongCharacter::TestFrontDetection()
{
    UE_LOG(LogTemp, Warning, TEXT("👁️ ===== TEST FRONT DETECTION ====="));

    FVector StartLocation = GetActorLocation() + FVector(0, 0, 50);
    FVector EndLocation = StartLocation + (GetActorForwardVector() * 500.f);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Pawn,
        QueryParams
    );

    DrawDebugLine(GetWorld(), StartLocation, EndLocation,
        bHit ? FColor::Green : FColor::Red, false, 3.f, 0, 3.f);

    if (bHit)
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor)
        {
            float Distance = FVector::Distance(StartLocation, HitResult.Location);
            UE_LOG(LogTemp, Warning, TEXT("🎯 前方检测成功: 目标=%s, 距离=%.1f米, 位置=(%.1f,%.1f,%.1f)"),
                *HitActor->GetName(), Distance / 100.f,
                HitResult.Location.X, HitResult.Location.Y, HitResult.Location.Z);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ 前方检测失败: 未找到任何目标"));
    }
}

void AWukongCharacter::StartHeavyCharge() {
    UE_LOG(LogTemp, Warning, TEXT("🔋 ===== START HEAVY CHARGE ====="));

    if (CurrentActionState != EWukongActionState::Idle) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 无法开始蓄力：当前状态不是空闲"));
        return;
    }

    if (CurrentStamina < HeavyAttackStaminaCost) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 无法开始蓄力：体力不足"));
        return;
    }

    CurrentActionState = EWukongActionState::HeavyCharge;
    bIsCharging = true;
    CurrentChargeTime = 0.f;

    UE_LOG(LogTemp, Warning, TEXT("✅ 开始重攻击蓄力"));
}

void AWukongCharacter::ReleaseHeavyAttack() {
    UE_LOG(LogTemp, Warning, TEXT("💥 ===== RELEASE HEAVY ATTACK ====="));

    if (!bIsCharging || CurrentActionState != EWukongActionState::HeavyCharge) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 没有在蓄力状态，忽略释放"));
        return;
    }

    float ChargeRatio = FMath::Min(CurrentChargeTime / MaxChargeTime, 1.f);
    float DamageMultiplier = FMath::Lerp(MinChargeDamageMultiplier, MaxChargeDamageMultiplier, ChargeRatio);

    UE_LOG(LogTemp, Warning, TEXT("📊 蓄力时间: %.2fs, 倍率: %.2f"), CurrentChargeTime, DamageMultiplier);

    bIsCharging = false;
    CurrentActionState = EWukongActionState::Idle;

    ExecuteHeavyAttack(DamageMultiplier);

    UE_LOG(LogTemp, Warning, TEXT("✅ ===== HEAVY CHARGE RELEASED =====\n"));
}

void AWukongCharacter::CancelHeavyCharge() {
    if (bIsCharging && CurrentActionState == EWukongActionState::HeavyCharge) {
        UE_LOG(LogTemp, Warning, TEXT("❌ ===== CANCEL HEAVY CHARGE ====="));
        UE_LOG(LogTemp, Warning, TEXT("📊 取消蓄力，蓄力时间: %.2fs"), CurrentChargeTime);

        bIsCharging = false;
        CurrentChargeTime = 0.f;
        CurrentActionState = EWukongActionState::Idle;

        UE_LOG(LogTemp, Warning, TEXT("✅ 蓄力已取消"));
    }
}

void AWukongCharacter::ExecuteHeavyAttack(float DamageMultiplier) {
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== EXECUTE HEAVY ATTACK ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 伤害倍率: %.2f"), DamageMultiplier);

    if (CurrentActionState == EWukongActionState::HeavyAttack ||
        CurrentActionState == EWukongActionState::Dodge) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 重攻击被阻止: 当前状态不适合"));
        return;
    }



    //CurrentStamina -= HeavyAttackStaminaCost;
    UseSkill(HeavyAttackStaminaCost);
    useskillslot();
    // 检查冷却和体力
  
    if (GetMesh() && GetMesh()->GetAnimInstance()) {
        UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
        AnimInst->StopAllMontages(0.1f);
        GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle);
    }

    bMyIsAttacking = false;
    bLightAttackQueued = false;
    CurrentLightComboIndex = 0;
    AtttackMyCount = 0;

    CurrentActionState = EWukongActionState::HeavyAttack;
    bMyIsAttacking = true;

    float ActualDamage = HeavyAttackBaseDamage * DamageMultiplier;
    UE_LOG(LogTemp, Warning, TEXT("⚔️ 重攻击伤害: 基础%.1f × 倍率%.2f = 实际%.1f"),
        HeavyAttackBaseDamage, DamageMultiplier, ActualDamage);

    if (HeavyAttackMontage) {
        PlayMontageSafe(HeavyAttackMontage, 0.7f, FName(TEXT("Default")));
        UE_LOG(LogTemp, Warning, TEXT("🎬 播放重攻击动画"));
        UE_LOG(LogTemp, Warning, TEXT("🎯 重击动画资源: %s"), *GetNameSafe(HeavyAttackMontage));
        UE_LOG(LogTemp, Warning, TEXT("📁 重击动画路径: %s"), *HeavyAttackMontage->GetPathName());
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("❌ HeavyAttackMontage 未加载！"));
    }

    FVector AttackDirection = GetActorForwardVector();

    if (!LastMovementInput.IsNearlyZero() && Controller) {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AttackDirection = (ForwardDirection * LastMovementInput.Y + RightDirection * LastMovementInput.X).GetSafeNormal();
        UE_LOG(LogTemp, Warning, TEXT("📊 使用输入方向进行重攻击突进"));
    }

    FVector CurrentLocation = GetActorLocation();
    FVector AttackTargetLocation = CurrentLocation + (AttackDirection * HeavyAttackDistance);

    FHitResult HitResult;
    FVector TraceStart = AttackTargetLocation + FVector(0, 0, 100);
    FVector TraceEnd = AttackTargetLocation - FVector(0, 0, 500);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic)) {
        AttackTargetLocation.Z = HitResult.Location.Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        UE_LOG(LogTemp, Warning, TEXT("🌍 检测到地面，调整高度到: %.1f"), AttackTargetLocation.Z);
    }

    FVector RequiredVelocity = (AttackTargetLocation - CurrentLocation) / HeavyAttackDuration;

    LaunchCharacter(RequiredVelocity, true, true);

    UE_LOG(LogTemp, Warning, TEXT("🚀 重攻击突进: 方向(%.2f,%.2f,%.2f), 距离=%.1f, 速度=%.1f"),
        AttackDirection.X, AttackDirection.Y, AttackDirection.Z,
        HeavyAttackDistance, RequiredVelocity.Size());

    GetWorldTimerManager().SetTimer(
        ComboWindowTimerHandle,
        [this, ActualDamage, AttackDirection]() {
            PerformHeavyAttackDamageDetection(ActualDamage, AttackDirection);
        },
        0.2f,
        false
    );

    GetWorldTimerManager().SetTimer(
        DodgeTimerHandle,
        [this]() {
            if (CurrentActionState == EWukongActionState::HeavyAttack) {
                CurrentActionState = EWukongActionState::Idle;
                bMyIsAttacking = false;
                UE_LOG(LogTemp, Warning, TEXT("🏁 重攻击结束，回到空闲状态"));
            }
        },
        HeavyAttackDuration,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("✅ ===== HEAVY ATTACK EXECUTED =====\n"));
}

// ========================================== 
// 轻攻击连击逻辑（修改播放逻辑）
// ========================================== 

void AWukongCharacter::LightAttack()
{
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== LIGHT ATTACK TRIGGERED ====="));

    if (!GetMesh() || !PrimaryMeleeMontage) {
        UE_LOG(LogTemp, Error, TEXT("❌ 轻攻击失败: Mesh=%d, Montage=%d"),
            GetMesh() != nullptr, PrimaryMeleeMontage != nullptr);
        return;
    }

    UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
    if (!AnimInst) {
        UE_LOG(LogTemp, Error, TEXT("❌ 轻攻击失败: AnimInstance不存在"));
        return;
    }

    bool IsPlayingAttack = false;
    FName CurrentSection = NAME_None;

    if (Attack1Montage && AnimInst->Montage_IsPlaying(Attack1Montage)) {
        IsPlayingAttack = true;
        CurrentSection = FName(TEXT("Attack1"));
    }
    else if (Attack2Montage && AnimInst->Montage_IsPlaying(Attack2Montage)) {
        IsPlayingAttack = true;
        CurrentSection = FName(TEXT("Attack2"));
    }
    else if (Attack3Montage && AnimInst->Montage_IsPlaying(Attack3Montage)) {
        IsPlayingAttack = true;
        CurrentSection = FName(TEXT("Attack3"));
    }
    else if (Attack4Montage && AnimInst->Montage_IsPlaying(Attack4Montage)) {
        IsPlayingAttack = true;
        CurrentSection = FName(TEXT("Attack4"));
    }
    else if (HeavyAttackMontage && AnimInst->Montage_IsPlaying(HeavyAttackMontage)) {
        IsPlayingAttack = true;
        CurrentSection = FName(TEXT("HeavyAttack"));
    }

    if (!IsPlayingAttack && PrimaryMeleeMontage && AnimInst->Montage_IsPlaying(PrimaryMeleeMontage)) {
        IsPlayingAttack = true;
        CurrentSection = AnimInst->Montage_GetCurrentSection(PrimaryMeleeMontage);
    }

    UE_LOG(LogTemp, Warning, TEXT("📊 当前状态: 正在攻击=%d, 当前段落=%s, 连击索引=%d, 缓冲中=%d"),
        IsPlayingAttack, *CurrentSection.ToString(), CurrentLightComboIndex, bLightAttackQueued);

    if (IsPlayingAttack)
    {
        bLightAttackQueued = true;
        float CurrentPos = AnimInst->Montage_GetPosition(PrimaryMeleeMontage);
        float TotalLength = PrimaryMeleeMontage->GetPlayLength();
        float Progress = TotalLength > 0.0f ? (CurrentPos / TotalLength) * 100.0f : 0.0f;

        UE_LOG(LogTemp, Warning, TEXT("✅ 【连击缓冲触发】段落:%s, 进度:%.1f%%, 位置:%.2fs/%.2fs"),
            *CurrentSection.ToString(), Progress, CurrentPos, TotalLength);
        UE_LOG(LogTemp, Warning, TEXT("🔄 缓冲状态已设置，等待当前攻击结束"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("🚀 【开始新连击】重置连击状态"));
    CurrentLightComboIndex = 0;
    bLightAttackQueued = false;
    bMyIsAttacking = true;
    AtttackMyCount = 1;

    PlayLightAttackMontage(0);

    UE_LOG(LogTemp, Warning, TEXT("✅ 【连击启动】Attack1开始播放，等待动画结束"));
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== LIGHT ATTACK END =====\n"));
}

void AWukongCharacter::OnLightAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Warning, TEXT("🏁 ===== ANIMATION ENDED ====="));

    FName CurrentSection = NAME_None;
    if (Montage == Attack1Montage) {
        CurrentSection = FName(TEXT("Attack1"));
    }
    else if (Montage == Attack2Montage) {
        CurrentSection = FName(TEXT("Attack2"));
    }
    else if (Montage == Attack3Montage) {
        CurrentSection = FName(TEXT("Attack3"));
    }
    else if (Montage == Attack4Montage) {
        CurrentSection = FName(TEXT("Attack4"));
    }
    else if (Montage == HeavyAttackMontage) {
        CurrentSection = FName(TEXT("HeavyAttack"));
        CurrentActionState = EWukongActionState::Idle;
        UE_LOG(LogTemp, Warning, TEXT("🏁 重攻击动画结束"));
        return;
    }
    else {
        CurrentSection = GetSectionNameForComboIndex(CurrentLightComboIndex);
    }

    FString InterruptReason = bInterrupted ? TEXT("外部打断") : TEXT("正常结束");

    UE_LOG(LogTemp, Warning, TEXT("📊 结束信息: 动画=%s, 段落=%s (索引=%d), 原因=%s, 缓冲中=%d"),
        *GetNameSafe(Montage), *CurrentSection.ToString(), CurrentLightComboIndex, *InterruptReason, bLightAttackQueued);

    if (bInterrupted)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ 【连击被打断】可能是闪避或其他动作中断"));
        UE_LOG(LogTemp, Warning, TEXT("🧹 清理连击状态，重置所有变量"));
        ClearComboQueue();
        UE_LOG(LogTemp, Warning, TEXT("🏁 ===== ANIMATION INTERRUPTED =====\n"));
        return;
    }

    if (bLightAttackQueued)
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ 【连击继续】检测到缓冲的攻击，开始处理"));

        bLightAttackQueued = false;
        int32 PreviousIndex = CurrentLightComboIndex;
        CurrentLightComboIndex++;

        if (CurrentLightComboIndex >= 4) {
            UE_LOG(LogTemp, Warning, TEXT("🎉 【连击完成】Attack4结束，连击序列完成"));
            ClearComboQueue();
            UE_LOG(LogTemp, Warning, TEXT("🏁 ===== COMBO SEQUENCE COMPLETE =====\n"));
            return;
        }
        else {
            UE_LOG(LogTemp, Warning, TEXT("➡️ 【连击递进】%d → %d"), PreviousIndex, CurrentLightComboIndex);
        }

        int32 NextAttackNumber = CurrentLightComboIndex + 1;
        UE_LOG(LogTemp, Warning, TEXT("🎯 【执行下一击】播放Attack%d"), NextAttackNumber);
        PlayLightAttackMontage(CurrentLightComboIndex);

        UE_LOG(LogTemp, Warning, TEXT("✅ 【连击继续完成】等待下一段动画结束"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🛑 【连击结束】没有缓冲的攻击，序列完成"));
        ClearComboQueue();
        UE_LOG(LogTemp, Warning, TEXT("🎉 【连击序列完成】所有攻击播放完毕"));
    }

    UE_LOG(LogTemp, Warning, TEXT("🏁 ===== ANIMATION END PROCESSED =====\n"));
}

void AWukongCharacter::ClearComboQueue()
{
    UE_LOG(LogTemp, Warning, TEXT("🧹 ===== CLEAR COMBO QUEUE ====="));

    bool WasQueued = bLightAttackQueued;
    int32 WasComboIndex = CurrentLightComboIndex;
    bool WasAttacking = bMyIsAttacking;
    int32 WasAttackCount = AtttackMyCount;

    UE_LOG(LogTemp, Warning, TEXT("📋 清理前状态: 缓冲=%d, 连击索引=%d, 攻击中=%d, 计数=%d"),
        WasQueued, WasComboIndex, WasAttacking, WasAttackCount);

    bLightAttackQueued = false;
    CurrentLightComboIndex = 0;
    bMyIsAttacking = false;
    AtttackMyCount = 0;
    if (CurrentActionState != EWukongActionState::HeavyAttack) {
        CurrentActionState = EWukongActionState::Idle;
    }

    UE_LOG(LogTemp, Warning, TEXT("🔄 状态已重置: 缓冲=false, 索引=0, 攻击=false, 计数=0"));

    GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle);
    UE_LOG(LogTemp, Warning, TEXT("⏰ 连击缓冲计时器已清理"));

    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
    {
        if (CurrentActionState != EWukongActionState::HeavyAttack) {
            bool WasPlaying = AnimInst->Montage_IsPlaying(PrimaryMeleeMontage);
            AnimInst->Montage_Stop(0.0f, PrimaryMeleeMontage);
            UE_LOG(LogTemp, Warning, TEXT("🎬 动画停止: 之前播放中=%d, 现在已停止"), WasPlaying);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ AnimInstance不存在，跳过动画停止"));
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ 连击清理完成 - 所有状态已重置"));
    UE_LOG(LogTemp, Warning, TEXT("🧹 ===== COMBO QUEUE CLEARED =====\n"));
}
// ========================================== 
// PossessedBy / BeginPlay / Input binding 等
// ========================================== 

void AWukongCharacter::PossessedBy(AController* NewController) {
    Super::PossessedBy(NewController);
    UE_LOG(LogTemp, Warning, TEXT("PossessedBy called. Controller=%s"), *GetNameSafe(NewController));
    if (APlayerController* PC = Cast<APlayerController>(NewController)) {
        if (ULocalPlayer* LP = PC->GetLocalPlayer()) {
            if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP)) {
                Subsystem->ClearAllMappings();
                if (!InputMappingContext || InputMappingContext->GetMappings().Num() < 11) {
                    UE_LOG(LogTemp, Warning, TEXT("Rebuilding Runtime IMC in PossessedBy (PrevCount=%d)"), InputMappingContext ? InputMappingContext->GetMappings().Num() : -1);
                    InputMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Wukong_Runtime_Ensured"));
                    InputMappingContext->MapKey(MoveAction, EKeys::D);
                    {
                        auto& AMap = InputMappingContext->MapKey(MoveAction, EKeys::A);
                        AMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));
                    }
                    {
                        auto& WMap = InputMappingContext->MapKey(MoveAction, EKeys::W);
                        auto* Swz = NewObject<UInputModifierSwizzleAxis>(this);
                        Swz->Order = EInputAxisSwizzle::YXZ;
                        WMap.Modifiers.Add(Swz);
                    }
                    {
                        auto& SMap = InputMappingContext->MapKey(MoveAction, EKeys::S);
                        auto* Swz = NewObject<UInputModifierSwizzleAxis>(this);
                        Swz->Order = EInputAxisSwizzle::YXZ;
                        SMap.Modifiers.Add(Swz);
                        SMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));
                    }
                    InputMappingContext->MapKey(LookAction, EKeys::MouseX);
                    {
                        auto& MY = InputMappingContext->MapKey(LookAction, EKeys::MouseY);
                        auto* SwzY = NewObject<UInputModifierSwizzleAxis>(this);
                        SwzY->Order = EInputAxisSwizzle::YXZ;
                        MY.Modifiers.Add(SwzY);
                    }
                    InputMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
                    InputMappingContext->MapKey(LightAttackAction, EKeys::LeftMouseButton);
                    InputMappingContext->MapKey(SprintAction, EKeys::LeftShift);
                    InputMappingContext->MapKey(DodgeAction, EKeys::LeftControl);
                    InputMappingContext->MapKey(HeavyAttackAction, EKeys::RightMouseButton);
                    InputMappingContext->MapKey(ToggleInvisibilityAction, EKeys::V);
                    InputMappingContext->MapKey(TransformAction, EKeys::R); // 🌀 变身按键
                }
                Subsystem->AddMappingContext(InputMappingContext, 100);
                UE_LOG(LogTemp, Warning, TEXT("Runtime IMC applied in PossessedBy. MappingCount=%d"), InputMappingContext->GetMappings().Num());
            }
        }
    }
}

void AWukongCharacter::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("BeginPlay Triggered"));

    if (IsLocallyControlled() && PlayerWidgetClass != nullptr)
    {
        MyPlayerHUD = CreateWidget<UMyPlayerWidget>(GetWorld(), PlayerWidgetClass);

        if (MyPlayerHUD)
        {
            MyPlayerHUD->AddToViewport();
            MyPlayerHUD->UpdateHealth(CurrentHealth, MaxHealth);
            MyPlayerHUD->UpdateMana(CurrentStamina, MaxStamina);
        }
    }
    if (MyPlayerHUD)
    {
        if (UInventoryWidget* Inv = MyPlayerHUD->GetInventoryWidget())
        {
            Inv->AddItem(EItemType::HealthPotion, TEXT("Health Potion"), 5);
            Inv->AddItem(EItemType::ManaPotion, TEXT("Mana Potion"), 3);
        }
    }

    SaveOriginalMaterials();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
            {
                Subsystem->ClearAllMappings();

                if (!InputMappingContext || InputMappingContext->GetMappings().Num() < 15)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Rebuilding Runtime IMC in BeginPlay (PrevCount=%d)"),
                        InputMappingContext ? InputMappingContext->GetMappings().Num() : -1);

                    InputMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Wukong_Runtime_Ensured"));

                    InputMappingContext->MapKey(MoveAction, EKeys::D);
                    {
                        auto& AMap = InputMappingContext->MapKey(MoveAction, EKeys::A);
                        AMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));
                    }
                    {
                        auto& WMap = InputMappingContext->MapKey(MoveAction, EKeys::W);
                        auto* Swz = NewObject<UInputModifierSwizzleAxis>(this);
                        Swz->Order = EInputAxisSwizzle::YXZ;
                        WMap.Modifiers.Add(Swz);
                    }
                    {
                        auto& SMap = InputMappingContext->MapKey(MoveAction, EKeys::S);
                        auto* Swz = NewObject<UInputModifierSwizzleAxis>(this);
                        Swz->Order = EInputAxisSwizzle::YXZ;
                        SMap.Modifiers.Add(Swz);
                        SMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));
                    }

                    InputMappingContext->MapKey(LookAction, EKeys::MouseX);
                    {
                        auto& MY = InputMappingContext->MapKey(LookAction, EKeys::MouseY);
                        auto* SwzY = NewObject<UInputModifierSwizzleAxis>(this);
                        SwzY->Order = EInputAxisSwizzle::YXZ;
                        MY.Modifiers.Add(SwzY);
                    }

                    InputMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
                    InputMappingContext->MapKey(LightAttackAction, EKeys::LeftMouseButton);
                    InputMappingContext->MapKey(SprintAction, EKeys::LeftShift);
                    InputMappingContext->MapKey(DodgeAction, EKeys::LeftControl);
                    InputMappingContext->MapKey(HeavyAttackAction, EKeys::RightMouseButton);
                    InputMappingContext->MapKey(ToggleInvisibilityAction, EKeys::V);
                    InputMappingContext->MapKey(TransformAction, EKeys::R); // 🌀 变身按键
                }

                Subsystem->AddMappingContext(InputMappingContext, 100);
                UE_LOG(LogTemp, Warning, TEXT("Runtime IMC applied in BeginPlay. MappingCount=%d"),
                    InputMappingContext->GetMappings().Num());
            }
        }
    }

    bMyIsAttacking = false;
    bLightAttackQueued = false;
    CurrentLightComboIndex = 0;
    AtttackMyCount = 0;
    CurrentActionState = EWukongActionState::Idle;

    if (PrimaryMeleeMontage) {
        UE_LOG(LogTemp, Warning, TEXT("📋 动画资源检查: %s"), *GetNameSafe(PrimaryMeleeMontage));
        UE_LOG(LogTemp, Warning, TEXT("📊 总段落数: %d"), PrimaryMeleeMontage->CompositeSections.Num());

        for (int32 i = 0; i < PrimaryMeleeMontage->CompositeSections.Num(); i++) {
            FCompositeSection& Section = PrimaryMeleeMontage->CompositeSections[i];
            UE_LOG(LogTemp, Warning, TEXT("  段落%d: %s"),
                i, *Section.SectionName.ToString());
        }

        TArray<FName> RequiredSections = { FName(TEXT("Attack1")), FName(TEXT("Attack2")),
                                         FName(TEXT("Attack3")), FName(TEXT("Attack4")) };
        for (FName RequiredSection : RequiredSections) {
            bool Found = false;
            for (FCompositeSection& Section : PrimaryMeleeMontage->CompositeSections) {
                if (Section.SectionName == RequiredSection) {
                    Found = true;
                    break;
                }
            }
            if (Found) {
                UE_LOG(LogTemp, Warning, TEXT("✅ 必需段落存在: %s"), *RequiredSection.ToString());
            }
            else {
                UE_LOG(LogTemp, Error, TEXT("❌ 必需段落缺失: %s"), *RequiredSection.ToString());
            }
        }
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("❌ PrimaryMeleeMontage未加载！"));
    }

    if (HeavyAttackMontage) {
        UE_LOG(LogTemp, Warning, TEXT("✅ HeavyAttackMontage已加载: %s"), *GetNameSafe(HeavyAttackMontage));
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("❌ HeavyAttackMontage未加载！"));
    }

    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
    {
        AnimInst->Montage_Stop(0.0f);
    }

    UE_LOG(LogTemp, Warning, TEXT("C++防干扰连击初始化完成."));
}

void AWukongCharacter::OnRep_Controller() {
    Super::OnRep_Controller();
    if (APlayerController* PC = Cast<APlayerController>(Controller)) {
        if (ULocalPlayer* LP = PC->GetLocalPlayer()) {
            if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP)) {
                if (InputMappingContext) {
                    Subsystem->AddMappingContext(InputMappingContext, 0);
                    UE_LOG(LogTemp, Warning, TEXT("Enhanced Input IMC Activated in OnRep_Controller"));
                }
            }
        }
    }
}

// 播放蒙太奇安全检查
bool AWukongCharacter::PlayMontageSafe(UAnimMontage* Montage, float InPlayRate, FName StartSection)
{
    UE_LOG(LogTemp, Warning, TEXT("🛡️ ===== PLAY MONTAGE SAFE ====="));

    if (!Montage) {
        UE_LOG(LogTemp, Error, TEXT("❌ Montage为空"));
        return false;
    }
    if (!GetMesh()) {
        UE_LOG(LogTemp, Error, TEXT("❌ Mesh不存在"));
        return false;
    }

    UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
    if (!AnimInst) {
        UE_LOG(LogTemp, Error, TEXT("❌ AnimInstance不存在"));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("📊 播放参数: Montage=%s, PlayRate=%.1f, StartSection=%s"),
        *GetNameSafe(Montage), InPlayRate, *StartSection.ToString());

    if (StartSection != NAME_None) {
        AnimInst->Montage_JumpToSection(StartSection, Montage);
        UE_LOG(LogTemp, Warning, TEXT("🎯 已跳转到段落: %s"), *StartSection.ToString());
    }

    float Played = AnimInst->Montage_Play(Montage, InPlayRate);

    FName ActualSection = AnimInst->Montage_GetCurrentSection(Montage);
    float TotalLength = Montage->GetPlayLength();
    float ActualPlayTime = Played > 0.0f ? TotalLength / InPlayRate : 0.0f;

    UE_LOG(LogTemp, Warning, TEXT("🎬 播放结果: 返回值=%.2f, 当前段落=%s, 总时长=%.2fs, 播放时长≈%.2fs"),
        Played, *ActualSection.ToString(), TotalLength, ActualPlayTime);

    bool Success = Played > 0.0f;

    if (Success) {
        FOnMontageEnded MontageEndedDelegate;

        if (Montage == HeavyAttackMontage) {
            MontageEndedDelegate.BindLambda([this](UAnimMontage* EndedMontage, bool bInterrupted) {
                if (!bInterrupted) {
                    CurrentActionState = EWukongActionState::Idle;
                    UE_LOG(LogTemp, Warning, TEXT("🏁 重攻击动画正常结束"));
                }
                else {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ 重攻击动画被中断"));
                }
                });
        }
        else {
            MontageEndedDelegate.BindUObject(this, &AWukongCharacter::OnLightAttackMontageEnded);
        }

        AnimInst->Montage_SetEndDelegate(MontageEndedDelegate, Montage);
        UE_LOG(LogTemp, Warning, TEXT("🔗 已绑定动画结束回调: %s"), *GetNameSafe(Montage));
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ 播放%s"), Success ? TEXT("成功") : TEXT("失败"));
    UE_LOG(LogTemp, Warning, TEXT("🛡️ ===== MONTAGE PLAY COMPLETE =====\n"));

    return Success;
}

// ==========================================
// 修复动画蓝图变量以防止除零错误
// ==========================================

void AWukongCharacter::FixAnimationBlueprintVariables() {
    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance()) {
        if (PrimaryMeleeMontage && AnimInst->Montage_IsPlaying(PrimaryMeleeMontage)) {
            bMyIsAttacking = true;
        }
        else {
            bMyIsAttacking = false;
        }

        if (AtttackMyCount == 0 && bMyIsAttacking) {
            AtttackMyCount = 1;
        }

        AnimInst->SetMorphTarget(FName(TEXT("Speed")), FMath::Max(GetVelocity().Size(), 0.1f));
        AnimInst->SetMorphTarget(FName(TEXT("ComboIndex")), (float)CurrentLightComboIndex);
    }
}

void AWukongCharacter::PreventAnimationBlueprintDivisionByZero() {

}
// ==========================================
// 输入绑定 (SetupPlayerInputComponent)
// ==========================================

void AWukongCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AWukongCharacter::OnTogglePauseMenu);
    PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &AWukongCharacter::OnToggleInventory);
    if (!MoveAction) {
        MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"));
        MoveAction->ValueType = EInputActionValueType::Axis2D;
    }
    if (!LookAction) {
        LookAction = NewObject<UInputAction>(this, TEXT("IA_Look"));
        LookAction->ValueType = EInputActionValueType::Axis2D;
    }
    if (!JumpAction) {
        JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"));
    }
    if (!LightAttackAction) {
        LightAttackAction = NewObject<UInputAction>(this, TEXT("IA_Light"));
    }
    if (!SprintAction) {
        SprintAction = NewObject<UInputAction>(this, TEXT("IA_Sprint"));
    }
    if (!DodgeAction) {
        DodgeAction = NewObject<UInputAction>(this, TEXT("IA_Dodge"));
    }
    if (!HeavyAttackAction) {
        HeavyAttackAction = NewObject<UInputAction>(this, TEXT("IA_Heavy"));
    }
    if (!ToggleInvisibilityAction) {
        ToggleInvisibilityAction = NewObject<UInputAction>(this, TEXT("IA_Invisibility"));
        ToggleInvisibilityAction->ValueType = EInputActionValueType::Boolean;
    }
    if (!TransformAction) {
        TransformAction = NewObject<UInputAction>(this, TEXT("IA_Transform"));
        TransformAction->ValueType = EInputActionValueType::Boolean;
        UE_LOG(LogTemp, Warning, TEXT("🌀 创建TransformAction"));
    }
    if (!TestDamageAction) {
        TestDamageAction = NewObject<UInputAction>(this, TEXT("IA_TestDamage"));
        TestDamageAction->ValueType = EInputActionValueType::Boolean;
    }
    if (!TestDeathAction) {
        TestDeathAction = NewObject<UInputAction>(this, TEXT("IA_TestDeath"));
        TestDeathAction->ValueType = EInputActionValueType::Boolean;
    }
    if (!TestRespawnAction) {
        TestRespawnAction = NewObject<UInputAction>(this, TEXT("IA_TestRespawn"));
        TestRespawnAction->ValueType = EInputActionValueType::Boolean;
    }
    if (!TestDetectAction) {
        TestDetectAction = NewObject<UInputAction>(this, TEXT("IA_TestDetect"));
        TestDetectAction->ValueType = EInputActionValueType::Boolean;
    }
    if (!TestCollisionAction) { // 🆕 添加测试碰撞检测动作
        TestCollisionAction = NewObject<UInputAction>(this, TEXT("IA_TestCollision"));
        TestCollisionAction->ValueType = EInputActionValueType::Boolean;
    }
    // 镜头抖动测试
    if (!CameraShakeAction)
    {
        CameraShakeAction = NewObject<UInputAction>(this, TEXT("IA_CameraShake"));
        CameraShakeAction->ValueType = EInputActionValueType::Boolean;
        UE_LOG(LogTemp, Warning, TEXT("🆕 创建 CameraShakeAction"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ CameraShakeAction 已存在"));
    }//新加的
    if (APlayerController* PC = Cast<APlayerController>(GetController())) {
        if (ULocalPlayer* LP = PC->GetLocalPlayer()) {
            if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP)) {
                Subsystem->ClearAllMappings();
                if (!InputMappingContext || InputMappingContext->GetMappings().Num() < 15) {
                    UE_LOG(LogTemp, Warning, TEXT("Rebuilding Runtime IMC (PrevCount=%d)"), InputMappingContext ? InputMappingContext->GetMappings().Num() : -1);
                    InputMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Wukong_Runtime_Ensured"));
                    InputMappingContext->MapKey(MoveAction, EKeys::D);
                    {
                        auto& AMap = InputMappingContext->MapKey(MoveAction, EKeys::A);
                        AMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));
                    }
                    {
                        auto& WMap = InputMappingContext->MapKey(MoveAction, EKeys::W);
                        auto* Swz = NewObject<UInputModifierSwizzleAxis>(this);
                        Swz->Order = EInputAxisSwizzle::YXZ;
                        WMap.Modifiers.Add(Swz);
                    }
                    {
                        auto& SMap = InputMappingContext->MapKey(MoveAction, EKeys::S);
                        auto* Swz = NewObject<UInputModifierSwizzleAxis>(this);
                        Swz->Order = EInputAxisSwizzle::YXZ;
                        SMap.Modifiers.Add(Swz);
                        SMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));
                    }
                    InputMappingContext->MapKey(LookAction, EKeys::MouseX);
                    {
                        auto& MY = InputMappingContext->MapKey(LookAction, EKeys::MouseY);
                        auto* SwzY = NewObject<UInputModifierSwizzleAxis>(this);
                        SwzY->Order = EInputAxisSwizzle::YXZ;
                        MY.Modifiers.Add(SwzY);
                    }
                    InputMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
                    InputMappingContext->MapKey(LightAttackAction, EKeys::LeftMouseButton);
                    InputMappingContext->MapKey(SprintAction, EKeys::LeftShift);
                    InputMappingContext->MapKey(DodgeAction, EKeys::F);
                    InputMappingContext->MapKey(HeavyAttackAction, EKeys::RightMouseButton);
                    InputMappingContext->MapKey(ToggleInvisibilityAction, EKeys::V);
                    InputMappingContext->MapKey(TransformAction, EKeys::R); // 🌀 变身按键

                    InputMappingContext->MapKey(TestDamageAction, EKeys::T);
                    InputMappingContext->MapKey(TestDeathAction, EKeys::Y);
                    InputMappingContext->MapKey(TestRespawnAction, EKeys::U);
                    InputMappingContext->MapKey(TestDetectAction, EKeys::G);
                    InputMappingContext->MapKey(TestCollisionAction, EKeys::H); // 🆕 测试碰撞检测按键

                    // 新增功能按键映射
                    InputMappingContext->MapKey(StunSkillAction, EKeys::Q);
                    InputMappingContext->MapKey(DrinkPotionAction, EKeys::E);

                    // 镜头抖动（J）
                    InputMappingContext->MapKey(CameraShakeAction, EKeys::J);//
                }
                Subsystem->AddMappingContext(InputMappingContext, 100);
                UE_LOG(LogTemp, Warning, TEXT("Runtime IMC applied in Setup. MappingCount=%d"), InputMappingContext->GetMappings().Num());
            }
        }
    }

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("EnhancedInputComponent detected. Binding actions..."));
        if (MoveAction)
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWukongCharacter::Move);
        if (LookAction)
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWukongCharacter::Look);
        if (JumpAction) {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }
        if (LightAttackAction)
            EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &AWukongCharacter::LightAttack);
        if (SprintAction) {
            EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AWukongCharacter::SprintStart);
            EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AWukongCharacter::SprintStop);
        }
        if (DodgeAction)
            EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AWukongCharacter::Dodge);
        if (HeavyAttackAction) {
            EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AWukongCharacter::StartHeavyCharge);
            EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Completed, this, &AWukongCharacter::ReleaseHeavyAttack);
        }
        if (ToggleInvisibilityAction) {
            EnhancedInputComponent->BindAction(ToggleInvisibilityAction, ETriggerEvent::Started, this, &AWukongCharacter::ToggleInvisibility);
        }

        // 🌀 变身按键绑定
        if (TransformAction) {
            EnhancedInputComponent->BindAction(TransformAction, ETriggerEvent::Started, this, &AWukongCharacter::TriggerTransform);
            UE_LOG(LogTemp, Warning, TEXT("🔗 绑定变身按键 (R键)"));
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TransformAction 为空！"));
        }

        // 测试功能绑定
        if (TestDamageAction) {
            EnhancedInputComponent->BindAction(TestDamageAction, ETriggerEvent::Started, this, &AWukongCharacter::TestTakeDamage);
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TestDamageAction 为空！"));
        }
        if (TestDeathAction) {
            EnhancedInputComponent->BindAction(TestDeathAction, ETriggerEvent::Started, this, &AWukongCharacter::TestDie);
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TestDeathAction 为空！"));
        }
        if (TestRespawnAction) {
            EnhancedInputComponent->BindAction(TestRespawnAction, ETriggerEvent::Started, this, &AWukongCharacter::TestRespawn);
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TestRespawnAction 为空！"));
        }
        if (TestDetectAction) {
            EnhancedInputComponent->BindAction(TestDetectAction, ETriggerEvent::Started, this, &AWukongCharacter::TestFrontDetection);
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TestDetectAction 为空！"));
        }
        // 🆕 测试碰撞检测绑定
        if (TestCollisionAction) {
            EnhancedInputComponent->BindAction(TestCollisionAction, ETriggerEvent::Started, this, &AWukongCharacter::TestCollisionDetection);
            UE_LOG(LogTemp, Warning, TEXT("🔗 绑定碰撞检测测试按键 (H键)"));
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TestCollisionAction 为空！"));
        }// 镜头抖动绑定
        if (CameraShakeAction)
        {
            EnhancedInputComponent->BindAction(
                CameraShakeAction,
                ETriggerEvent::Started,
                this,
                &AWukongCharacter::OnPressJ_ShakeCamera
            );

            UE_LOG(LogTemp, Warning, TEXT("🔗 绑定镜头抖动按键 (J键)"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ CameraShakeAction 为空！"));
        }//新加的
        // 新增功能绑定
        if (StunSkillAction) {
            EnhancedInputComponent->BindAction(StunSkillAction, ETriggerEvent::Started, this, &AWukongCharacter::StunSkill);
            UE_LOG(LogTemp, Warning, TEXT("🔗 绑定定身技能按键 (Q键)"));
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ StunSkillAction 为空！"));
        }

        if (DrinkPotionAction) {
            EnhancedInputComponent->BindAction(DrinkPotionAction, ETriggerEvent::Started, this, &AWukongCharacter::DrinkPotion);
            UE_LOG(LogTemp, Warning, TEXT("🔗 绑定喝药按键 (E键)"));
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ DrinkPotionAction 为空！"));
        }

    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("PlayerInputComponent is NOT EnhancedInputComponent! Type=%s"), *PlayerInputComponent->GetClass()->GetName());
    }
}

void AWukongCharacter::ReceiveDamage(float DamageAmount)
{
    CurrentHealth -= DamageAmount;

    if (CurrentHealth < 0.0f) CurrentHealth = 0.0f;

    if (MyPlayerHUD)
    {
        float Percent = CurrentHealth / MaxHealth;
        MyPlayerHUD->UpdateHealth(CurrentHealth, MaxHealth);
    }
}

void AWukongCharacter::UseSkill(float ManaCost)
{
    CurrentStamina -= ManaCost;

    if (CurrentStamina < 0.0f) CurrentStamina = 0.0f;

    if (MyPlayerHUD)
    {
        MyPlayerHUD->UpdateMana(CurrentStamina, MaxStamina);
    }
}

void  AWukongCharacter::useskillslot()
{
    MyPlayerHUD->TriggerSkillCooldown(3.0);
}

// ==========================================
// 🎭 隐身系统实现
// ==========================================

void AWukongCharacter::ToggleInvisibility()
{
    UE_LOG(LogTemp, Warning, TEXT("🎭 ===== TOGGLE INVISIBILITY ====="));
    UseSkill(DodgeStaminaCost);
    if (!bCanToggleInvisibility || CurrentStamina < DodgeStaminaCost) {
        UE_LOG(LogTemp, Warning, TEXT("⏰ 隐身正在冷却中，无法切换"));
        return;
    }

    bIsInvisible = !bIsInvisible;
    SetInvisibility(bIsInvisible);

    bCanToggleInvisibility = false;
    GetWorldTimerManager().SetTimer(
        InvisibilityCooldownTimerHandle,
        this,
        &AWukongCharacter::EndInvisibilityCooldown,
        InvisibilityCooldown,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("✅ 隐身状态已切换: %s"), bIsInvisible ? TEXT("隐身") : TEXT("显形"));
}

void AWukongCharacter::SetInvisibility(bool bInvisible)
{
    UE_LOG(LogTemp, Warning, TEXT("🎭 ===== SET INVISIBILITY ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 参数: bInvisible=%d"), bInvisible);

    USkeletalMeshComponent* MeshComponent = GetMesh();
    UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

    if (!MeshComponent || !CapsuleComp) {
        UE_LOG(LogTemp, Error, TEXT("❌ 无法获取网格或碰撞组件"));
        return;
    }

    if (OriginalMaterials.Num() == 0) {
        SaveOriginalMaterials();
    }

    if (bInvisible) {
        UE_LOG(LogTemp, Warning, TEXT("🌫️ 进入隐身状态"));
        CurrentActionState = EWukongActionState::Invisible;

        MeshComponent->SetVisibility(false);
        CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        for (int32 i = 0; i < MeshComponent->GetNumMaterials(); i++) {
            UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(MeshComponent->GetMaterial(i), this);
            if (DynamicMaterial) {
                DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.2f);
                MeshComponent->SetMaterial(i, DynamicMaterial);
            }
        }

        if (GetCharacterMovement()) {
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("👤 退出隐身状态"));
        CurrentActionState = EWukongActionState::Idle;

        MeshComponent->SetVisibility(true);
        CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        if (OriginalMaterials.Num() > 0) {
            for (int32 i = 0; i < OriginalMaterials.Num() && i < MeshComponent->GetNumMaterials(); ++i) {
                if (OriginalMaterials[i]) {
                    MeshComponent->SetMaterial(i, OriginalMaterials[i]);
                }
            }
        }

        if (GetCharacterMovement()) {
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }
}

void AWukongCharacter::SaveOriginalMaterials()
{
    UE_LOG(LogTemp, Warning, TEXT("💾 保存原始材质"));

    USkeletalMeshComponent* MeshComponent = GetMesh();
    if (MeshComponent) {
        OriginalMaterials.Empty();

        for (int32 i = 0; i < MeshComponent->GetNumMaterials(); i++) {
            OriginalMaterials.Add(MeshComponent->GetMaterial(i));
            UE_LOG(LogTemp, Warning, TEXT("  材质槽 %d: %s"), i,
                OriginalMaterials[i] ? *OriginalMaterials[i]->GetName() : TEXT("空"));
        }

        UE_LOG(LogTemp, Warning, TEXT("✅ 保存了 %d 个原始材质"), OriginalMaterials.Num());
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("❌ 无法获取网格组件"));
    }
}

void AWukongCharacter::EndInvisibilityCooldown()
{
    bCanToggleInvisibility = true;
    UE_LOG(LogTemp, Warning, TEXT("✅ 隐身冷却结束，可以再次切换"));
}

// ==========================================
// 🌀 变身系统实现
// ==========================================

void AWukongCharacter::TriggerTransform()
{
    UE_LOG(LogTemp, Warning, TEXT("🌀 ===== TRIGGER TRANSFORM ====="));

    if (bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 死亡状态无法变身"));
        return;
    }

    // 检查是否正在执行其他动作
    if (CurrentActionState == EWukongActionState::Dodge ||
        CurrentActionState == EWukongActionState::HeavyAttack ||
        CurrentActionState == EWukongActionState::LightAttack ||
        CurrentActionState == EWukongActionState::Sprint)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 正在执行其他动作，无法变身"));
        return;
    }
    UseSkill(TransformStaminaCost);

    // ③ 再次检查体力是否足够
    if (CurrentStamina < TransformStaminaCost)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 变身失败：体力不足"));
        return;
    }
    if (bIsTransformed)
    {
        // 如果已经变身，则结束变身
        TransformEnd();
    }
    else
    {
        // 开始变身
        TransformStart();
    }
}

void AWukongCharacter::TransformStart()
{
    UE_LOG(LogTemp, Warning, TEXT("🌀 ===== TRANSFORM START ====="));

    if (bIsTransformed)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 已经是变身状态"));
        return;
    }

    // 设置状态
    CurrentActionState = EWukongActionState::TransformStart;
    bIsTransformed = true;

    // 保存原始大小
    OriginalScale = GetActorScale3D();

    // 应用变身大小
    FVector NewScale = OriginalScale * TransformScaleMultiplier;
    SetActorScale3D(NewScale);

    // 生成粒子效果
    SpawnTransformParticle();

    // 更新状态为变身完成
    CurrentActionState = EWukongActionState::Transformed;

    // 设置变身结束定时器
    GetWorldTimerManager().SetTimer(
        TransformTimerHandle,
        this,
        &AWukongCharacter::TransformEnd,
        TransformDuration,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("✅ 变身开始: 原始大小(%.2f,%.2f,%.2f) -> 新大小(%.2f,%.2f,%.2f)"),
        OriginalScale.X, OriginalScale.Y, OriginalScale.Z,
        NewScale.X, NewScale.Y, NewScale.Z);
}

void AWukongCharacter::TransformEnd()
{
    UE_LOG(LogTemp, Warning, TEXT("🌀 ===== TRANSFORM END ====="));

    if (!bIsTransformed)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 不是变身状态"));
        return;
    }

    // 恢复原始大小
    SetActorScale3D(OriginalScale);

    // 重置状态
    CurrentActionState = EWukongActionState::TransformEnd;
    bIsTransformed = false;

    // 清理计时器
    GetWorldTimerManager().ClearTimer(TransformTimerHandle);

    // 稍后回到空闲状态
    GetWorldTimerManager().SetTimer(
        TransformTimerHandle,
        [this]() {
            CurrentActionState = EWukongActionState::Idle;
            UE_LOG(LogTemp, Warning, TEXT("✅ 变身完全结束，回到空闲状态"));
        },
        0.5f,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("✅ 变身结束: 大小恢复为(%.2f,%.2f,%.2f)"),
        OriginalScale.X, OriginalScale.Y, OriginalScale.Z);
}

void AWukongCharacter::SpawnTransformParticle()
{
    UE_LOG(LogTemp, Warning, TEXT("🌀 ===== SPAWN TRANSFORM PARTICLE ====="));

    if (!TransformParticleEffect)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 未设置变身粒子效果，请检查TransformParticleEffect变量"));
        return;
    }

    // 获取角色的位置
    FVector SpawnLocation = GetActorLocation();

    // 创建粒子系统组件
    UParticleSystemComponent* ParticleComponent = UGameplayStatics::SpawnEmitterAtLocation(
        GetWorld(),
        TransformParticleEffect,
        SpawnLocation,
        FRotator::ZeroRotator,
        FVector(TransformScaleMultiplier) // 根据变身大小调整粒子大小
    );

    if (ParticleComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ 变身粒子效果已生成: %s"), *TransformParticleEffect->GetName());
        UE_LOG(LogTemp, Warning, TEXT("📍 生成位置: (%.1f,%.1f,%.1f)"),
            SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ 无法生成变身粒子效果"));
    }
}

// ==========================================
// 重攻击函数（兼容性）
// ==========================================

void AWukongCharacter::HeavyAttack()
{
    // 1. 检查冷却状态
    if (bIsHeavyAttackOnCooldown)
    {
        // 可以显示剩余冷却时间
        if (GetWorld())
        {
            float RemainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(HeavyAttackCooldownTimerHandle);
            UE_LOG(LogTemp, Warning, TEXT("⚠️ 重攻击冷却中，剩余: %.1f秒"), RemainingTime);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ 重攻击冷却中"));
        }
        return;
    }

    // 2. 检查体力
    if (CurrentStamina < HeavyAttackStaminaCost)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 重攻击被阻止: 体力不足"));
        return;
    }

    // 3. 执行攻击
    ExecuteHeavyAttack(1.0f);

    // 4. 启动冷却（必须在攻击执行后调用！）
    StartHeavyAttackCooldown();

    // 5. 消耗体力
    CurrentStamina -= HeavyAttackStaminaCost;

    UE_LOG(LogTemp, Log, TEXT("重攻击执行成功，开始冷却"));
}

void AWukongCharacter::StartHeavyAttackCooldown()
{
    if (!bIsHeavyAttackOnCooldown)
    {
        bIsHeavyAttackOnCooldown = true;

        if (GetWorld())
        {
            // 设置计时器，3秒后结束冷却
            GetWorld()->GetTimerManager().SetTimer(
                HeavyAttackCooldownTimerHandle,
                this,
                &AWukongCharacter::OnHeavyAttackCooldownEnd,
                HeavyAttackCooldownTime,
                false // 不循环，只执行一次
            );

            UE_LOG(LogTemp, Log, TEXT("重攻击开始冷却，冷却时间: %f秒"), HeavyAttackCooldownTime);
        }
    }
}

void AWukongCharacter::OnHeavyAttackCooldownEnd()
{
    bIsHeavyAttackOnCooldown = false;
    UE_LOG(LogTemp, Log, TEXT("重攻击冷却结束"));

    // 可选：播放音效或视觉效果提示冷却结束
    // PlayCoolDownEndEffect();
}
void AWukongCharacter::PlayHeavyAttackMontage()
{
    if (HeavyAttackMontage)
    {
        PlayMontageSafe(HeavyAttackMontage, 1.0f, FName(TEXT("Default")));
    }
}
void AWukongCharacter::StunSkill()
{
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== STUN SKILL TRIGGERED ====="));

    // 检查当前状态是否可以使用技能
    if (CurrentActionState == EWukongActionState::Stun ||
        CurrentActionState == EWukongActionState::DrinkingPotion ||
        CurrentActionState == EWukongActionState::Dodge ||
        CurrentActionState == EWukongActionState::HeavyAttack) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 定身技能被阻止: 当前状态不适合"));
        return;
    }

    // 🔋 体力检查
    if (CurrentStamina < StunSkillStaminaCost) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 定身技能被阻止: 体力不足"));
        return;
    }

    // 消耗体力
    CurrentStamina -= StunSkillStaminaCost;
    UE_LOG(LogTemp, Warning, TEXT("🔋 消耗体力: %.1f，剩余: %.1f"), StunSkillStaminaCost, CurrentStamina);

    // 设置状态为定身技能使用中
    CurrentActionState = EWukongActionState::Stun;

    // 🆕 使用新的碰撞检测系统
    TArray<AActor*> Enemies = DetectEnemiesInRange(StunSkillRange, EAttackDetectionType::Line);

    if (Enemies.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯 定身技能命中 %d 个目标"), Enemies.Num());

        for (AActor* Enemy : Enemies)
        {
            if (IsValid(Enemy))
            {
                AParagonFengMao* FengMaoEnemy = Cast<AParagonFengMao>(Enemy);
                if (FengMaoEnemy && !FengMaoEnemy->bIsDead)
                {
                    UE_LOG(LogTemp, Warning, TEXT("💫 对敌人 %s 施加定身效果"), *FengMaoEnemy->GetName());
                    ApplyStunToTarget(FengMaoEnemy);

                    // 同时造成伤害
                    FDamageEvent DamageEvent;
                    FHitResult HitResult;
                    FPointDamageEvent PointDamageEvent(StunSkillDamage, HitResult, GetActorForwardVector(), nullptr);
                    FengMaoEnemy->TakeDamage(StunSkillDamage, PointDamageEvent, nullptr, this);

                    // 显示伤害数字
                    ShowDamageNumbers(FengMaoEnemy->GetActorLocation() + FVector(0, 0, 100), StunSkillDamage, false);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 前方没有检测到敌人"));
    }

    // 设置技能结束定时器
    GetWorldTimerManager().SetTimer(
        DodgeTimerHandle, // 复用计时器
        [this]() {
            if (CurrentActionState == EWukongActionState::Stun) {
                CurrentActionState = EWukongActionState::Idle;
                UE_LOG(LogTemp, Warning, TEXT("🏁 定身技能结束，回到空闲状态"));
            }
        },
        1.0f, // 1秒后结束
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("✅ ===== STUN SKILL EXECUTED =====\n"));
}

// 对目标施加定身效果
void AWukongCharacter::ApplyStunToTarget(AActor* Target)
{
    AParagonFengMao* Enemy = Cast<AParagonFengMao>(Target);
    if (!Enemy || Enemy->bIsDead) {
        return;
    }

    // 设置敌人状态为定身
    Enemy->SetAIState(EFengMaoAIState::Idle); // 设置为空闲状态，停止AI行为

    // 禁用敌人的移动
    if (Enemy->GetCharacterMovement()) {
        Enemy->GetCharacterMovement()->DisableMovement();
    }

    UE_LOG(LogTemp, Warning, TEXT("💫 敌人 %s 被定身"), *Enemy->GetName());

    // 设置定身结束定时器
    FTimerHandle StunTimerHandle;
    GetWorldTimerManager().SetTimer(
        StunTimerHandle,
        [Enemy]() {
            if (Enemy && !Enemy->bIsDead) {
                // 恢复敌人的移动
                if (Enemy->GetCharacterMovement()) {
                    Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                }

                // 恢复AI状态
                if (Enemy->GetAIState() == EFengMaoAIState::Idle) {
                    Enemy->SetAIState(EFengMaoAIState::Patrol); // 恢复巡逻状态
                }

                UE_LOG(LogTemp, Warning, TEXT("💫 敌人 %s 定身结束"), *Enemy->GetName());
            }
        },
        StunDuration,
        false
    );
}

// 加快敌人苏醒
void AWukongCharacter::WakeUpEnemy(class AParagonFengMao* Enemy)
{
    if (!Enemy || Enemy->bIsDead) {
        return;
    }

    // 立即恢复敌人的移动
    if (Enemy->GetCharacterMovement()) {
        Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }

    // 恢复AI状态
    if (Enemy->GetAIState() == EFengMaoAIState::Idle) {
        Enemy->SetAIState(EFengMaoAIState::Patrol); // 恢复巡逻状态
    }

    UE_LOG(LogTemp, Warning, TEXT("💫 敌人 %s 被提前唤醒"), *Enemy->GetName());
}

// 执行喝药
void AWukongCharacter::DrinkPotion()
{
    UE_LOG(LogTemp, Warning, TEXT("🧪 ===== DRINK POTION TRIGGERED ====="));

    // 检查是否还有药水
    if (PotionCount <= 0) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 没有药水了"));
        return;
    }

    // 检查当前状态是否可以喝药
    if (CurrentActionState == EWukongActionState::DrinkingPotion ||
        CurrentActionState == EWukongActionState::Stun ||
        CurrentActionState == EWukongActionState::Dodge ||
        CurrentActionState == EWukongActionState::HeavyAttack) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 喝药被阻止: 当前状态不适合"));
        return;
    }

    // 消耗一瓶药水
    PotionCount--;
    UE_LOG(LogTemp, Warning, TEXT("🧪 消耗一瓶药水，剩余: %d瓶"), PotionCount);

    // 设置状态为喝药中
    CurrentActionState = EWukongActionState::DrinkingPotion;

    // 播放喝药动画
    if (DrinkPotionMontage) {
        PlayMontageSafe(DrinkPotionMontage, 1.0f, FName(TEXT("Default")));
        UE_LOG(LogTemp, Warning, TEXT("🎬 播放喝药动画"));
    }

    // 立即回复一部分生命值
    float HealAmount = FMath::Min(InstantHealAmount, MaxHealth - CurrentHealth);
    CurrentHealth += HealAmount;
    CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
    UE_LOG(LogTemp, Warning, TEXT("💚 瞬间回复 %.1f 生命值，当前生命: %.1f/%.1f"), HealAmount, CurrentHealth, MaxHealth);

    // 🆕 显示治疗数字
    ShowDamageNumbers(GetActorLocation() + FVector(0, 0, 100), HealAmount, true);

    // 设置喝药结束定时器
    GetWorldTimerManager().SetTimer(
        DodgeTimerHandle, // 复用计时器
        [this]() {
            StartDrinkingPotion();
        },
        0.5f, // 0.5秒后开始持续回复
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("✅ ===== DRINK POTION EXECUTED =====\n"));
}

// 开始喝药（持续回复阶段）
void AWukongCharacter::StartDrinkingPotion()
{
    UE_LOG(LogTemp, Warning, TEXT("🧪 开始持续回复生命值"));

    // 计算持续回复次数
    int32 HealTicks = FMath::CeilToInt(OverTimeHealDuration / OverTimeHealInterval);
    float HealPerTick = OverTimeHealAmount / HealTicks;

    // 设置持续回复定时器
    GetWorldTimerManager().SetTimer(
        PotionHealTimerHandle,
        [this, HealPerTick]() {
            if (CurrentHealth < MaxHealth && !bIsDead) {
                float HealAmount = FMath::Min(HealPerTick, MaxHealth - CurrentHealth);
                CurrentHealth += HealAmount;
                CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
                UE_LOG(LogTemp, Warning, TEXT("💚 持续回复 %.1f 生命值，当前生命: %.1f/%.1f"), HealAmount, CurrentHealth, MaxHealth);

                // 🆕 显示小的治疗数字
                ShowDamageNumbers(GetActorLocation() + FVector(FMath::RandRange(-20, 20), FMath::RandRange(-20, 20), 100), HealAmount, true);
            }
        },
        OverTimeHealInterval,
        true, // 循环定时器
        0.0f // 立即开始
    );

    // 设置喝药结束定时器
    GetWorldTimerManager().SetTimer(
        DodgeTimerHandle, // 复用计时器
        [this]() {
            FinishDrinkingPotion();
        },
        OverTimeHealDuration,
        false
    );
}

// 完成喝药
void AWukongCharacter::FinishDrinkingPotion()
{
    // 清除持续回复定时器
    GetWorldTimerManager().ClearTimer(PotionHealTimerHandle);

    // 恢复空闲状态
    if (CurrentActionState == EWukongActionState::DrinkingPotion) {
        CurrentActionState = EWukongActionState::Idle;
        UE_LOG(LogTemp, Warning, TEXT("🧪 喝药结束，回到空闲状态"));
    }

    UE_LOG(LogTemp, Warning, TEXT("🧪 持续回复结束"));
}

void AWukongCharacter::OnTogglePauseMenu()
{
    // 1. 获取控制当前角色的 PlayerController
    // 因为 SetInputMode 和 ShowMouseCursor 都在 Controller 里
    APlayerController* PC = Cast<APlayerController>(GetController());

    if (!PC) return; // 如果是被 AI 控制或者没有 Controller，直接返回

    // --- 逻辑 A: 如果游戏已经暂停，则视为"继续游戏" ---
    if (UGameplayStatics::IsGamePaused(this))
    {
        // 移除 UI
        if (PauseMenuInstance)
        {
            PauseMenuInstance->RemoveFromParent();
        }

        // 恢复游戏状态
        UGameplayStatics::SetGamePaused(this, false);
        PC->SetInputMode(FInputModeGameOnly());
        PC->SetShowMouseCursor(false);
        return;
    }

    // --- 逻辑 B: 如果游戏未暂停，则显示菜单 ---

    // 1. 懒加载：如果没有创建过 Widget，就创建一次
    if (!PauseMenuInstance)
    {
        // CreateWidget 第一个参数通常传 PlayerController，但在 Character 里传 this 也可以，UE 会自动找 World
        PauseMenuInstance = CreateWidget<UPauseMenuWidget>(PC, UPauseMenuWidget::StaticClass());
    }

    // 2. 显示并暂停
    if (PauseMenuInstance)
    {
        PauseMenuInstance->AddToViewport(100); // 层级设高一点

        UGameplayStatics::SetGamePaused(this, true);

        // 设置输入模式：仅允许 UI 操作，并且不锁定鼠标
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(PauseMenuInstance->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }
}
// ==========================================
// 🆕 碰撞检测系统实现
// ==========================================

// 🎯 检测范围内的敌人（通用接口）
TArray<AActor*> AWukongCharacter::DetectEnemiesInRange(float Radius, EAttackDetectionType DetectionType)
{
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== 开始碰撞检测 ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 参数: 半径=%.1f, 类型=%d"), Radius, (int32)DetectionType);

    TArray<AActor*> DetectedEnemies;

    switch (DetectionType)
    {
    case EAttackDetectionType::Circle:
        DetectedEnemies = CircleDetection(Radius);
        break;
    case EAttackDetectionType::Sector:
        DetectedEnemies = SectorDetection(Radius, DefaultAttackAngle);
        break;
    case EAttackDetectionType::Line:
        DetectedEnemies = LineDetection(Radius, Radius * 0.2f); // 宽度为半径的20%
        break;
    case EAttackDetectionType::Sphere:
        DetectedEnemies = SphereDetection(Radius);
        break;
    default:
        DetectedEnemies = CircleDetection(Radius);
        break;
    }

    // 保存最后检测到的敌人列表
    LastDetectedEnemies = DetectedEnemies;

    // 绘制调试可视化
    if (bShowDebugVisuals && GetWorld())
    {
        FVector DetectionCenter = GetActorLocation() + (GetActorForwardVector() * (Radius * 0.5f));
        DetectionCenter.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f;

        FColor DebugColor = FColor::Green;
        if (DetectedEnemies.Num() == 0)
            DebugColor = FColor::Red;
        else if (DetectedEnemies.Num() >= 3)
            DebugColor = FColor::Yellow;

        DrawDebugDetectionShape(DetectionType, DetectionCenter, FVector(Radius, 0, 0), GetActorRotation(), DebugColor, 2.0f);
    }

    UE_LOG(LogTemp, Warning, TEXT("🎯 检测完成: 发现 %d 个敌人"), DetectedEnemies.Num());
    return DetectedEnemies;
}

// 🎯 圆形检测
TArray<AActor*> AWukongCharacter::CircleDetection(float Radius)
{
    TArray<AActor*> EnemiesInRange;

    // 使用球形重叠检测
    TArray<FHitResult> HitResults;
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);

    FVector DetectionCenter = GetActorLocation() + (GetActorForwardVector() * (Radius * 0.5f));
    DetectionCenter.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.bTraceComplex = false;

    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults,
        DetectionCenter,
        DetectionCenter,
        FQuat::Identity,
        ECC_Pawn,
        SphereShape,
        QueryParams
    );

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            AActor* HitActor = Hit.GetActor();
            if (HitActor && HitActor != this)
            {
                // 检查是否是敌人（不仅仅是封魔敌人，兼容未来其他敌人）
                if (HitActor->IsA(AParagonFengMao::StaticClass()) ||
                    HitActor->ActorHasTag(FName("Enemy")))
                {
                    // 检查敌人是否存活
                    AParagonFengMao* Enemy = Cast<AParagonFengMao>(HitActor);
                    if (!Enemy || !Enemy->bIsDead)
                    {
                        EnemiesInRange.Add(HitActor);

                        // 计算距离
                        float Distance = FVector::Distance(GetActorLocation(), HitActor->GetActorLocation());
                        UE_LOG(LogTemp, Warning, TEXT("   🔵 圆形检测命中: %s (距离: %.1f)"),
                            *HitActor->GetName(), Distance);
                    }
                }
            }
        }
    }

    return EnemiesInRange;
}

// 🎯 扇形检测
TArray<AActor*> AWukongCharacter::SectorDetection(float Radius, float AngleDegrees)
{
    TArray<AActor*> EnemiesInSector;

    // 先进行圆形检测
    TArray<AActor*> AllEnemiesInRange = CircleDetection(Radius);

    if (AllEnemiesInRange.Num() == 0)
    {
        return EnemiesInSector;
    }

    // 筛选在扇形范围内的敌人
    FVector PlayerForward = GetActorForwardVector();
    FVector PlayerLocation = GetActorLocation();
    float HalfAngle = AngleDegrees * 0.5f;
    float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HalfAngle));

    for (AActor* Enemy : AllEnemiesInRange)
    {
        if (!Enemy) continue;

        FVector ToEnemy = (Enemy->GetActorLocation() - PlayerLocation).GetSafeNormal();
        float DotProduct = FVector::DotProduct(PlayerForward, ToEnemy);

        // 检查是否在扇形范围内
        if (DotProduct >= CosHalfAngle)
        {
            EnemiesInSector.Add(Enemy);

            // 计算实际角度
            float Angle = FMath::Acos(DotProduct) * (180.0f / PI);
            UE_LOG(LogTemp, Warning, TEXT("   🔶 扇形检测命中: %s (角度: %.1f度)"),
                *Enemy->GetName(), Angle);
        }
    }

    return EnemiesInSector;
}

// 🎯 直线检测
TArray<AActor*> AWukongCharacter::LineDetection(float Length, float Width)
{
    TArray<AActor*> EnemiesInLine;

    // 使用胶囊体检测
    FVector StartLocation = GetActorLocation() + FVector(0, 0, 50);
    FVector EndLocation = StartLocation + (GetActorForwardVector() * Length);

    TArray<FHitResult> HitResults;
    FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Width, Length * 0.5f);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults,
        StartLocation,
        EndLocation,
        FQuat::Identity,
        ECC_Pawn,
        CapsuleShape,
        QueryParams
    );

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            AActor* HitActor = Hit.GetActor();
            if (HitActor && HitActor != this)
            {
                // 检查是否是敌人
                if (HitActor->IsA(AParagonFengMao::StaticClass()) ||
                    HitActor->ActorHasTag(FName("Enemy")))
                {
                    // 检查敌人是否存活
                    AParagonFengMao* Enemy = Cast<AParagonFengMao>(HitActor);
                    if (!Enemy || !Enemy->bIsDead)
                    {
                        EnemiesInLine.Add(HitActor);

                        float Distance = FVector::Distance(StartLocation, Hit.Location);
                        UE_LOG(LogTemp, Warning, TEXT("   ➡️ 直线检测命中: %s (距离: %.1f)"),
                            *HitActor->GetName(), Distance);
                    }
                }
            }
        }
    }

    return EnemiesInLine;
}

// 🎯 球形检测
TArray<AActor*> AWukongCharacter::SphereDetection(float Radius)
{
    // 球形检测与圆形检测类似，但检测中心不同
    TArray<AActor*> EnemiesInSphere;

    // 使用球形重叠检测，检测中心在角色位置
    TArray<FHitResult> HitResults;
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);

    FVector DetectionCenter = GetActorLocation();
    DetectionCenter.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.bTraceComplex = false;

    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults,
        DetectionCenter,
        DetectionCenter,
        FQuat::Identity,
        ECC_Pawn,
        SphereShape,
        QueryParams
    );

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            AActor* HitActor = Hit.GetActor();
            if (HitActor && HitActor != this)
            {
                // 检查是否是敌人
                if (HitActor->IsA(AParagonFengMao::StaticClass()) ||
                    HitActor->ActorHasTag(FName("Enemy")))
                {
                    // 检查敌人是否存活
                    AParagonFengMao* Enemy = Cast<AParagonFengMao>(HitActor);
                    if (!Enemy || !Enemy->bIsDead)
                    {
                        EnemiesInSphere.Add(HitActor);

                        float Distance = FVector::Distance(DetectionCenter, HitActor->GetActorLocation());
                        UE_LOG(LogTemp, Warning, TEXT("   🔘 球形检测命中: %s (距离: %.1f)"),
                            *HitActor->GetName(), Distance);
                    }
                }
            }
        }
    }

    return EnemiesInSphere;
}

// 🎯 对检测到的敌人应用伤害
void AWukongCharacter::ApplyDamageToDetectedEnemies(TArray<AActor*> Enemies, float BaseDamage, bool bIsHeavyAttack)
{
    if (Enemies.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚔️ 没有敌人可造成伤害"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("⚔️ 对 %d 个敌人造成伤害 (基础伤害: %.1f, 重攻击: %s)"),
        Enemies.Num(), BaseDamage, bIsHeavyAttack ? TEXT("是") : TEXT("否"));

    for (AActor* Enemy : Enemies)
    {
        if (IsValid(Enemy))
        {
            // 检查是否是封魔敌人
            AParagonFengMao* FengMaoEnemy = Cast<AParagonFengMao>(Enemy);
            if (FengMaoEnemy && !FengMaoEnemy->bIsDead)
            {
                // 计算最终伤害
                float FinalDamage = BaseDamage;
                bool bIsCritical = (FMath::FRand() < CriticalHitChance);

                if (bIsCritical)
                {
                    FinalDamage *= CriticalHitMultiplier;
                    UE_LOG(LogTemp, Warning, TEXT("   💥 暴击！伤害倍率: %.1f"), CriticalHitMultiplier);
                }

                // 创建伤害事件
                FDamageEvent DamageEvent;
                FHitResult HitResult;
                FPointDamageEvent PointDamageEvent(FinalDamage, HitResult, GetActorForwardVector(), nullptr);

                // 应用伤害
                float ActualDamage = FengMaoEnemy->TakeDamage(FinalDamage, PointDamageEvent, GetController(), this);

                UE_LOG(LogTemp, Warning, TEXT("   ⚔️ 对 %s 造成 %.1f 伤害 (实际: %.1f), 剩余生命: %.1f"),
                    *FengMaoEnemy->GetName(), FinalDamage, ActualDamage, FengMaoEnemy->CurrentHealth);

                // 显示伤害数字
                FVector DamageLocation = FengMaoEnemy->GetActorLocation() + FVector(0, 0, 100);
                ShowDamageNumbers(DamageLocation, FinalDamage, bIsCritical);

                // 如果是重攻击且伤害足够大，触发硬直效果
                if (bIsHeavyAttack && FinalDamage >= 30.0f)
                {
                    UE_LOG(LogTemp, Warning, TEXT("   💥 重攻击触发硬直效果"));
                    // 这里可以添加硬直逻辑
                }
            }
        }
    }
}

// 🎯 显示伤害数字（在游戏画面中显示）
void AWukongCharacter::ShowDamageNumbers(FVector Location, float Damage, bool bIsCritical)
{
    if (!GetWorld()) return;

    // 保存伤害数字信息
    DamageNumberLocations.Add(Location);
    DamageNumberValues.Add(Damage);
    DamageNumberIsCritical.Add(bIsCritical);

    // 使用DrawDebugString在游戏画面中显示伤害数字
    FString DamageText = FString::Printf(TEXT("%.0f"), Damage);
    FColor TextColor = bIsCritical ? FColor::Orange : FColor::White;
    float TextScale = bIsCritical ? 2.0f : 1.5f;

    // 在伤害位置上方显示数字
    FVector TextLocation = Location + FVector(0, 0, 50);

    // 使用DrawDebugString显示伤害数字
    DrawDebugString(
        GetWorld(),
        TextLocation,
        DamageText,
        nullptr,
        TextColor,
        1.5f, // 显示时间（秒）
        false, // 不绘制阴影
        TextScale // 文字缩放
    );

    // 如果暴击，显示额外的效果
    if (bIsCritical)
    {
        // 绘制暴击星号
        DrawDebugString(
            GetWorld(),
            TextLocation + FVector(0, 0, 30),
            TEXT("💥"),
            nullptr,
            FColor::Red,
            1.5f,
            false,
            1.2f
        );

        // 绘制暴击效果圈
        DrawDebugCircle(
            GetWorld(),
            Location,
            30.0f,
            8,
            FColor::Orange,
            false,
            1.5f,
            0,
            2.0f
        );
    }

    UE_LOG(LogTemp, Warning, TEXT("🔢 显示伤害数字: %.1f %s 在位置: %s"),
        Damage, bIsCritical ? TEXT("(暴击)") : TEXT(""), *Location.ToString());
}

// 🎯 绘制调试检测形状
void AWukongCharacter::DrawDebugDetectionShape(EAttackDetectionType DetectionType, FVector Center, FVector Extents, FRotator Rotation, FColor Color, float Duration)
{
    if (!GetWorld()) return;

    switch (DetectionType)
    {
    case EAttackDetectionType::Circle:
    {
        // 绘制圆形
        DrawDebugCircle(
            GetWorld(),
            Center,
            Extents.X,
            32,
            Color,
            false,
            Duration,
            0,
            2.0f,
            FVector(1, 0, 0),
            FVector(0, 1, 0),
            false
        );
        break;
    }

    case EAttackDetectionType::Sector:
    {
        // 绘制扇形
        float AngleDegrees = DefaultAttackAngle;
        float HalfAngle = FMath::DegreesToRadians(AngleDegrees * 0.5f);
        int32 Segments = 16;

        // 绘制扇形边缘
        for (int32 i = 0; i <= Segments; i++)
        {
            float Angle = -HalfAngle + (2 * HalfAngle * i / Segments);
            FVector Direction = Rotation.Vector().RotateAngleAxis(FMath::RadiansToDegrees(Angle), FVector(0, 0, 1));
            FVector EndPoint = Center + Direction * Extents.X;

            DrawDebugLine(
                GetWorld(),
                Center,
                EndPoint,
                Color,
                false,
                Duration,
                0,
                2.0f
            );
        }

        // 绘制扇形弧线
        for (int32 i = 0; i < Segments; i++)
        {
            float Angle1 = -HalfAngle + (2 * HalfAngle * i / Segments);
            float Angle2 = -HalfAngle + (2 * HalfAngle * (i + 1) / Segments);

            FVector Dir1 = Rotation.Vector().RotateAngleAxis(FMath::RadiansToDegrees(Angle1), FVector(0, 0, 1));
            FVector Dir2 = Rotation.Vector().RotateAngleAxis(FMath::RadiansToDegrees(Angle2), FVector(0, 0, 1));

            FVector Point1 = Center + Dir1 * Extents.X;
            FVector Point2 = Center + Dir2 * Extents.X;

            DrawDebugLine(
                GetWorld(),
                Point1,
                Point2,
                Color,
                false,
                Duration,
                0,
                2.0f
            );
        }
        break;
    }

    case EAttackDetectionType::Line:
    {
        // 绘制直线（胶囊体）
        FVector Start = Center - (Rotation.Vector() * Extents.Y);
        FVector End = Center + (Rotation.Vector() * Extents.Y);

        DrawDebugCapsule(
            GetWorld(),
            Center,
            Extents.Y,
            Extents.X,
            Rotation.Quaternion(),
            Color,
            false,
            Duration,
            0,
            2.0f
        );
        break;
    }

    case EAttackDetectionType::Sphere:
    {
        // 绘制球形
        DrawDebugSphere(
            GetWorld(),
            Center,
            Extents.X,
            16,
            Color,
            false,
            Duration,
            0,
            2.0f
        );
        break;
    }
    }
}

// 🎯 显示攻击范围调试信息
void AWukongCharacter::ShowAttackRangeDebug(float Radius, EAttackDetectionType DetectionType, FColor Color, float Duration)
{
    UE_LOG(LogTemp, Warning, TEXT("👁️ 显示攻击范围调试: 半径=%.1f, 类型=%d"), Radius, (int32)DetectionType);

    FVector DetectionCenter = GetActorLocation() + (GetActorForwardVector() * (Radius * 0.5f));
    DetectionCenter.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f;

    DrawDebugDetectionShape(DetectionType, DetectionCenter, FVector(Radius, 0, 0), GetActorRotation(), Color, Duration);
}

// 🎯 测试碰撞检测系统
void AWukongCharacter::TestCollisionDetection()
{
    UE_LOG(LogTemp, Warning, TEXT("🧪 ===== 测试碰撞检测系统 ====="));

    // 测试所有检测类型
    float TestRadius = 200.0f;

    // 1. 测试圆形检测
    UE_LOG(LogTemp, Warning, TEXT("1. 测试圆形检测"));
    TArray<AActor*> CircleEnemies = DetectEnemiesInRange(TestRadius, EAttackDetectionType::Circle);
    ShowAttackRangeDebug(TestRadius, EAttackDetectionType::Circle, FColor::Blue, 3.0f);

    // 2. 测试扇形检测
    UE_LOG(LogTemp, Warning, TEXT("2. 测试扇形检测"));
    TArray<AActor*> SectorEnemies = DetectEnemiesInRange(TestRadius, EAttackDetectionType::Sector);
    ShowAttackRangeDebug(TestRadius, EAttackDetectionType::Sector, FColor::Green, 3.0f);

    // 3. 测试直线检测
    UE_LOG(LogTemp, Warning, TEXT("3. 测试直线检测"));
    TArray<AActor*> LineEnemies = DetectEnemiesInRange(TestRadius, EAttackDetectionType::Line);
    ShowAttackRangeDebug(TestRadius, EAttackDetectionType::Line, FColor::Yellow, 3.0f);

    // 4. 测试球形检测
    UE_LOG(LogTemp, Warning, TEXT("4. 测试球形检测"));
    TArray<AActor*> SphereEnemies = DetectEnemiesInRange(TestRadius, EAttackDetectionType::Sphere);
    ShowAttackRangeDebug(TestRadius, EAttackDetectionType::Sphere, FColor::Purple, 3.0f);

    // 汇总结果
    UE_LOG(LogTemp, Warning, TEXT("📊 检测结果汇总:"));
    UE_LOG(LogTemp, Warning, TEXT("   圆形检测: %d 个敌人"), CircleEnemies.Num());
    UE_LOG(LogTemp, Warning, TEXT("   扇形检测: %d 个敌人"), SectorEnemies.Num());
    UE_LOG(LogTemp, Warning, TEXT("   直线检测: %d 个敌人"), LineEnemies.Num());
    UE_LOG(LogTemp, Warning, TEXT("   球形检测: %d 个敌人"), SphereEnemies.Num());

    UE_LOG(LogTemp, Warning, TEXT("✅ 碰撞检测测试完成"));
}

// 🎯 测试整个检测系统
void AWukongCharacter::TestDetectionSystem()
{
    UE_LOG(LogTemp, Warning, TEXT("🔧 ===== 测试完整检测系统 ====="));

    // 测试检测敌人
    TestCollisionDetection();

    // 测试伤害应用
    TArray<AActor*> TestEnemies = DetectEnemiesInRange(DefaultAttackRadius, EAttackDetectionType::Circle);
    if (TestEnemies.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚔️ 测试伤害应用"));
        ApplyDamageToDetectedEnemies(TestEnemies, 10.0f, false);
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ 完整检测系统测试完成"));
}
//抖动函数实现
void AWukongCharacter::OnPressJ_ShakeCamera()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->PlayerCameraManager)
        return;

    UMyCameraModifier* Modifier =
        Cast<UMyCameraModifier>(
            PC->PlayerCameraManager->AddNewCameraModifier(
                UMyCameraModifier::StaticClass()
            )
        );

    if (Modifier)
    {
        // 持续 0.25 秒，强度 2.0
        Modifier->StartShake(0.25f, 2.0f);
    }
}

void AWukongCharacter::OnToggleInventory()
{
    if (MyPlayerHUD) MyPlayerHUD->ToggleInventory();
}