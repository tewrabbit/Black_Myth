#include "WukongCharacter.h" // 引入自己类的声明
#include "EnhancedInputComponent.h" // 引入增强输入系统的核心组件
#include "EnhancedInputSubsystems.h" // 引入增强输入系统的核心组件
#include "GameFramework/CharacterMovementComponent.h" // 引入角色移动组件
#include "UObject/ConstructorHelpers.h" // 用于C++加载资源的组件
#include "Animation/AnimInstance.h" // 用于播放动画的组件
#include "TimerManager.h" // 用于计时器
#include "InputAction.h" // 输入系统核心
#include "InputMappingContext.h" // 输入系统核心
#include "InputModifiers.h" // 输入系统核心（包含 Swizzle/Negate 等）
#include "Kismet/KismetMathLibrary.h" // 用于数学运算
#include "GameFramework/Controller.h" // 用于获取控制器
#include "InputCoreTypes.h" // 保留以确保 EKeys 等可见
#include "Engine/LocalPlayer.h" // 构造函数
#include "Animation/AnimInstance.h" // 动画实例
#include "Components/CapsuleComponent.h" // 胶囊体组件
#include "Engine/World.h" // 用于射线检测
#include "Kismet/GameplayStatics.h" // 用于GameplayStatics工具函数
#include "ParagonFengMao.h" // 敌人角色类
#include "GameFramework/CharacterMovementComponent.h" // 角色移动组件
#include "Components/SkeletalMeshComponent.h" // 骨骼网格体组件（用于物理模拟）
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
    GetCharacterMovement()->bUseControllerDesiredRotation = true; // 第三人称更常见的配置：由移动方向驱动角色朝向
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 🎯 创建输入动作对象 - 定义玩家可以做的所有操作
    // 这些对象会在SetupPlayerInputComponent中绑定到实际的函数
    MoveAction = CreateDefaultSubobject<UInputAction>(TEXT("MoveAction"));
    MoveAction->ValueType = EInputActionValueType::Axis2D;        // XY轴输入（WASD）

    LookAction = CreateDefaultSubobject<UInputAction>(TEXT("LookAction"));
    LookAction->ValueType = EInputActionValueType::Axis2D;        // XY轴输入（鼠标移动）

    JumpAction = CreateDefaultSubobject<UInputAction>(TEXT("JumpAction"));
    JumpAction->ValueType = EInputActionValueType::Boolean;       // 布尔值（按下/释放）

    LightAttackAction = CreateDefaultSubobject<UInputAction>(TEXT("LightAttackAction"));
    LightAttackAction->ValueType = EInputActionValueType::Boolean; // 布尔值（左键）

    SprintAction = CreateDefaultSubobject<UInputAction>(TEXT("SprintAction"));
    SprintAction->ValueType = EInputActionValueType::Boolean;      // 布尔值（Shift）

    DodgeAction = CreateDefaultSubobject<UInputAction>(TEXT("DodgeAction"));
    DodgeAction->ValueType = EInputActionValueType::Boolean;       // 布尔值（F键）

    HeavyAttackAction = CreateDefaultSubobject<UInputAction>(TEXT("HeavyAttackAction"));
    HeavyAttackAction->ValueType = EInputActionValueType::Boolean; // 布尔值（右键）

    // 新增的输入动作
    StunSkillAction = CreateDefaultSubobject<UInputAction>(TEXT("StunSkillAction"));
    StunSkillAction->ValueType = EInputActionValueType::Boolean;   // 布尔值（Q键）

    DrinkPotionAction = CreateDefaultSubobject<UInputAction>(TEXT("DrinkPotionAction"));
    DrinkPotionAction->ValueType = EInputActionValueType::Boolean; // 布尔值（E键）

    // 测试功能按键
    TestDamageAction = CreateDefaultSubobject<UInputAction>(TEXT("TestDamageAction"));
    TestDamageAction->ValueType = EInputActionValueType::Boolean;   // 布尔值（T键）

    TestDeathAction = CreateDefaultSubobject<UInputAction>(TEXT("TestDeathAction"));
    TestDeathAction->ValueType = EInputActionValueType::Boolean;    // 布尔值（Y键）

    TestRespawnAction = CreateDefaultSubobject<UInputAction>(TEXT("TestRespawnAction"));
    TestRespawnAction->ValueType = EInputActionValueType::Boolean;  // 布尔值（U键）

    TestDetectAction = CreateDefaultSubobject<UInputAction>(TEXT("TestDetectAction"));
    TestDetectAction->ValueType = EInputActionValueType::Boolean;   // 布尔值（G键）

    // 🎪 创建输入映射上下文 - 定义按键到动作的映射关系
    InputMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("InputMappingContext"));

    // 🗺️ 配置按键映射 - 让键盘按键触发对应的InputAction
    // 移动系统：WASD映射到MoveAction的不同方向
    // D键 = +X（右移）
    InputMappingContext->MapKey(MoveAction, EKeys::D);

    // A键 = -X（左移，使用Negate修饰器取反）
    {
        auto& AMap = InputMappingContext->MapKey(MoveAction, EKeys::A);
        AMap.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("MoveNegateA")));
    }

    // W键 = +Y（前进，使用Swizzle把X轴贡献映射到Y轴）
    {
        auto& WMap = InputMappingContext->MapKey(MoveAction, EKeys::W);
        auto* SwizzleW = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("MoveSwizzleW"));
        SwizzleW->Order = EInputAxisSwizzle::YXZ; // X->Y, Y->X, Z->Z
        WMap.Modifiers.Add(SwizzleW);
    }

    // S键 = -Y（后退，先Swizzle再Negate）
    {
        auto& SMap = InputMappingContext->MapKey(MoveAction, EKeys::S);
        auto* SwizzleS = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("MoveSwizzleS"));
        SwizzleS->Order = EInputAxisSwizzle::YXZ; // 先把X贡献映射到Y
        SMap.Modifiers.Add(SwizzleS);
        SMap.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("MoveNegateS"))); // 再取负
    }
    // 👀 视角控制映射 - 鼠标移动控制相机
    // MouseX → 水平旋转（左右看）
    InputMappingContext->MapKey(LookAction, EKeys::MouseX);

    // MouseY → 垂直旋转（上下看），需要Swizzle映射
    {
        auto& MY = InputMappingContext->MapKey(LookAction, EKeys::MouseY);
        auto* SwizzleY = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("LookSwizzleY"));
        SwizzleY->Order = EInputAxisSwizzle::YXZ; // 把鼠标X轴贡献映射到Y轴
        MY.Modifiers.Add(SwizzleY);
        // 如需反转俯仰方向，可以取消下面的注释
        // MY.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("LookNegateY")));
    }

    // 🎯 其他动作按键映射 - 简单的一对一映射
    InputMappingContext->MapKey(JumpAction, EKeys::SpaceBar);          // 跳跃
    InputMappingContext->MapKey(LightAttackAction, EKeys::LeftMouseButton);  // 轻攻击
    InputMappingContext->MapKey(SprintAction, EKeys::LeftShift);        // 冲刺
    InputMappingContext->MapKey(DodgeAction, EKeys::F);                 // 闪避
    InputMappingContext->MapKey(HeavyAttackAction, EKeys::RightMouseButton); // 重攻击
    InputMappingContext->MapKey(StunSkillAction, EKeys::Q);             // 定身技能
    InputMappingContext->MapKey(DrinkPotionAction, EKeys::E);           // 喝药

    // 测试功能按键映射
    InputMappingContext->MapKey(TestDamageAction, EKeys::T);   // 测试受伤
    InputMappingContext->MapKey(TestDeathAction, EKeys::Y);    // 测试死亡
    InputMappingContext->MapKey(TestRespawnAction, EKeys::U);  // 测试重生
    InputMappingContext->MapKey(TestDetectAction, EKeys::G);   // 测试前方检测

    UE_LOG(LogTemp, Warning, TEXT("Constructor IMC Mapping count=%d"), InputMappingContext->GetMappings().Num());
    UE_LOG(LogTemp, Warning, TEXT("Actions valid? Move=%d Look=%d Jump=%d Light=%d Sprint=%d Dodge=%d Heavy=%d Test=%d"), MoveAction != nullptr, LookAction != nullptr, JumpAction != nullptr, LightAttackAction != nullptr, SprintAction != nullptr, DodgeAction != nullptr, HeavyAttackAction != nullptr, TestDamageAction != nullptr);

    // ==========================================
    // 📦 初始化角色属性和加载动画资源
    // ==========================================

    // 🎯 初始化战斗状态变量
    AtttackMyCount = 0;              // 连击计数从0开始
    bMyIsAttacking = false;          // 初始不在攻击状态
    CurrentActionState = EWukongActionState::Idle; // 初始为空闲状态

    // 🎬 加载轻攻击连击动画 - 包含4段连击的主蒙太奇文件
    // ⚠️ 重要：路径必须和你的实际资源路径匹配！
    static ConstructorHelpers::FObjectFinder<UAnimMontage> LightMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/PrimaryLightCombo_Montage")
    );
    if (LightMontageObj.Succeeded())
    {
        PrimaryMeleeMontage = LightMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ PrimaryMeleeMontage Loaded OK: %s"), *GetNameSafe(PrimaryMeleeMontage));

        // 🔍 验证动画资源是否包含所需段落
        if (PrimaryMeleeMontage->CompositeSections.Num() >= 4) {
            UE_LOG(LogTemp, Warning, TEXT("✅ 动画包含%d个段落，满足连击需求"), PrimaryMeleeMontage->CompositeSections.Num());
        }
        else {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ 动画只包含%d个段落，可能需要添加更多Attack段落"), PrimaryMeleeMontage->CompositeSections.Num());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PrimaryMeleeMontage FAILED TO LOAD!"));
        UE_LOG(LogTemp, Error, TEXT("Please check the path or assign it in Blueprint instead."));
    }

    // === 开局/重生蒙太奇（硬编码加载） ===
    static ConstructorHelpers::FObjectFinder<UAnimMontage> StartMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/SelectScreen_Start_Montage")
    );

    if (StartMontageObj.Succeeded())
    {
        StartMontage = StartMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ StartMontage Loaded: %s"), *GetNameSafe(StartMontage));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to load StartMontage"));
    }

    //D:\Black_Myth_Cpp\Content\ParagonSunWukong\Characters\Heroes\Wukong\Animations
    // 🎯 加载独立的攻击动画文件
    // Attack1动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> Attack1Obj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_A_Slow_Montage")
    );
    if (Attack1Obj.Succeeded()) {
        Attack1Montage = Attack1Obj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ Attack1Montage Loaded: %s"), *GetNameSafe(Attack1Montage));
    }

    // Attack2动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> Attack2Obj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_B_Slow_Montage")
    );
    if (Attack2Obj.Succeeded()) {
        Attack2Montage = Attack2Obj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ Attack2Montage Loaded: %s"), *GetNameSafe(Attack2Montage));
    }

    // Attack3动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> Attack3Obj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_C_Slow_Montage")
    );
    if (Attack3Obj.Succeeded()) {
        Attack3Montage = Attack3Obj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ Attack3Montage Loaded: %s"), *GetNameSafe(Attack3Montage));
    }

    // Attack4动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> Attack4Obj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_D_Slow_Montage")
    );
    if (Attack4Obj.Succeeded()) {
        Attack4Montage = Attack4Obj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ Attack4Montage Loaded: %s"), *GetNameSafe(Attack4Montage));
    }

    // 💥 加载重攻击动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> HeavyMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Primary_Melee_D_Slow_Montage")
    );
    if (HeavyMontageObj.Succeeded()) {
        HeavyAttackMontage = HeavyMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ HeavyAttackMontage Loaded: %s"), *GetNameSafe(HeavyAttackMontage));
    }
    else {
        // 如果找不到重攻击动画，使用第一段轻攻击作为替代
        UE_LOG(LogTemp, Warning, TEXT("⚠️ HeavyAttackMontage not found, using Attack1 as fallback"));
        HeavyAttackMontage = Attack1Montage;
    }

    // 💨 加载闪避动画 - 可选的闪避动画资源
    static ConstructorHelpers::FObjectFinder<UAnimMontage> DodgeMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Evade1")
    );

    if (DodgeMontageObj.Succeeded())
    {
        DodgeMontage = DodgeMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ DodgeMontage Loaded OK: %s"), *GetNameSafe(DodgeMontage));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ DodgeMontage not found, using programmatic dodge"));
        DodgeMontage = nullptr; // 如果没有动画就用程序化移动代替
    }


    // 💀 加载死亡动画 - 可选的死亡动画资源
    static ConstructorHelpers::FObjectFinder<UAnimMontage> DeathMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Death_Montage")
    );

    if (DeathMontageObj.Succeeded())
    {
        DeathMontage = DeathMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ DeathMontage Loaded OK: %s"), *GetNameSafe(DeathMontage));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ DeathMontage not found, using physics ragdoll"));
        DeathMontage = nullptr; // 如果没有动画就用物理效果代替
    }

    // 💫 加载定身技能动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> StunSkillMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/no") // 使用轻攻击动画作为替代
    );

    if (StunSkillMontageObj.Succeeded())
    {
        StunSkillMontage = StunSkillMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ StunSkillMontage Loaded OK: %s"), *GetNameSafe(StunSkillMontage));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ StunSkillMontage not found, using Attack1 as fallback"));
        StunSkillMontage = Attack1Montage;
    }

    // 🧪 加载喝药动画
    static ConstructorHelpers::FObjectFinder<UAnimMontage> DrinkPotionMontageObj(
        TEXT("/Game/ParagonSunWukong/Characters/Heroes/Wukong/Animations/Evade1") // 使用闪避动画作为替代
    );

    if (DrinkPotionMontageObj.Succeeded())
    {
        DrinkPotionMontage = DrinkPotionMontageObj.Object;
        UE_LOG(LogTemp, Warning, TEXT("✅ DrinkPotionMontage Loaded OK: %s"), *GetNameSafe(DrinkPotionMontage));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ DrinkPotionMontage not found, using DodgeMontage as fallback"));
        DrinkPotionMontage = DodgeMontage;
    }

    // 🔧 初始化所有游戏属性 - 设置角色的默认数值
    CurrentActionState = EWukongActionState::Idle; // 初始状态为空闲

    // 🔋 体力系统初始化
    CurrentStamina = MaxStamina = 100.f;          // 初始和最大体力都是100
    StaminaRecoveryRate = 10.f;                   // 每秒恢复10点体力
    LightAttackStaminaCost = 10.f;                // 轻攻击消耗10点体力
    DodgeStaminaCost = 20.f;                      // 闪避消耗20点体力
    HeavyAttackStaminaCost = 30.f;                // 重攻击消耗30点体力
    HeavyAttackDistance = 500.f;                  // 重攻击突进距离
    HeavyAttackDuration = 0.4f;                   // 重攻击突进持续时间
    StunSkillStaminaCost = 25.f;                  // 定身技能消耗25点体力

    // ❤️ 生命值系统初始化
    CurrentHealth = MaxHealth = 200.f;            // 初始和最大生命值都是200
    HealthRecoveryRate = 5.f;                     // 每秒恢复5点生命值
    bIsDead = false;                              // 初始不死亡

    // ⚔️ 伤害系统初始化
    LightAttackDamage = 20.f;                     // 轻攻击伤害20点
    HeavyAttackBaseDamage = 50.f;                 // 重攻击基础伤害50点
    StunSkillDamage = 30.f;                       // 定身技能伤害30点

    // 🔋 蓄力系统初始化
    CurrentChargeTime = 0.f;                      // 当前蓄力时间
    MaxChargeTime = 2.f;                          // 最大蓄力时间2秒
    MinChargeDamageMultiplier = 1.f;              // 最小蓄力伤害倍率
    MaxChargeDamageMultiplier = 3.f;              // 最大蓄力伤害倍率
    bIsCharging = false;                          // 初始不处于蓄力状态

    // ⚔️ 战斗系统初始化
    LightComboBufferWindow = 1.0f;                // 连击缓冲窗口1秒
    CurrentLightComboIndex = 0;                   // 连击索引从0开始
    bLightAttackQueued = false;                   // 初始没有排队的攻击
    bIsInvincible = false;                        // 初始不处于无敌状态

    // 💨 闪避系统初始化
    DodgeDistance = 300.f;                        // 闪避移动300单位距离
    DodgeSpeed = 1000.f;                          // 闪避速度（距离/时间）
    DodgeDuration = 0.5f;                         // 闪避持续0.5秒（无敌时间）
    DodgeCooldown = 0.5f;                         // 闪避冷却0.5秒
    bCanDodge = true;                             // 初始可以闪避
    LastMovementInput = FVector2D::ZeroVector;    // 最后移动输入为零向量

    // 💫 定身技能初始化
    StunDuration = 3.0f;                          // 定身持续3秒
    StunSkillRange = 300.f;                       // 定身技能范围300单位

    // 🧪 喝药系统初始化
    PotionCount = 3;                              // 初始有3瓶药水
    InstantHealAmount = 50.f;                     // 瞬间回复50点生命值
    OverTimeHealAmount = 30.f;                    // 持续回复30点生命值
    OverTimeHealDuration = 5.0f;                  // 持续5秒
    OverTimeHealInterval = 1.0f;                  // 每1秒回复一次
}

