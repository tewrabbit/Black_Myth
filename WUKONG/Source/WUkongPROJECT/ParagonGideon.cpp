// ParagonGideon.cpp
#include "ParagonGideon.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
// 包含小怪头文件
#include "ParagonNarbash.h"
#include "ParagonRampage.h"

AParagonGideon::AParagonGideon()
{
    // 设置Boss默认值，继承自ParagonFengMao但具有Boss特性
    
    // 高血量设计（符合Boss定位）
    MaxHealth = 800.f;  // 远高于普通敌人
    CurrentHealth = MaxHealth;
    
    // 攻击力随阶段提升
    AttackDamage = 35.f;  // 基础攻击力高于普通敌人
    HeavyAttackDamage = 60.f;
    
    // 移动速度适中（Boss不应过于敏捷）
    PatrolSpeed = 280.f;
    ChaseSpeed = 450.f;
    
    // 检测范围较大（Boss感知能力强）
    DetectionRange = 1200.f;
    AttackRange = 200.f;
    
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
    
    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon Boss初始化完成，进入第一阶段"));
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
        AttackDamage = 50.f;     // 攻击力提升
        HeavyAttackDamage = 80.f;
        ChaseSpeed *= 1.2f;      // 移动速度提升20%
        
        // 使用特殊技能
        UseSpecialAbility();
        
        UE_LOG(LogTemp, Warning, TEXT("👹 Gideon进入二阶段！属性全面提升"));
        break;
        
    case EBossPhase::Phase3:
        // 三阶段狂暴化
        AttackDamage = 70.f;     // 攻击力大幅提升
        HeavyAttackDamage = 120.f;
        ChaseSpeed *= 1.4f;      // 移动速度提升40%
        
        // 进入狂暴状态
        EnterEnrageMode();
        
        UE_LOG(LogTemp, Warning, TEXT("👹👹 Gideon进入三阶段！狂暴状态！"));
        break;
    }
}

void AParagonGideon::AttackPlayer()
{
    // 根据当前阶段调整攻击行为
    switch (CurrentPhase)
    {
    case EBossPhase::Phase1:
        // 第一阶段使用基础攻击
        Super::AttackPlayer();
        break;
        
    case EBossPhase::Phase2:
        // 第二阶段30%概率使用特殊技能
        if (FMath::FRand() < 0.3f && !bHasShield)
        {
            ActivateShield(); // 30%概率激活护盾
        }
        else if (FMath::FRand() < 0.2f)
        {
            PerformAreaAttack(); // 20%概率范围攻击
        }
        else
        {
            Super::AttackPlayer(); // 其他情况使用基础攻击
        }
        break;
        
    case EBossPhase::Phase3:
        // 第三阶段50%概率使用特殊技能
        if (FMath::FRand() < 0.3f && !bHasShield)
        {
            ActivateShield(); // 30%概率激活护盾
        }
        else if (FMath::FRand() < 0.3f)
        {
            PerformAreaAttack(); // 30%概率范围攻击
        }
        else if (FMath::FRand() < 0.2f)
        {
            SummonMinions(); // 20%概率召唤小怪
        }
        else
        {
            Super::AttackPlayer(); // 其他情况使用基础攻击
        }
        break;
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
        PerformAreaAttack(); // 默认范围攻击
        break;
    }
}

void AParagonGideon::PerformAreaAttack()
{
    if (bIsDead) return;
    
    UE_LOG(LogTemp, Warning, TEXT("👹 Gideon释放范围攻击！"));
    
    // 创建范围攻击效果（实际实现可在蓝图中完成）
    // 这里可以添加对周围玩家造成伤害的逻辑
    
    // 播放范围攻击动画（如果有的话）
    if (SkillMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Play(SkillMontage, 1.0f);
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
        // 计算小怪生成位置（围绕Boss分布）
        FVector SpawnLocation = BossLocation + FVector(FMath::RandRange(-200.f, 200.f), FMath::RandRange(-200.f, 200.f), 0.f);
        
        // 随机选择一种小怪类型
        TSubclassOf<AParagonFengMao> MinionClass;
        switch (i)
        {
        case 0:
            MinionClass = AParagonNarbash::StaticClass();
            break;
        case 1:
            MinionClass = AParagonRampage::StaticClass();
            break;
        case 2:
        default:
            MinionClass = AParagonFengMao::StaticClass();
            break;
        }
        
        // 生成小怪
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        
        AParagonFengMao* Minion = World->SpawnActor<AParagonFengMao>(MinionClass, SpawnLocation, BossRotation, SpawnParams);
        if (Minion)
        {
            // 削弱小怪能力（降至50%）
            Minion->MaxHealth *= 0.5f;
            Minion->CurrentHealth = Minion->MaxHealth;
            Minion->AttackDamage *= 0.5f;
            Minion->HeavyAttackDamage *= 0.5f;
            Minion->PatrolSpeed *= 0.7f;
            Minion->ChaseSpeed *= 0.7f;
            
            UE_LOG(LogTemp, Warning, TEXT("👹 召唤了削弱版小怪: %s"), *MinionClass->GetName());
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
    AttackDamage *= 1.2f;
    ChaseSpeed *= 1.1f;
    
    // 狂暴状态持续一段时间后自动解除
    if (GetWorld())
    {
        GetWorldTimerManager().SetTimer(EnrageTimerHandle, [this]() {
            bIsEnraged = false;
            UE_LOG(LogTemp, Warning, TEXT("👹👹 Gideon狂暴状态结束"));
        }, 20.0f, false);
    }
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