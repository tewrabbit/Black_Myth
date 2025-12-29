// ParagonGideon.h
#pragma once

#include "CoreMinimal.h"
#include "ParagonFengMao.h"
#include "ParagonNarbash.h"
#include "ParagonRampage.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimMontage.h"
#include "ParagonGideon.generated.h"
// Boss阶段枚举
UENUM(BlueprintType)
enum class EBossPhase : uint8
{
    Phase1 UMETA(DisplayName = "第一阶段"),
    Phase2 UMETA(DisplayName = "第二阶段"),
    Phase3 UMETA(DisplayName = "第三阶段")
};

// 技能类型枚举
UENUM(BlueprintType)
enum class ESkillType : uint8
{
    Fire UMETA(DisplayName = "火焰"),
    Ice UMETA(DisplayName = "冰霜"),
    Dark UMETA(DisplayName = "黑暗")
};

UCLASS()
class BLACK_MYTH_CPP_API AParagonGideon : public AParagonFengMao
{
    GENERATED_BODY()

public:
    // 构造函数
    AParagonGideon();

    UFUNCTION(BlueprintCallable, Category = "Boss")
    FVector GetBossLocation() const;

protected:
    // 生命周期函数
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 重写关键函数实现Boss特有行为
    virtual void AttackPlayer() override;
    virtual void Die() override;
    // Boss特有函数
    void CheckPhaseTransition();
    void TransitionToPhase(EBossPhase NewPhase);
    void UseSpecialAbility();
    void SummonMinions();
    void ActivateShield();
    void DeactivateShield();
    void EnterEnrageMode();
    void ApplyMeleeDamage();

    // 攻击特效处理函数
    ESkillType GetRandomSkillTypeForPhase();
    void SpawnAttackEffectAtLocation(ESkillType SkillType, FVector Location);
    void SpawnChestEffect();
    void ApplyAttackEffectDamage(ESkillType SkillType, FVector Location);

    // 持续伤害处理函数
    void StartContinuousDamage(ESkillType SkillType, FVector Location);
    void ApplyContinuousDamage();

    // 特效资源
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSubclassOf<AActor> FireSkillEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSubclassOf<AActor> IceSkillEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSubclassOf<AActor> DarkSkillEffectClass;

    // 胸前特效资源
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSubclassOf<AActor> ChestEffectClass;

    // 动画同步相关
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float AttackEffectDelay = 0.5f;  // 攻击特效延迟时间

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float DamageDelay = 0.5f;        // 伤害延迟时间

private:
    // 阶段系统
    UPROPERTY(VisibleAnywhere, Category = "Boss")
    EBossPhase CurrentPhase;

    UPROPERTY(EditAnywhere, Category = "Boss Stats")
    float Phase2Threshold = 0.7f; // 70%血量时进入二阶段

    UPROPERTY(EditAnywhere, Category = "Boss Stats")
    float Phase3Threshold = 0.3f; // 30%血量时进入三阶段

    // 特殊机制相关
    UPROPERTY(VisibleAnywhere, Category = "Boss Abilities")
    bool bHasShield;

    UPROPERTY(VisibleAnywhere, Category = "Boss Abilities")
    bool bIsEnraged;

    UPROPERTY(EditAnywhere, Category = "Boss Abilities")
    float SpecialAbilityCooldown = 15.f; // 特殊技能冷却时间

    UPROPERTY(EditAnywhere, Category = "Boss Abilities")
    float ShieldDuration = 10.f; // 护盾持续时间

    UPROPERTY(EditAnywhere, Category = "Boss Abilities")
    int32 MinionsToSummon = 3; // 每次召唤的小怪数量

    // 近战攻击伤害
    UPROPERTY(EditAnywhere, Category = "Boss Stats")
    float MeleeAttackDamage = 4.f; // 近战攻击伤害

    // 小怪蓝图类
    UPROPERTY(EditAnywhere, Category = "Minions")
    TSubclassOf<AParagonFengMao> FengMaoMinionClass;

    UPROPERTY(EditAnywhere, Category = "Minions")
    TSubclassOf<AParagonNarbash> NarbashMinionClass;

    UPROPERTY(EditAnywhere, Category = "Minions")
    TSubclassOf<AParagonRampage> RampageMinionClass;

    // Gideon专用攻击冷却时间（延长1秒）
    float GideonAttackCooldown;

    // 上次攻击时间记录
    float LastGideonAttackTime;

    // 特效伤害间隔控制
    float LastEffectDamageTime;  // 上次特效伤害时间
    float EffectDamageInterval;  // 特效伤害间隔时间

    // 持续伤害控制
    FTimerHandle ContinuousDamageTimerHandle;  // 持续伤害定时器句柄
    int32 MaxContinuousDamageTicks;           // 最大持续伤害次数
    int32 CurrentContinuousDamageTicks;       // 当前持续伤害次数
    FVector CurrentEffectLocation;            // 当前特效位置
    ESkillType CurrentEffectSkillType;        // 当前特效技能类型

    FTimerHandle SpecialAbilityTimerHandle;
    FTimerHandle ShieldTimerHandle;
    FTimerHandle EnrageTimerHandle;
};