// 🔄 Tick函数 - 每帧执行的核心逻辑（通常每秒60次）
void AWukongCharacter::Tick(float DeltaTime) {
    Super::Tick(DeltaTime); // 调用父类的Tick函数

    // 🔍 动画状态调试（开发时使用，发布时可移除）
    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance()) {
        UAnimMontage* CurrentPlayingMontage = nullptr;
        FName CurrentSection = NAME_None;
        float CurrentPos = 0.0f;

        // 检查哪个攻击动画正在播放
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
        // 兼容PrimaryMeleeMontage
        else if (PrimaryMeleeMontage && AnimInst->Montage_IsPlaying(PrimaryMeleeMontage)) {
            CurrentPlayingMontage = PrimaryMeleeMontage;
            CurrentSection = AnimInst->Montage_GetCurrentSection(PrimaryMeleeMontage);
            CurrentPos = AnimInst->Montage_GetPosition(PrimaryMeleeMontage);
        }

        if (CurrentPlayingMontage) {
            static float LastPos = 0.0f;

            // 只有位置变化明显时才输出，避免日志刷屏
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

    // 🔋 体力恢复逻辑 - 持续性体力恢复
    if (CurrentActionState != EWukongActionState::Sprint) { // 冲刺时不恢复体力
        CurrentStamina += StaminaRecoveryRate * DeltaTime; // 恢复量 = 每秒恢复速率 × 时间间隔
        CurrentStamina = FMath::Min(CurrentStamina, MaxStamina); // 确保不超过最大值
    }

    // ❤️ 生命值恢复逻辑 - 持续性生命值恢复
    if (!bIsDead && CurrentHealth < MaxHealth) { // 死亡时不恢复，满了也不恢复
        CurrentHealth += HealthRecoveryRate * DeltaTime; // 恢复量 = 每秒恢复速率 × 时间间隔
        CurrentHealth = FMath::Min(CurrentHealth, MaxHealth); // 确保不超过最大值
    }

    // 🩺 状态调试信息（每秒输出一次）
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

    // 🔋 蓄力计时器逻辑 - 在蓄力状态下累积时间
    if (bIsCharging && CurrentActionState == EWukongActionState::HeavyCharge) {
        CurrentChargeTime += DeltaTime;
        CurrentChargeTime = FMath::Min(CurrentChargeTime, MaxChargeTime); // 确保不超过最大蓄力时间

        // 可以在这里添加蓄力效果，比如屏幕特效、音效等
        float ChargeRatio = CurrentChargeTime / MaxChargeTime;
        UE_LOG(LogTemp, Warning, TEXT("🔋 蓄力中... %.1f%% (%.2fs/%.2fs)"),
            ChargeRatio * 100.f, CurrentChargeTime, MaxChargeTime);
    }
    else if (bIsCharging && CurrentActionState != EWukongActionState::HeavyCharge) {
        // 如果状态改变但还在蓄力，取消蓄力
        CancelHeavyCharge();
    }

    // 🛡️ 动画防护措施 - 防止动画蓝图出bug
    PreventAnimationBlueprintDivisionByZero();     // 基础防护（目前为空）
    FixAnimationBlueprintVariables();              // 高级防护：同步变量到动画蓝图
}

// ==========================================
// C++ 逻辑实现：替代蓝图 Switch 逻辑
// ==========================================

/* 🎯 执行攻击逻辑 - 替代蓝图中的复杂Switch语句
   这个函数统一处理所有攻击的播放逻辑
   @param NewAtttackMyCount - 新的连击数（动画蓝图用这个决定显示哪段攻击）
   @param SectionName - 要播放的动画段落名（如"Attack1"） */
void AWukongCharacter::ExecuteAttackLogic(int32 NewAtttackMyCount, FName SectionName)
{
    // ⚡ === 执行攻击逻辑 ===
    UE_LOG(LogTemp, Warning, TEXT("⚡ ===== EXECUTE ATTACK LOGIC ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 参数: AttackCount=%d, Section=%s"), NewAtttackMyCount, *SectionName.ToString());

    // 步骤1：更新攻击状态
    bool OldAttacking = bMyIsAttacking;
    bMyIsAttacking = true;
    UE_LOG(LogTemp, Warning, TEXT("📊 攻击状态更新: %d → %d"), OldAttacking, bMyIsAttacking);

    // 步骤2：更新连击计数
    int32 OldCount = AtttackMyCount;
    AtttackMyCount = NewAtttackMyCount;
    UE_LOG(LogTemp, Warning, TEXT("🔢 连击计数更新: %d → %d"), OldCount, AtttackMyCount);

    // 步骤3：选择要播放的动画文件
    UAnimMontage* MontageToPlay = nullptr;

    // 根据AtttackMyCount选择对应的动画文件
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
        // 连击重置，不播放动画
        UE_LOG(LogTemp, Warning, TEXT("🔄 连击重置，不播放动画"));
        MontageToPlay = nullptr;
        break;
    default:
        UE_LOG(LogTemp, Error, TEXT("❌ 无效的AttackCount: %d"), AtttackMyCount);
        MontageToPlay = nullptr;
        break;
    }

    // 步骤4：播放选中的动画
    if (MontageToPlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎬 播放动画: %s"), *GetNameSafe(MontageToPlay));

        if (PlayMontageSafe(MontageToPlay, 1.5f, SectionName))
        {
            UE_LOG(LogTemp, Warning, TEXT("✅ 动画播放成功"));

            // ⚔️ 设置轻攻击伤害检测 - 在动画播放后0.3秒检测伤害
            GetWorldTimerManager().SetTimer(
                DodgeTimerHandle, // 复用计时器
                [this]() {
                    PerformLightAttackDamageDetection();
                },
                0.3f, // 动画开始后0.3秒检测伤害
                false
            );

            // 设置连击缓冲窗口
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

/* 🔄 重置攻击状态 - 连击窗口结束后清理状态
   防止状态变量一直保持true，导致动画蓝图行为异常 */
void AWukongCharacter::ResetAttackState()
{
    // 如果还在播放攻击动画，不要重置（避免打断连击）
    if (PrimaryMeleeMontage)
    {
        UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
        if (AnimInst && AnimInst->Montage_IsPlaying(PrimaryMeleeMontage))
            return; // 正在播放动画，不重置
    }

    // 如果有排队的攻击，也不要重置（保持连击连贯性）
    if (bLightAttackQueued)
        return; // 有缓存，不重置

    // 安全重置所有状态变量
    bMyIsAttacking = false;        // 不再攻击
    AtttackMyCount = 0;           // 连击数清零
    GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle); // 清理计时器
}


/* 🏷️ 获取段落名称 - 把连击索引转换为动画段落名
   动画蒙太奇中不同的攻击段落命名规则
   @param ComboIndex - 连击索引（0=第一段，1=第二段，2=第三段，3=第四段）
   @return FName - 对应的段落名 */
FName AWukongCharacter::GetSectionNameForComboIndex(int32 ComboIndex)
{
    switch (ComboIndex)
    {
    case 0: return FName(TEXT("Attack1")); // 第一段攻击动画
    case 1: return FName(TEXT("Attack2")); // 第二段攻击动画
    case 2: return FName(TEXT("Attack3")); // 第三段攻击动画
    case 3: return FName(TEXT("Attack4")); // 第四段攻击动画（通常是终结技）
    default: return FName(TEXT("Attack1")); // 默认播放第一段
    }
}

/**
 * @brief 根据连击索引调用 ExecuteAttackLogic
 */
void AWukongCharacter::PlayLightAttackMontage(int32 ComboIndex)
{
    // 🎯 === 播放指定连击段落的独立动画 ===
    UE_LOG(LogTemp, Warning, TEXT("🎬 ===== PLAY ATTACK MONTAGE ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 参数: ComboIndex=%d"), ComboIndex);

    // 根据连击索引选择对应的动画文件
    UAnimMontage* SelectedMontage = nullptr;
    int32 AttackCount = ComboIndex + 1; // 1, 2, 3, 4

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
        AttackCount = 4; // Attack4使用计数4
        UE_LOG(LogTemp, Warning, TEXT("🎯 选择Attack4动画文件（终结技）"));
        break;
    }

    if (SelectedMontage) {
        // 直接播放选中的独立动画文件
        ExecuteAttackLogic(AttackCount, FName(TEXT("Default"))); // 所有独立的动画都用Default段落
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

/* 🏃 移动函数 - 处理WASD移动输入
   @param Value - 输入值，包含X(左右)和Y(前后)方向 */
void AWukongCharacter::Move(const FInputActionValue& Value) {
    FVector2D MovementVector = Value.Get<FVector2D>();

    // 记录最后一次移动输入，用于闪避时确定方向
    LastMovementInput = MovementVector;

    if (Controller) {
        // 步骤1：获取控制器的旋转（只关心水平朝向）
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0); // 只取Yaw（水平旋转）

        // 步骤2：从旋转矩阵中提取前后左右方向向量
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X); // 前进方向
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);   // 右移方向

        // 步骤3：应用移动输入到角色
        AddMovementInput(ForwardDirection, MovementVector.Y); // Y轴控制前后移动
        AddMovementInput(RightDirection, MovementVector.X);   // X轴控制左右移动
    }
}

/* 👀 视角控制函数 - 处理鼠标移动的视角旋转
   @param Value - 输入值，包含X(左右转)和Y(上下看)方向 */
void AWukongCharacter::Look(const FInputActionValue& Value) {
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller) {
        // 步骤1：处理水平旋转（Yaw）- 无限制自由旋转
        AddControllerYawInput(LookAxisVector.X);

        // 步骤2：处理垂直旋转（Pitch）- 需要角度限制
        FRotator CurrentRotation = Controller->GetControlRotation();

        // 步骤3：计算新的俯仰角
        float NewPitch = CurrentRotation.Pitch + LookAxisVector.Y;

        // 步骤4：限制俯仰角范围（-80°到+45°）
        // 防止玩家看到天空地下，保持舒适的视角体验
        NewPitch = FMath::ClampAngle(NewPitch, -80.0f, 45.0f);

        // 步骤5：应用新的旋转角度
        FRotator NewRotation = CurrentRotation;
        NewRotation.Pitch = NewPitch;
        Controller->SetControlRotation(NewRotation);
    }
}

// ==========================================
// ⚔️ 战斗与动作逻辑 - 冲刺、闪避、重攻击等功能
// ==========================================

/* 🏃‍♂️ 开始冲刺 - 按下左Shift键时调用
   让角色进入高速移动状态，但会消耗体力 */
void AWukongCharacter::SprintStart() {
    UE_LOG(LogTemp, Warning, TEXT("SprintStart Triggered"));
    CurrentActionState = EWukongActionState::Sprint;          // 设置状态为冲刺
    GetCharacterMovement()->MaxWalkSpeed = 1000.0f;          // 冲刺速度：1000单位/秒
}

/* 🧍 结束冲刺 - 释放左Shift键时调用
   恢复正常移动速度 */
void AWukongCharacter::SprintStop() {
    UE_LOG(LogTemp, Warning, TEXT("SprintStop Triggered"));
    CurrentActionState = EWukongActionState::Idle;           // 恢复空闲状态
    GetCharacterMovement()->MaxWalkSpeed = 500.0f;           // 正常速度：500单位/秒
}

/* 💨 闪避函数 - 按F键执行的闪避动作
   快速移动一段距离，期间无敌，但消耗体力并有冷却时间 */
void AWukongCharacter::Dodge() {
    // 🛡️ 前置条件检查
    // 不能在闪避过程中再次闪避，也不能在冷却时间内闪避
    if (CurrentActionState == EWukongActionState::Dodge || !bCanDodge) {
        UE_LOG(LogTemp, Warning, TEXT("Dodge blocked: already dodging or cooling down"));
        return;
    }

    // 🔋 体力检查 - 确保有足够的体力进行闪避
    if (CurrentStamina < DodgeStaminaCost) {
        UE_LOG(LogTemp, Warning, TEXT("Dodge blocked: stamina low"));
        return;
    }

    // 🌤️ 空中状态检查 - 在空中时降低闪避效果
    bool bIsInAir = GetCharacterMovement()->IsFalling();
    float ActualDodgeDistance = DodgeDistance;
    if (bIsInAir) {
        // 在空中时，闪避距离减半，防止角色速度过快
        ActualDodgeDistance *= 0.5f; // 使用局部变量
        UE_LOG(LogTemp, Warning, TEXT("Dodge in air: distance reduced to %.1f"), ActualDodgeDistance);
    }

    // 💰 消耗体力 - 扣除闪避所需的体力值
    CurrentStamina -= DodgeStaminaCost;

    // === 动作打断逻辑 ===
    // 停止当前正在播放的动画
    if (GetMesh() && GetMesh()->GetAnimInstance()) {
        UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();

        // 停止所有蒙太奇播放
        AnimInst->StopAllMontages(0.1f); // 0.1秒的融合时间

        // 清理所有动画相关的定时器
        GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle);
        GetWorldTimerManager().ClearTimer(DodgeTimerHandle);
        GetWorldTimerManager().ClearTimer(DodgeCooldownTimerHandle);
    }

    // 重置状态变量
    bMyIsAttacking = false;
    bLightAttackQueued = false;
    CurrentLightComboIndex = 0;
    AtttackMyCount = 0;

    // === 开始闪避 ===
    CurrentActionState = EWukongActionState::Dodge;
    bIsInvincible = true;
    bCanDodge = false;

    // 计算闪避方向
    FVector DodgeDirection = GetActorForwardVector(); // 默认向前

    // 如果有移动输入，使用输入方向
    if (!LastMovementInput.IsNearlyZero()) {
        // 获取控制器的旋转
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // 计算基于输入的方向
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        DodgeDirection = (ForwardDirection * LastMovementInput.Y + RightDirection * LastMovementInput.X).GetSafeNormal();
    }

    // 播放闪避动画（如果有的话）
    if (DodgeMontage) {
        // 有动画：播放动画并同步移动
        PlayMontageSafe(DodgeMontage, 1.5f);

        // 动画驱动的移动 - 让动画自然控制角色移动
        // 检查是否应该使用Root Motion
        if (DodgeMontage->HasRootMotion()) {
            // 如果动画有Root Motion，让它驱动移动
            GetCharacterMovement()->SetMovementMode(MOVE_None); // 禁用物理移动，启用Root Motion
        }
        else {
            // 没有Root Motion，使用分步移动，保持正常物理
            FVector CurrentLocation = GetActorLocation();
            FVector DodgeTargetLocation = CurrentLocation + (DodgeDirection * ActualDodgeDistance);

            // 计算所需速度（距离/时间）
            float DodgeMoveTime = 0.3f; // 0.3秒完成移动
            FVector RequiredVelocity = (DodgeTargetLocation - CurrentLocation) / DodgeMoveTime;

            // 使用LaunchCharacter，但不覆盖Z方向，保持正常物理
            LaunchCharacter(RequiredVelocity, true, false); // XY覆盖，Z不覆盖
        }
    }
    else {
        // 无动画：纯程序化闪避
        // 使用分步移动，但保持正常物理属性
        FVector CurrentLocation = GetActorLocation();
        FVector DodgeTargetLocation = CurrentLocation + (DodgeDirection * ActualDodgeDistance);

        // 计算所需速度（距离/时间）
        float DodgeMoveTime = 0.07f; // 0.1秒完成移动
        FVector RequiredVelocity = (DodgeTargetLocation - CurrentLocation) / DodgeMoveTime;

        // 使用LaunchCharacter，但不覆盖Z方向，保持正常物理
        LaunchCharacter(RequiredVelocity, true, false); // XY覆盖，Z不覆盖

        UE_LOG(LogTemp, Warning, TEXT("Dodge without animation: Controlled move to target"));
    }

    // 设置闪避结束定时器
    GetWorldTimerManager().SetTimer(
        DodgeTimerHandle,
        [this]() {
            bIsInvincible = false;
            CurrentActionState = EWukongActionState::Idle;
            // 重新启用移动模式（如果被禁用了）
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            UE_LOG(LogTemp, Warning, TEXT("Dodge ended, back to Idle"));
        },
        DodgeDuration,
        false
    );

    // 设置冷却定时器
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

/* 💥 受伤函数 - 处理受到的伤害
   @param DamageAmount - 伤害数值
   @param DamageEvent - 伤害事件
   @param EventInstigator - 伤害发起者
   @param DamageCauser - 伤害来源
   @return 实际造成的伤害 */
float AWukongCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    // 如果已经死亡，不再受伤
    if (bIsDead)
    {
        return 0.f;
    }

    // 计算实际伤害（可以在这里添加伤害减免、护盾等逻辑）
    float ActualDamage = DamageAmount;

    // 扣除生命值
    CurrentHealth -= ActualDamage;
    CurrentHealth = FMath::Max(CurrentHealth, 0.f); // 确保不低于0

    UE_LOG(LogTemp, Warning, TEXT("💥 受到伤害: %.1f, 剩余生命值: %.1f/%.1f"),
        ActualDamage, CurrentHealth, MaxHealth);

    // 检查是否死亡
    if (CurrentHealth <= 0.f && !bIsDead)
    {
        Die();
    }

    return ActualDamage;
}

/* 💀 死亡处理 - 角色死亡时的逻辑 */
void AWukongCharacter::Die()
{
    if (bIsDead)
    {
        return; // 防止重复死亡
    }

    bIsDead = true;
    CurrentActionState = EWukongActionState::Idle; // 停止所有动作

    UE_LOG(LogTemp, Warning, TEXT("💀 角色死亡！生命值: %.1f"), CurrentHealth);

    // 停止所有动画
    if (GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->StopAllMontages(0.1f);
    }

    // 禁用移动
    GetCharacterMovement()->DisableMovement();

    // 设置死亡姿势（这里可以播放死亡动画）
    // TODO: 添加死亡动画

    // 延迟重生
    GetWorldTimerManager().SetTimer(
        DodgeTimerHandle, // 复用计时器
        [this]() {
            Respawn();
        },
        3.f, // 3秒后重生
        false
    );
}
/* 🔄 重生处理 - 角色重生时的逻辑 */
void AWukongCharacter::Respawn()
{
    UE_LOG(LogTemp, Warning, TEXT("🔄 角色重生！"));

    // 重置状态
    bIsDead = false;
    CurrentHealth = MaxHealth;
    CurrentStamina = MaxStamina;
    CurrentActionState = EWukongActionState::Idle;

    // 重新启用移动
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    // 重置位置到初始位置（或者检查点）
    SetActorLocation(FVector(0.f, 0.f, 100.f)); // 临时重生点

    // 重置朝向
    SetActorRotation(FRotator::ZeroRotator);
    UE_LOG(LogTemp, Warning, TEXT("✅ 重生完成，生命值: %.1f/%.1f"), CurrentHealth, MaxHealth);
}
/* ⚔️ 对目标造成伤害 - 用于攻击命中时的伤害应用
   @param Damage - 伤害数值
   @param Target - 目标角色 */
void AWukongCharacter::ApplyDamageToTarget(float Damage, AActor* Target)
{
    if (!Target)
    {
        return;
    }

    // 创建伤害事件 - 使用简化的伤害事件
    FDamageEvent DamageEvent;
    FHitResult HitResult; // 创建一个空的HitResult用于伤害事件
    FPointDamageEvent PointDamageEvent(Damage, HitResult, GetActorForwardVector(), nullptr);

    // 对目标造成伤害
    Target->TakeDamage(Damage, PointDamageEvent, GetController(), this);

    UE_LOG(LogTemp, Warning, TEXT("⚔️ 对目标造成伤害: %.1f"), Damage);
}

/* ⚔️ 执行重攻击伤害检测 - 检测命中目标并造成伤害
   @param Damage - 伤害数值
   @param AttackDirection - 攻击方向 */
void AWukongCharacter::PerformHeavyAttackDamageDetection(float Damage, FVector AttackDirection)
{
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== 重攻击伤害检测开始 ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 伤害数值: %.1f"), Damage);
    UE_LOG(LogTemp, Warning, TEXT("📍 攻击方向: (%.2f,%.2f,%.2f)"), AttackDirection.X, AttackDirection.Y, AttackDirection.Z);

    // 从角色位置开始，向攻击方向发射射线检测
    FVector StartLocation = GetActorLocation() + FVector(0, 0, 50); // 从稍微高一点的位置开始
    FVector EndLocation = StartLocation + (AttackDirection * 300.f); // 检测300单位距离内的目标

    UE_LOG(LogTemp, Warning, TEXT("📍 检测范围: 从(%.1f,%.1f,%.1f) 到(%.1f,%.1f,%.1f)"),
        StartLocation.X, StartLocation.Y, StartLocation.Z,
        EndLocation.X, EndLocation.Y, EndLocation.Z);

    // 射线检测参数
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // 忽略自己

    // 执行射线检测
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Pawn, // 检测Pawn（角色）
        QueryParams
    );

    // 调试绘制射线
    DrawDebugLine(GetWorld(), StartLocation, EndLocation,
        bHit ? FColor::Red : FColor::Green, false, 2.f, 0, 2.f);

    if (bHit)
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor)
        {
            float Distance = FVector::Distance(StartLocation, HitResult.Location);
            UE_LOG(LogTemp, Warning, TEXT("🎯 ✅ 前方有目标被打中!"));
            UE_LOG(LogTemp, Warning, TEXT("📋 命中详情: 目标=%s, 距离=%.1f米"), *HitActor->GetName(), Distance / 100.f);
            UE_LOG(LogTemp, Warning, TEXT("📍 命中位置: (%.1f,%.1f,%.1f)"), HitResult.Location.X, HitResult.Location.Y, HitResult.Location.Z);

            // 对命中的目标造成伤害
            ApplyDamageToTarget(Damage, HitActor);

            // 可以在这里添加击中特效、音效等
            UE_LOG(LogTemp, Warning, TEXT("⚔️ 造成伤害: %.1f"), Damage);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ ❌ 前方没有目标被打中"));
        UE_LOG(LogTemp, Warning, TEXT("💡 提示: 确保目标在检测范围内(300单位内)且为Pawn类型"));
    }

    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== 重攻击伤害检测结束 =====\n"));
}

/* ⚔️ 执行轻攻击伤害检测 - 检测命中目标并造成伤害 */
void AWukongCharacter::PerformLightAttackDamageDetection()
{
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== 轻攻击伤害检测开始 ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 伤害数值: %.1f"), LightAttackDamage);

    // 从角色位置开始，向前方发射射线检测
    FVector StartLocation = GetActorLocation() + FVector(0, 0, 50); // 从稍微高一点的位置开始
    FVector EndLocation = StartLocation + (GetActorForwardVector() * 200.f); // 检测200单位距离内的目标

    UE_LOG(LogTemp, Warning, TEXT("📍 检测范围: 从(%.1f,%.1f,%.1f) 到(%.1f,%.1f,%.1f)"),
        StartLocation.X, StartLocation.Y, StartLocation.Z,
        EndLocation.X, EndLocation.Y, EndLocation.Z);

    // 射线检测参数
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // 忽略自己

    // 执行射线检测
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Pawn, // 检测Pawn（角色）
        QueryParams
    );

    // 调试绘制射线
    DrawDebugLine(GetWorld(), StartLocation, EndLocation,
        bHit ? FColor::Red : FColor::Green, false, 2.f, 0, 2.f);

    if (bHit)
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor)
        {
            float Distance = FVector::Distance(StartLocation, HitResult.Location);
            UE_LOG(LogTemp, Warning, TEXT("🎯 ✅ 前方有目标被打中!"));
            UE_LOG(LogTemp, Warning, TEXT("📋 命中详情: 目标=%s, 距离=%.1f米"), *HitActor->GetName(), Distance / 100.f);
            UE_LOG(LogTemp, Warning, TEXT("📍 命中位置: (%.1f,%.1f,%.1f)"), HitResult.Location.X, HitResult.Location.Y, HitResult.Location.Z);

            // 对命中的目标造成伤害
            ApplyDamageToTarget(LightAttackDamage, HitActor);

            // 可以在这里添加击中特效、音效等
            UE_LOG(LogTemp, Warning, TEXT("⚔️ 造成伤害: %.1f"), LightAttackDamage);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ ❌ 前方没有目标被打中"));
        UE_LOG(LogTemp, Warning, TEXT("💡 提示: 确保目标在检测范围内(200单位内)且为Pawn类型"));
    }

    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== 轻攻击伤害检测结束 =====\n"));
}

// ==========================================
// 测试功能 - 调试和测试用
// ==========================================

/* 🩹 测试自己受伤 - 模拟受到伤害 */
void AWukongCharacter::TestTakeDamage()
{
    UE_LOG(LogTemp, Warning, TEXT("🩹 ===== TEST TAKE DAMAGE TRIGGERED ====="));
    UE_LOG(LogTemp, Warning, TEXT("🔍 函数被调用了！时间: %.2f"), GetWorld()->GetTimeSeconds());

    // 给自己造成30点伤害
    float TestDamage = 30.f;
    FDamageEvent DamageEvent;
    FHitResult HitResult; // 创建一个空的HitResult用于伤害事件
    FPointDamageEvent PointDamageEvent(TestDamage, HitResult, GetActorForwardVector(), nullptr);

    TakeDamage(TestDamage, PointDamageEvent, nullptr, nullptr);

    UE_LOG(LogTemp, Warning, TEXT("✅ 测试伤害应用完成: %.1f 伤害"), TestDamage);
}

/* 💀 测试死亡 - 直接触发死亡 */
void AWukongCharacter::TestDie()
{
    UE_LOG(LogTemp, Warning, TEXT("💀 ===== TEST DIE ====="));

    if (!bIsDead)
    {
        // 直接设置生命值为0来触发死亡
        CurrentHealth = 0.f;
        Die();
        UE_LOG(LogTemp, Warning, TEXT("✅ 测试死亡触发"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 已经死亡，无法再次测试死亡"));
    }
}

/* 🔄 测试重生 - 直接触发重生 */
void AWukongCharacter::TestRespawn()
{
    UE_LOG(LogTemp, Warning, TEXT("🔄 ===== TEST RESPAWN ====="));

    if (bIsDead)
    {
        // 取消死亡定时器，直接重生
        GetWorldTimerManager().ClearTimer(DodgeTimerHandle);
        Respawn();
        UE_LOG(LogTemp, Warning, TEXT("✅ 测试重生触发"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 未死亡，无法测试重生"));
    }
}

/* 👁️ 测试前方检测 - 检测前方是否有目标 */
void AWukongCharacter::TestFrontDetection()
{
    UE_LOG(LogTemp, Warning, TEXT("👁️ ===== TEST FRONT DETECTION ====="));

    // 从角色位置开始，向前方发射射线检测
    FVector StartLocation = GetActorLocation() + FVector(0, 0, 50); // 从稍微高一点的位置开始
    FVector EndLocation = StartLocation + (GetActorForwardVector() * 500.f); // 检测500单位距离内的目标

    // 射线检测参数
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // 忽略自己

    // 执行射线检测
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Pawn, // 检测Pawn（角色）
        QueryParams
    );

    // 调试绘制射线
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

/* 🔋 开始重攻击蓄力 - 按下右键时调用
   开始蓄力计时，准备重攻击 */
void AWukongCharacter::StartHeavyCharge() {
    UE_LOG(LogTemp, Warning, TEXT("🔋 ===== START HEAVY CHARGE ====="));

    // 检查是否可以开始蓄力
    if (CurrentActionState != EWukongActionState::Idle) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 无法开始蓄力：当前状态不是空闲"));
        return;
    }

    // 体力检查
    if (CurrentStamina < HeavyAttackStaminaCost) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 无法开始蓄力：体力不足"));
        return;
    }

    // 开始蓄力
    CurrentActionState = EWukongActionState::HeavyCharge;
    bIsCharging = true;
    CurrentChargeTime = 0.f;

    UE_LOG(LogTemp, Warning, TEXT("✅ 开始重攻击蓄力"));
}

/* 💥 释放重攻击 - 松开右键时调用
   根据蓄力时间释放重攻击 */
void AWukongCharacter::ReleaseHeavyAttack() {
    UE_LOG(LogTemp, Warning, TEXT("💥 ===== RELEASE HEAVY ATTACK ====="));

    // 如果没有在蓄力，直接返回
    if (!bIsCharging || CurrentActionState != EWukongActionState::HeavyCharge) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 没有在蓄力状态，忽略释放"));
        return;
    }

    // 计算蓄力倍率
    float ChargeRatio = FMath::Min(CurrentChargeTime / MaxChargeTime, 1.f);
    float DamageMultiplier = FMath::Lerp(MinChargeDamageMultiplier, MaxChargeDamageMultiplier, ChargeRatio);

    UE_LOG(LogTemp, Warning, TEXT("📊 蓄力时间: %.2fs, 倍率: %.2f"), CurrentChargeTime, DamageMultiplier);

    // 结束蓄力状态
    bIsCharging = false;
    CurrentActionState = EWukongActionState::Idle;

    // 执行重攻击（传递伤害倍率）
    ExecuteHeavyAttack(DamageMultiplier);

    UE_LOG(LogTemp, Warning, TEXT("✅ ===== HEAVY CHARGE RELEASED =====\n"));
}

/* ❌ 取消重攻击蓄力 - 当蓄力被打断时调用 */
void AWukongCharacter::CancelHeavyCharge() {
    if (bIsCharging && CurrentActionState == EWukongActionState::HeavyCharge) {
        UE_LOG(LogTemp, Warning, TEXT("❌ ===== CANCEL HEAVY CHARGE ====="));
        UE_LOG(LogTemp, Warning, TEXT("📊 取消蓄力，蓄力时间: %.2fs"), CurrentChargeTime);

        // 重置蓄力状态
        bIsCharging = false;
        CurrentChargeTime = 0.f;
        CurrentActionState = EWukongActionState::Idle;

        UE_LOG(LogTemp, Warning, TEXT("✅ 蓄力已取消"));
    }
}

/* 💥 执行重攻击 - 实际的重攻击逻辑
   @param DamageMultiplier - 伤害倍率（基于蓄力时间） */
void AWukongCharacter::ExecuteHeavyAttack(float DamageMultiplier) {
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== EXECUTE HEAVY ATTACK ====="));
    UE_LOG(LogTemp, Warning, TEXT("📊 伤害倍率: %.2f"), DamageMultiplier);

    // 存储伤害倍率，用于后续伤害计算
    float CurrentDamageMultiplier = DamageMultiplier;

    // 🛡️ 前置条件检查
    if (CurrentActionState == EWukongActionState::HeavyAttack ||
        CurrentActionState == EWukongActionState::Dodge) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 重攻击被阻止: 当前状态不适合"));
        return;
    }

    // 🔋 体力检查
    if (CurrentStamina < HeavyAttackStaminaCost) {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 重攻击被阻止: 体力不足"));
        return;
    }

    // 💰 消耗体力
    CurrentStamina -= HeavyAttackStaminaCost;

    // === 动作打断逻辑 ===
    if (GetMesh() && GetMesh()->GetAnimInstance()) {
        UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
        AnimInst->StopAllMontages(0.1f);
        GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle);
    }

    // 重置轻攻击状态
    bMyIsAttacking = false;
    bLightAttackQueued = false;
    CurrentLightComboIndex = 0;
    AtttackMyCount = 0;

    // === 开始重攻击 ===
    CurrentActionState = EWukongActionState::HeavyAttack;
    bMyIsAttacking = true;  // 设置攻击状态标志，让动画蓝图知道正在攻击

    // 计算实际伤害
    float ActualDamage = HeavyAttackBaseDamage * DamageMultiplier;
    UE_LOG(LogTemp, Warning, TEXT("⚔️ 重攻击伤害: 基础%.1f × 倍率%.2f = 实际%.1f"),
        HeavyAttackBaseDamage, DamageMultiplier, ActualDamage);

    // 🎬 播放重攻击动画
    if (HeavyAttackMontage) {
        PlayMontageSafe(HeavyAttackMontage, 0.7f, FName(TEXT("Default")));
        UE_LOG(LogTemp, Warning, TEXT("🎬 播放重攻击动画"));
        UE_LOG(LogTemp, Warning, TEXT("🎯 重击动画资源: %s"), *GetNameSafe(HeavyAttackMontage));
        UE_LOG(LogTemp, Warning, TEXT("📁 重击动画路径: %s"), *HeavyAttackMontage->GetPathName());
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("❌ HeavyAttackMontage 未加载！"));
    }

    // 🏃‍♂️ === 关键：立即执行向前突进 ===
    // 计算突进方向（默认为角色面向前方）
    FVector AttackDirection = GetActorForwardVector();

    // 如果有移动输入，使用输入方向（和闪避一样的逻辑）
    if (!LastMovementInput.IsNearlyZero() && Controller) {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AttackDirection = (ForwardDirection * LastMovementInput.Y + RightDirection * LastMovementInput.X).GetSafeNormal();
        UE_LOG(LogTemp, Warning, TEXT("📊 使用输入方向进行重攻击突进"));
    }

    // 计算目标位置和所需速度
    FVector CurrentLocation = GetActorLocation();
    FVector AttackTargetLocation = CurrentLocation + (AttackDirection * HeavyAttackDistance);

    // 考虑地形高度（防止掉下悬崖）
    FHitResult HitResult;
    FVector TraceStart = AttackTargetLocation + FVector(0, 0, 100);  // 从上方100单位开始
    FVector TraceEnd = AttackTargetLocation - FVector(0, 0, 500);    // 向下探测500单位

    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic)) {
        // 找到地面，调整目标位置高度
        AttackTargetLocation.Z = HitResult.Location.Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        UE_LOG(LogTemp, Warning, TEXT("🌍 检测到地面，调整高度到: %.1f"), AttackTargetLocation.Z);
    }

    // 计算所需速度
    FVector RequiredVelocity = (AttackTargetLocation - CurrentLocation) / HeavyAttackDuration;

    // 🚀 执行突进
    LaunchCharacter(RequiredVelocity, true, true); // XY和Z都覆盖

    UE_LOG(LogTemp, Warning, TEXT("🚀 重攻击突进: 方向(%.2f,%.2f,%.2f), 距离=%.1f, 速度=%.1f"),
        AttackDirection.X, AttackDirection.Y, AttackDirection.Z,
        HeavyAttackDistance, RequiredVelocity.Size());

    // ⚔️ 设置伤害检测定时器 - 在突进开始后0.2秒检测命中
    GetWorldTimerManager().SetTimer(
        ComboWindowTimerHandle, // 复用计时器
        [this, ActualDamage, AttackDirection]() {
            // 执行伤害检测和应用
            PerformHeavyAttackDamageDetection(ActualDamage, AttackDirection);
        },
        0.2f, // 突进开始后0.2秒检测伤害
        false
    );

    // ⏰ 设置重攻击结束定时器
    GetWorldTimerManager().SetTimer(
        DodgeTimerHandle,  // 可以复用这个计时器
        [this]() {
            // 重攻击结束
            if (CurrentActionState == EWukongActionState::HeavyAttack) {
                CurrentActionState = EWukongActionState::Idle;
                bMyIsAttacking = false;  // 重置攻击状态标志
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
    // 📋 === 轻攻击函数开始 ===
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== LIGHT ATTACK TRIGGERED ====="));

    // 🔧 基础检查
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

    // 📊 当前状态快照 - 检查是否有任何攻击动画在播放
    bool IsPlayingAttack = false;
    FName CurrentSection = NAME_None;

    // 检查所有攻击动画是否在播放
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

    // 兼容旧的PrimaryMeleeMontage检查
    if (!IsPlayingAttack && PrimaryMeleeMontage && AnimInst->Montage_IsPlaying(PrimaryMeleeMontage)) {
        IsPlayingAttack = true;
        CurrentSection = AnimInst->Montage_GetCurrentSection(PrimaryMeleeMontage);
    }

    UE_LOG(LogTemp, Warning, TEXT("📊 当前状态: 正在攻击=%d, 当前段落=%s, 连击索引=%d, 缓冲中=%d"),
        IsPlayingAttack, *CurrentSection.ToString(), CurrentLightComboIndex, bLightAttackQueued);

    if (IsPlayingAttack)
    {
        // ✅ 触发连击缓冲机制
        bLightAttackQueued = true;
        float CurrentPos = AnimInst->Montage_GetPosition(PrimaryMeleeMontage);
        float TotalLength = PrimaryMeleeMontage->GetPlayLength();
        float Progress = TotalLength > 0.0f ? (CurrentPos / TotalLength) * 100.0f : 0.0f;

        UE_LOG(LogTemp, Warning, TEXT("✅ 【连击缓冲触发】段落:%s, 进度:%.1f%%, 位置:%.2fs/%.2fs"),
            *CurrentSection.ToString(), Progress, CurrentPos, TotalLength);
        UE_LOG(LogTemp, Warning, TEXT("🔄 缓冲状态已设置，等待当前攻击结束"));
        return;
    }

    // 🚀 开始新的连击序列
    UE_LOG(LogTemp, Warning, TEXT("🚀 【开始新连击】重置连击状态"));
    CurrentLightComboIndex = 0;
    bLightAttackQueued = false;
    bMyIsAttacking = true;
    AtttackMyCount = 1;

    // 🎯 执行第一段攻击（PlayMontageSafe会自动绑定回调）
    PlayLightAttackMontage(0);

    UE_LOG(LogTemp, Warning, TEXT("✅ 【连击启动】Attack1开始播放，等待动画结束"));
    UE_LOG(LogTemp, Warning, TEXT("🎯 ===== LIGHT ATTACK END =====\n"));
}

void AWukongCharacter::OnLightAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 📋 === 动画结束事件处理开始 ===
    UE_LOG(LogTemp, Warning, TEXT("🏁 ===== ANIMATION ENDED ====="));

    // 📊 分析结束原因 - 根据结束的动画文件确定当前段落
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
        // 重攻击结束，直接返回空闲状态
        CurrentActionState = EWukongActionState::Idle;
        UE_LOG(LogTemp, Warning, TEXT("🏁 重攻击动画结束"));
        return;
    }
    else {
        // 兼容旧的PrimaryMeleeMontage
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
        // 🎯 连击继续 - 处理缓冲的攻击
        UE_LOG(LogTemp, Warning, TEXT("✅ 【连击继续】检测到缓冲的攻击，开始处理"));

        bLightAttackQueued = false;
        int32 PreviousIndex = CurrentLightComboIndex;
        CurrentLightComboIndex++;

        // 🔄 处理连击结束
        if (CurrentLightComboIndex >= 4) {
            // Attack4结束后连击完成
            UE_LOG(LogTemp, Warning, TEXT("🎉 【连击完成】Attack4结束，连击序列完成"));
            ClearComboQueue();
            UE_LOG(LogTemp, Warning, TEXT("🏁 ===== COMBO SEQUENCE COMPLETE =====\n"));
            return;
        }
        else {
            UE_LOG(LogTemp, Warning, TEXT("➡️ 【连击递进】%d → %d"), PreviousIndex, CurrentLightComboIndex);
        }

        // 🎯 执行下一段攻击（PlayMontageSafe会自动绑定回调）
        int32 NextAttackNumber = CurrentLightComboIndex + 1;
        UE_LOG(LogTemp, Warning, TEXT("🎯 【执行下一击】播放Attack%d"), NextAttackNumber);
        PlayLightAttackMontage(CurrentLightComboIndex);

        UE_LOG(LogTemp, Warning, TEXT("✅ 【连击继续完成】等待下一段动画结束"));
    }
    else
    {
        // 🛑 连击结束
        UE_LOG(LogTemp, Warning, TEXT("🛑 【连击结束】没有缓冲的攻击，序列完成"));
        ClearComboQueue();
        UE_LOG(LogTemp, Warning, TEXT("🎉 【连击序列完成】所有攻击播放完毕"));
    }

    UE_LOG(LogTemp, Warning, TEXT("🏁 ===== ANIMATION END PROCESSED =====\n"));
}

void AWukongCharacter::ClearComboQueue()
{
    // 🧹 === 连击状态清理 ===
    UE_LOG(LogTemp, Warning, TEXT("🧹 ===== CLEAR COMBO QUEUE ====="));

    // 📋 记录清理前的状态（用于调试）
    bool WasQueued = bLightAttackQueued;
    int32 WasComboIndex = CurrentLightComboIndex;
    bool WasAttacking = bMyIsAttacking;
    int32 WasAttackCount = AtttackMyCount;

    UE_LOG(LogTemp, Warning, TEXT("📋 清理前状态: 缓冲=%d, 连击索引=%d, 攻击中=%d, 计数=%d"),
        WasQueued, WasComboIndex, WasAttacking, WasAttackCount);

    // 🔄 执行状态重置
    bLightAttackQueued = false;
    CurrentLightComboIndex = 0;
    bMyIsAttacking = false;
    AtttackMyCount = 0;
    // 只有在不是重攻击时才重置为Idle
    if (CurrentActionState != EWukongActionState::HeavyAttack) {
        CurrentActionState = EWukongActionState::Idle;
    }

    UE_LOG(LogTemp, Warning, TEXT("🔄 状态已重置: 缓冲=false, 索引=0, 攻击=false, 计数=0"));

    // ⏰ 清理计时器
    GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle);
    UE_LOG(LogTemp, Warning, TEXT("⏰ 连击缓冲计时器已清理"));

    // 🎬 停止动画播放（除了重攻击）
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
// PossessedBy / BeginPlay / Input binding 等（和你原来保持一致） 
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

    // =============================== 
    // Enhanced Input Runtime IMC 绑定
    // =============================== 
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

                    // WASD/方向键映射
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

                    // Look
                    InputMappingContext->MapKey(LookAction, EKeys::MouseX);
                    {
                        auto& MY = InputMappingContext->MapKey(LookAction, EKeys::MouseY);
                        auto* SwzY = NewObject<UInputModifierSwizzleAxis>(this);
                        SwzY->Order = EInputAxisSwizzle::YXZ;
                        MY.Modifiers.Add(SwzY);
                    }

                    // 其他动作
                    InputMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
                    InputMappingContext->MapKey(LightAttackAction, EKeys::LeftMouseButton);
                    InputMappingContext->MapKey(SprintAction, EKeys::LeftShift);
                    InputMappingContext->MapKey(DodgeAction, EKeys::LeftControl);
                    InputMappingContext->MapKey(HeavyAttackAction, EKeys::RightMouseButton);
                }

                Subsystem->AddMappingContext(InputMappingContext, 100);
                UE_LOG(LogTemp, Warning, TEXT("Runtime IMC applied in BeginPlay. MappingCount=%d"),
                    InputMappingContext->GetMappings().Num());
            }
        }
    }
    // --- 播放开局/重生蒙太奇 ---
    if (StartMontage)
    {
        PlayMontageSafe(StartMontage, 1.0f, FName(TEXT("UpperBody")));
        UE_LOG(LogTemp, Warning, TEXT("have been Playing StartMontage with UpperBody slot"));
    }


    // ===============================
    // 防止动画蓝图干扰：完全依赖 C++ 变量
    // ===============================
    bMyIsAttacking = false;
    bLightAttackQueued = false;
    CurrentLightComboIndex = 0;
    AtttackMyCount = 0;
    CurrentActionState = EWukongActionState::Idle;

    // 🔍 检查动画资源的段落信息
    if (PrimaryMeleeMontage) {
        UE_LOG(LogTemp, Warning, TEXT("📋 动画资源检查: %s"), *GetNameSafe(PrimaryMeleeMontage));
        UE_LOG(LogTemp, Warning, TEXT("📊 总段落数: %d"), PrimaryMeleeMontage->CompositeSections.Num());

        for (int32 i = 0; i < PrimaryMeleeMontage->CompositeSections.Num(); i++) {
            FCompositeSection& Section = PrimaryMeleeMontage->CompositeSections[i];
            UE_LOG(LogTemp, Warning, TEXT("  段落%d: %s"),
                i, *Section.SectionName.ToString());
        }

        // 检查我们需要的段落是否存在
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

    // 检查重攻击动画
    if (HeavyAttackMontage) {
        UE_LOG(LogTemp, Warning, TEXT("✅ HeavyAttackMontage已加载: %s"), *GetNameSafe(HeavyAttackMontage));
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("❌ HeavyAttackMontage未加载！"));
    }

    // 如果需要，可以直接初始化动画蓝图的变量防止读取异常
    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
    {
        AnimInst->Montage_Stop(0.0f); // 停止所有蒙太奇
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
    // 🛡️ === 安全播放动画 ===
    UE_LOG(LogTemp, Warning, TEXT("🛡️ ===== PLAY MONTAGE SAFE ====="));

    // 🔧 基础验证
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

    // 📊 参数信息
    UE_LOG(LogTemp, Warning, TEXT("📊 播放参数: Montage=%s, PlayRate=%.1f, StartSection=%s"),
        *GetNameSafe(Montage), InPlayRate, *StartSection.ToString());

    // 🎯 段落跳转（如果指定了起始段落）
    if (StartSection != NAME_None) {
        AnimInst->Montage_JumpToSection(StartSection, Montage);
        UE_LOG(LogTemp, Warning, TEXT("🎯 已跳转到段落: %s"), *StartSection.ToString());
    }

    // 🎬 执行播放
    float Played = AnimInst->Montage_Play(Montage, InPlayRate);

    // 📈 结果分析
    FName ActualSection = AnimInst->Montage_GetCurrentSection(Montage);
    float TotalLength = Montage->GetPlayLength();
    float ActualPlayTime = Played > 0.0f ? TotalLength / InPlayRate : 0.0f;

    UE_LOG(LogTemp, Warning, TEXT("🎬 播放结果: 返回值=%.2f, 当前段落=%s, 总时长=%.2fs, 播放时长≈%.2fs"),
        Played, *ActualSection.ToString(), TotalLength, ActualPlayTime);

    bool Success = Played > 0.0f;

    // 🔗 如果播放成功，为这个动画绑定结束回调
    if (Success) {
        FOnMontageEnded MontageEndedDelegate;

        // 根据动画类型绑定不同的回调
        if (Montage == HeavyAttackMontage) {
            // 重攻击的回调
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
            // 轻攻击的回调（使用原来的）
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
    // 简化的动画蓝图防护：只设置最关键的变量防止除零
    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance()) {
        // 同步攻击状态 - 确保动画蓝图和C++代码的攻击状态一致
        if (PrimaryMeleeMontage && AnimInst->Montage_IsPlaying(PrimaryMeleeMontage)) {
            bMyIsAttacking = true;
        }
        else {
            bMyIsAttacking = false;
        }

        // 确保AtttackMyCount不为0（防止除零错误）
        if (AtttackMyCount == 0 && bMyIsAttacking) {
            AtttackMyCount = 1;
        }

        // 设置基本的安全变量
        AnimInst->SetMorphTarget(FName(TEXT("Speed")), FMath::Max(GetVelocity().Size(), 0.1f));
        AnimInst->SetMorphTarget(FName(TEXT("ComboIndex")), (float)CurrentLightComboIndex);
    }
}

// ==========================================
// 防止动画蓝图除零错误的保护措施
// ==========================================

void AWukongCharacter::PreventAnimationBlueprintDivisionByZero() {

}

// ==========================================
// 输入绑定 (SetupPlayerInputComponent)
// ==========================================

void AWukongCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // 先防止 Action 为空导致"blank action"警告 
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
    if (!TestDamageAction) {
        TestDamageAction = NewObject<UInputAction>(this, TEXT("IA_TestDamage"));
        TestDamageAction->ValueType = EInputActionValueType::Boolean;
        UE_LOG(LogTemp, Warning, TEXT("🆕 创建TestDamageAction"));
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("✅ TestDamageAction已存在"));
    }
    if (!TestDeathAction) {
        TestDeathAction = NewObject<UInputAction>(this, TEXT("IA_TestDeath"));
        TestDeathAction->ValueType = EInputActionValueType::Boolean;
        UE_LOG(LogTemp, Warning, TEXT("🆕 创建TestDeathAction"));
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("✅ TestDeathAction已存在"));
    }
    if (!TestRespawnAction) {
        TestRespawnAction = NewObject<UInputAction>(this, TEXT("IA_TestRespawn"));
        TestRespawnAction->ValueType = EInputActionValueType::Boolean;
        UE_LOG(LogTemp, Warning, TEXT("🆕 创建TestRespawnAction"));
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("✅ TestRespawnAction已存在"));
    }
    if (!TestDetectAction) {
        TestDetectAction = NewObject<UInputAction>(this, TEXT("IA_TestDetect"));
        TestDetectAction->ValueType = EInputActionValueType::Boolean;
        UE_LOG(LogTemp, Warning, TEXT("🆕 创建TestDetectAction"));
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("✅ TestDetectAction已存在"));
    }
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

                    // 测试功能按键映射
                    InputMappingContext->MapKey(TestDamageAction, EKeys::T);
                    InputMappingContext->MapKey(TestDeathAction, EKeys::Y);
                    InputMappingContext->MapKey(TestRespawnAction, EKeys::U);
                    InputMappingContext->MapKey(TestDetectAction, EKeys::G);

                    // 新增功能按键映射
                    InputMappingContext->MapKey(StunSkillAction, EKeys::Q);
                    InputMappingContext->MapKey(DrinkPotionAction, EKeys::E);
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
            // 蓄力系统：按下开始蓄力，松开释放攻击
            EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AWukongCharacter::StartHeavyCharge);
            EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Completed, this, &AWukongCharacter::ReleaseHeavyAttack);
        }

        // 测试功能绑定
        if (TestDamageAction) {
            EnhancedInputComponent->BindAction(TestDamageAction, ETriggerEvent::Started, this, &AWukongCharacter::TestTakeDamage);
            UE_LOG(LogTemp, Warning, TEXT("🔗 绑定测试伤害按键 (T键)"));
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TestDamageAction 为空！"));
        }
        if (TestDeathAction) {
            EnhancedInputComponent->BindAction(TestDeathAction, ETriggerEvent::Started, this, &AWukongCharacter::TestDie);
            UE_LOG(LogTemp, Warning, TEXT("🔗 绑定测试死亡按键 (Y键)"));
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TestDeathAction 为空！"));
        }
        if (TestRespawnAction) {
            EnhancedInputComponent->BindAction(TestRespawnAction, ETriggerEvent::Started, this, &AWukongCharacter::TestRespawn);
            UE_LOG(LogTemp, Warning, TEXT("🔗 绑定测试重生按键 (U键)"));
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TestRespawnAction 为空！"));
        }
        if (TestDetectAction) {
            EnhancedInputComponent->BindAction(TestDetectAction, ETriggerEvent::Started, this, &AWukongCharacter::TestFrontDetection);
            UE_LOG(LogTemp, Warning, TEXT("🔗 绑定测试检测按键 (G键)"));
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("❌ TestDetectAction 为空！"));
        }

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

// 执行定身技能
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

    // 检测前方敌人
    FVector ForwardVector = GetActorForwardVector();
    FVector StartLocation = GetActorLocation() + FVector(0, 0, 50); // 稍微抬高起点
    FVector EndLocation = StartLocation + ForwardVector * StunSkillRange;

    // 使用球形重叠检测敌人
    TArray<FHitResult> HitResults;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(100.f); // 检测半径
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // 忽略自己

    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults,
        StartLocation,
        EndLocation,
        FQuat::Identity,
        ECC_Pawn,
        CollisionShape,
        QueryParams
    );

    if (bHit && HitResults.Num() > 0) {
        UE_LOG(LogTemp, Warning, TEXT("🎯 检测到前方有 %d 个目标"), HitResults.Num());

        // 对每个击中的敌人施加定身效果
        for (const FHitResult& Hit : HitResults) {
            if (Hit.GetActor() && Hit.GetActor() != this) {
                AParagonFengMao* Enemy = Cast<AParagonFengMao>(Hit.GetActor());
                if (Enemy && !Enemy->bIsDead) {
                    UE_LOG(LogTemp, Warning, TEXT("💫 对敌人 %s 施加定身效果"), *Enemy->GetName());
                    ApplyStunToTarget(Enemy);
                }
            }
        }
    }
    else {
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